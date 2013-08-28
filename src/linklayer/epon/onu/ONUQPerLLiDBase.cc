//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "ONUQPerLLiDBase.h"


ONUQPerLLiDBase::ONUQPerLLiDBase(){
	regTOMsg = new cMessage("regTOMsg", REGTOMSG);
	regSendMsg = new cMessage("regSendMsg", REGSENDMSG);

	allQsBlocked=false;
	allQsEmpty=true;
	nextQIndex=0;
}
ONUQPerLLiDBase::~ONUQPerLLiDBase(){
	cancelAndDelete(regSendMsg);
	cancelAndDelete(regTOMsg);

	for (uint32_t i=0; i<pon_queues.size(); i++){
		pon_queues[i].clean();
	}
	pon_queues.clear();
}

void ONUQPerLLiDBase::initialize(int stage)
{
	if (stage == 0){
		numOfLLIDs=1;
		serviceList=NULL;

		if (dynamic_cast<ServiceConfig *>( findModuleUp("serviceConfig")) != NULL){
			serviceList = &(dynamic_cast<ServiceConfig *>( findModuleUp("serviceConfig"))->srvs);
			// 1 llid per service
			numOfLLIDs = serviceList->size();
		}
		// Parameters
		regTimeOut = par("regTimeOut");
		regTimeOut/=1000;
		queueLimit = par("queueLimit");



		// Parameters
		granularity = par("statsGranularity").doubleValue();


		// Create Q for BroadCasts
		QueuePerLLid tmp_qpllid;
		tmp_qpllid.setServiceName("BroadCast");
		tmp_qpllid.prior = 0.0;
		tmp_qpllid.llid = LLID_EPON_BC;
		tmp_qpllid.vec->setGranularity(granularity);
		tmp_qpllid.queueLimit = queueLimit;
		addSortedLLID(tmp_qpllid);

		WATCH_VECTOR(pon_queues);


		// Get olt module
		olt_mac = simulation.getModuleByPath("epon_olt.olt_if.epon_mac");
		if (!olt_mac)
			opp_error("Cannot get EPON MAC module for registration!");
	}else if (stage == 1){
		EtherMACBase *mac = dynamic_cast<EtherMACBase *>
			(gate("lowerLayerOut")->getNextGate()->getOwnerModule()->gate("lowerLayerOut")->getNextGate()->getOwnerModule());
		opt_mac = mac->getMACAddress();
	}
}

void ONUQPerLLiDBase::handleMessage(cMessage *msg)
{

	// Self Message
	if (msg->isSelfMessage())
	{
		EV << "Self-message " << msg << " received\n";

		if (msg == regTOMsg){
			EV << "*** ONUMacCtl: Registration FAILED"<<endl;
		}else if (msg == regSendMsg)
			sendMPCPReg();
		else
			error("Unknown self message received!");

		return;
	}


	// Network Message
	cGate *ingate = msg->getArrivalGate();
	EV << "Frame " << msg << " arrived on port " << ingate->getName() << "...\n";

	if (ingate->getId() ==  gate( "lowerLayerIn")->getId()){
		processFrameFromLowerLayer(msg);
	}
	else if (ingate->getId() ==  gate( "upperLayerIn")->getId()){
		// Add the frame to the proper Q
		processFrameFromHigherLayer(msg);
	}else{
		EV << "ONUMacCtl: Message came FROM THE WRONG DIRRECTION???? Dropping\n";
		delete msg;
	}

}


void ONUQPerLLiDBase::processFrameFromHigherLayer(cMessage *msg){
	EV << "ONUQPerLLiDBase: Incoming from higher layer...\n";


	EPON_LLidCtrlInfo *nfo = dynamic_cast<EPON_LLidCtrlInfo *>(msg->getControlInfo());

	// If frame has no info or is BC add to the BroadCast Q
	int Q_index=-1;
	if (!nfo){
		Q_index = getIndexForLLID(getDefaultLLiD());
	}else{
		Q_index = getIndexForLLID(nfo->llid);
	}

	// Check if we can forward frame
	if (Q_index == -1){
		EV << "*** WRONG LLID : DROPPING" <<endl;
		delete msg;
		return;
	}

	// Update the incoming rate BEFORE discarding message
	// in case the Q is full.
	cPacket *pkt = dynamic_cast<cPacket *>(msg);
	pon_queues[Q_index].vec->numIncomingBits+=pkt->getBitLength();

	// IF all empty send... WAKE UP AND ENQUEUE
	// if you send the message directly, it will still work
	// But subclasses do not get the chance to inspect Q polices
	// like data rate limit!
	if (allQsEmpty || allQsBlocked){
		allQsEmpty = false;
		allQsBlocked = false;

		EV << "Sending WAKE UP message to MAC"<<endl;
		send(new cMessage("WAKE UP", WAKEUPMSG), "lowerLayerOut");
	}

	// Check that Q is not full
	if (pon_queues[Q_index].length() >= pon_queues[Q_index].queueLimit){
		pon_queues[Q_index].vec->numBytesDropped+=pkt->getByteLength();
		pon_queues[Q_index].vec->recordVectors();
		EV << "Dropping"<<endl;
		delete msg;
		return;
	}else{
		// Record the incoming bytes
		EV << "Enqueue"<<endl;
		pon_queues[Q_index].vec->recordVectors();
	}


	// Finally add to the Q
	pon_queues[Q_index].insert(dynamic_cast<cPacket *>(msg));




}

void ONUQPerLLiDBase::processFrameFromLowerLayer(cMessage *msg){
	EV << "ONUQPerLLiDBase: Incoming from lower layer...\n";

	EthernetIIFrame * frame = dynamic_cast<EthernetIIFrame *>(msg);


	if (frame && frame->getEtherType() == MPCP_TYPE){
		processMPCP(frame );
		return;
	}

	// Drop if it is not for us based on LLID
	if (!checkLLIDforUs(msg)) {
		delete msg;
		return;
	}

	send(msg,"upperLayerOut");
}

bool ONUQPerLLiDBase::checkLLIDforUs(cMessage *msg){
	// Drop if it is not for us based on LLID
	EPON_LLidCtrlInfo * nfo =  dynamic_cast<EPON_LLidCtrlInfo *>(msg->getControlInfo());
	// Check for BC
	if (nfo && nfo->llid != LLID_EPON_BC){
		// -1 = No such llid
		if (getIndexForLLID(nfo->llid) == -1) {
			return false;
		}
	}
	return true;
}


void ONUQPerLLiDBase::processMPCP(EthernetIIFrame *frame ){
	EV << "ONUMacCtl: MPCP Frame processing\n";
	MPCP * mpcp = check_and_cast<MPCP *>(frame);


	switch (mpcp->getOpcode())
	{
		case MPCP_REGISTER:
		{

			MPCPRegAck *ack = new MPCPRegAck();
			MPCPRegister * reg = check_and_cast<MPCPRegister *>(frame);

			EV << "ONUMacCtl: Type is MPCP_REGISTER\n";
			cancelEvent(regTOMsg);
			EV << "ONUMacCtl: Canceling RegTOMsg\n";

			ack->setOpcode(MPCP_REGACK);
			ack->setName("MPCPAck");
			ack->setEtherType(MPCP_TYPE);
			ack->setDest(mpcp->getSrc());
			ack->setByteLength(MPCP_HEADER_LEN);

			int LLID_num=reg->getPtpNumReg();

			EV<< "Assigned LLIDs:" <<LLID_num<<endl;
			for (uint8_t i=0; i<reg->getLLIDsArraySize(); i++){
				// 0xFFF = 4095 intrand -> [0-4095)
				EV<< (int)i<<"  " <<(int)reg->getLLIDs(i)<<endl;
				QueuePerLLid tmp_qpllid;
				tmp_qpllid.llid = reg->getLLIDs(i);
				std::string tmp_name="Default";
				tmp_qpllid.prior = 0.0;
				if (serviceList && serviceList->size() > i) {
					tmp_name=serviceList->at(i).name;
					EV<<tmp_name<<" -- "<<reg->getLLIDs(i)<<endl;
					tmp_qpllid.prior = serviceList->at(i).priority;
					tmp_qpllid.isDefault = false;
				}

				// Use the last LLiD as the default
				if ( reg->getLLIDsArraySize() -1 == i){
					EV << "Setting the default LLiD (BE) to "<<tmp_qpllid.llid<<endl;
					tmp_name="Default";
					tmp_qpllid.prior = 0.0001;
					tmp_qpllid.isDefault = true;
				}

				tmp_qpllid.setServiceName(tmp_name);
				tmp_qpllid.queueLimit = queueLimit;
				tmp_qpllid.vec->setGranularity(granularity);
				addSortedLLID(tmp_qpllid);

			}

			//send(ack,"lowerLayerOut");
			// Urb@n: Send directly on upstream to avoid collisions
			ack->addByteLength(PREAMBLE_BYTES);
			ack->setSrc(opt_mac);
			sendDirect(ack, olt_mac, olt_mac->gate("direct")->getId());

			// Send the frame on top layer that manages LLIDs
			send(frame->dup(),"upperLayerOut");

			break;
		}
		case MPCP_GATE:
		{
			MPCPGate * gate = check_and_cast<MPCPGate *>(frame);
			EV << "ONUQPerLLiDBase: Type is MPCP_GATE\n";


			// Register Grant
			if (gate->getListLen() == 1
				&& gate->getDest().isBroadcast())
			{
				EV << "ONUMacCtl: MPCP REGISTER GRANT (DOOUUU)" <<endl;
				// Process here number of llids...
				/**
				 * +1 Cause the last one is going to be the BE or
				 * Default queue...
				 */
				if (serviceList) numOfLLIDs = serviceList->size()+1;
				startMPCPReg(gate->getDuration(0));
				break;
			}

			break;
		}
		default:
			EV << "ONUMacCtl: Unrecognized MPCP OpCode!!\n";
			return;
	};

	delete frame;
}
uint16_t ONUQPerLLiDBase::getMPCPRepSize(){
	return MPCP_HEADER_LEN+1+pon_queues.size()*2;
}
MPCPReport * ONUQPerLLiDBase::requestMPCP_REPORT(){
	MPCPReport * rep = new MPCPReport();
	rep->setOpcode(MPCP_REPORT);
	rep->setEtherType(MPCP_TYPE);
	// TODO: replace with OLT mac... at some point
	rep->setDest(MACAddress("FF:FF:FF:FF:FF:FF"));
	rep->setSrc(opt_mac);

	rep->setQInfoArraySize(pon_queues.size());

	uint8_t bitFlag=0;
	// For Each Queue
	for (uint i=0; i<pon_queues.size(); i++){
		// Enable the proper bit
		bitFlag|=(0x01<<(7-i));
		EV<<"BF: "<<bitFlag<<endl;

		// Convert bits in the queue to ns16 request
		// TODO: hardcoded line rate 1Gpbs
		uint32_t tmpreq=MPCPTools::bitsToNS16(pon_queues[i].getBitLength(), 1);

		// Set the requested bandwidth...
		rep->setQInfo(i, tmpreq);
	}

	// Set the Map
	rep->setBitMap(bitFlag);

	// Header + bitMap + #*uint16 (uint32 is used to avoid overflows...)
	rep->setByteLength(getMPCPRepSize());

	// TODO: the std say that it should be mapped to an LLID
	// use EtherFrameWithLLID...
	return rep;
}


void ONUQPerLLiDBase::sendMPCPReg(){



	MPCPRegReq *regreq = new MPCPRegReq();
	regreq->setDest(MACAddress("FF:FF:FF:FF:FF:FF"));
	regreq->setEtherType(MPCP_TYPE);
	regreq->setName("MPCPRegReq");
	regreq->setOpcode(MPCP_REGREQ);
	regreq->setPtpNumReq(numOfLLIDs);

	regreq->setByteLength(MPCP_HEADER_LEN+MPCP_LIST_LEN);

	// Send REG REQ and Schedule a timeout
	//send(regreq, "lowerLayerOut");

	// Urb@n: Send directly on upstream to avoid collisions
	regreq->addByteLength(PREAMBLE_BYTES);
	regreq->setSrc(opt_mac);
	sendDirect(regreq, olt_mac, olt_mac->gate("direct")->getId());

	scheduleAt(simTime() +regTimeOut, regTOMsg );
}



void ONUQPerLLiDBase::startMPCPReg(uint32_t regMaxRandomSleep){

	uint32_t rndBackOff = dblrand() * regMaxRandomSleep;
	EV<<"Sending MPCP REGREQ in " << rndBackOff << "(< " <<regMaxRandomSleep<<")"<< endl;
	simtime_t t;
	t.setRaw(simTime().raw() + MPCPTools::ns16ToSimTime(rndBackOff));
	scheduleAt(t, regSendMsg );
}

int ONUQPerLLiDBase::getIndexForLLID(uint16_t llid){
	for (uint32_t i=0; i<pon_queues.size(); i++)
		if (pon_queues[i].llid == llid) return i;

	return -1;
}

int ONUQPerLLiDBase::getDefaultLLiD(){
	for (uint32_t i=0; i<pon_queues.size(); i++)
		if (pon_queues[i].isDefault) return pon_queues[i].llid;

	return LLID_EPON_BC;
}

int ONUQPerLLiDBase::getIndexForService(std::string name){
	for (uint32_t i=0; i<pon_queues.size(); i++)
			if (pon_queues[i].getServiceName() == name)
				return i;

	return -1;
}

void ONUQPerLLiDBase::addSortedLLID(QueuePerLLid tmp_qpllid){
	// Just add it if empty...
	if (pon_queues.empty()) {
		pon_queues.push_back(tmp_qpllid);
		return;
	}

	// Add in the proper possition
	for (PonQueues::iterator it = pon_queues.begin(); it != pon_queues.end(); it++){
		if ((*it).prior < tmp_qpllid.prior){
			pon_queues.insert(it, tmp_qpllid);
			return;
		}
	}

	// If not added till here... add to the end
	pon_queues.push_back(tmp_qpllid);
}

/**
 * Check is all the Qs are empty. IF yes set
 * the all empty variable. This must be done after
 * the frame request from the lower layer.
 */
void ONUQPerLLiDBase::checkIfAllEmpty(){
	if (pon_queues.size() == 0) allQsEmpty = true;
	if (pon_queues[0].isEmpty()
			&& pon_queues.size() == 1) allQsEmpty = true;


	bool found=false;
	// if the current Q is empty -> find the next one
	if (pon_queues[nextQIndex].isEmpty()){
		for (int i=0; i<(int)pon_queues.size(); i++){
			if (!pon_queues[i].isEmpty()) {
				found=true;
				break;
			}
		}
	// If not empty.. we have a frame
	}else found=true;

	allQsEmpty = !found;
}

cModule * ONUQPerLLiDBase::findModuleUp(const char * name){
	cModule *mod = NULL;
	for (cModule *curmod=this; !mod && curmod; curmod=curmod->getParentModule())
	     mod = curmod->getSubmodule(name);
	return mod;
}


void ONUQPerLLiDBase::finish(){
	simtime_t t = simTime();

	for (uint32_t i=0; i<pon_queues.size(); i++){
		std::string srvName=pon_queues[i].getServiceName();
		std::string name=srvName+" BytesDropped";
		recordScalar(name.c_str(), pon_queues[i].vec->numBytesDropped  );
		name=srvName+" BytesSent";
		recordScalar(name.c_str(), pon_queues[i].vec->numBytesSent  );

		if (t>0)
		{
			name=srvName+" bytes dropped/sec";
			unsigned long dropped = pon_queues[i].vec->numBytesDropped;
			recordScalar(name.c_str(), dropped/t);
			name=srvName+" bytes sent/sec";
			unsigned long sent = pon_queues[i].vec->numBytesSent;
			recordScalar(name.c_str(), sent/t);
			name=srvName+" Drop PerCent";
			if (dropped+sent  == 0)
				recordScalar(name.c_str(), 0.0);
			else
				recordScalar(name.c_str(), (double)100*((double)dropped/(dropped+sent)));
		}
	}
}

bool ONUQPerLLiDBase::isEmpty(){
	checkIfAllEmpty();
	return allQsEmpty;
}

int ONUQPerLLiDBase::getNumPendingRequests()
{
    return 0;
}
void ONUQPerLLiDBase::clear(){}
void ONUQPerLLiDBase::addListener(IPassiveQueueListener *listener){}
void ONUQPerLLiDBase::removeListener(IPassiveQueueListener *listener){}
