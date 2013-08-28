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

#include "OLTMacCtl_NP.h"


Define_Module(OLTMacCtl_NP);

OLTMacCtl_NP::~OLTMacCtl_NP(){
	cancelAndDelete(txEnd);
}

void OLTMacCtl_NP::initialize()
{
	clock_reg=0;

	// Watch the clock!!!
	// (AND TRY NOT TO TOUCH IT)
    WATCH(clock_reg);


    txEnd = new cMessage("TxEnd",TXENDMSG);
    transmitState=TX_IDLE;
    WATCH(transmitState);


    // Initialize the Q mgmt module
    queue_mod = dynamic_cast<IPassiveQueue *>(getNeighbourOnGate("upperLayerOut"));
    if (!queue_mod)
        	error("ONUMacCtl: A IPassiveQueue is needed above mac control");

}

void OLTMacCtl_NP::handleMessage(cMessage *msg)
{

	// Do clock_reg sync
	clockSync();

	// Self Message
	if (msg->isSelfMessage())
	{
		EV << "Self-message " << msg << " received\n";
		if (msg->getKind() == TXENDMSG)
			handleTxEnd();
		else
			EV << "UnKnown Self Message\n";

		return;
	}



	// Network Message
	cGate *ingate = msg->getArrivalGate();
	EV << "Frame " << msg << " arrived on port " << ingate->getName() << "...\n";



	if (ingate->getId() ==  gate( "lowerLayerIn")->getId()){
		processFrameFromMAC(msg);
	}
	else if (ingate->getId() ==  gate( "upperLayerIn")->getId()){
		processFrameFromHigherLayer(msg);
	}else{
		EV << "OLTMacCtl_NP: Message came FROM THE WRONG DIRRECTION???? Dropping\n";
		delete msg;
	}
}

void OLTMacCtl_NP::processFrameFromHigherLayer(cMessage *msg){
	EV << "OLTMacCtl_NP: Incoming to PON area...\n";
	EthernetIIFrame * frame = dynamic_cast<EthernetIIFrame *>(msg);
	if (frame && frame->getEtherType() == MPCP_TYPE){
		MPCP * mpcp = check_and_cast<MPCP *>(msg);
		mpcp->setTs(MPCPTools::simTimeToNS16());
	}


	EV << "OLTMacCtl_NP::Enqueue Frame" <<endl;
	tmp_queue.insert(msg);


	// handle tx end will react like
	// a transmission just finished ==
	// check the Q module and keep on sending.
	if (transmitState == TX_IDLE){
		EV << "OLTMacCtl_NP::We where IDLE... starting transmission" <<endl;
		handleTxEnd();
	}
}

void OLTMacCtl_NP::processFrameFromMAC(cMessage *msg){
	EV << "OLTMacCtl_NP: Message from PON area, forwarding to higher layer\n";

	// Not Control
	send(msg,"upperLayerOut");
	numFramesFromLL++;
}

/**
 * Handle transmissions and queue.
 */
void OLTMacCtl_NP::doTransmit(cMessage * msg){
	cPacket * packet = check_and_cast<cPacket *>(msg);

	if (!packet)
			error("Queue module returned NULL frame");


	// Calculate TX and remaining time
	// NOTE: Change here for more bandwidth
	uint64_t txrate=GIGABIT_ETHERNET_TXRATE;

	uint32_t nextMsgSize =  packet->getByteLength();
	if (nextMsgSize == 0) {
		error("Message size 0");
	}
	uint32_t bytes=nextMsgSize+PREAMBLE_BYTES+SFD_BYTES;
	bytes+=INTERFRAME_GAP_BITS/8;


	// TODO: Add laser on/off delay
	simtime_t timereq= ((double)(bytes*8)/txrate);
	EV << "Bytes: "<<bytes<<" bits: "<<bytes*8<<" TX RATE: "<<txrate<<endl;
	EV << "TX State: "<<transmitState<<endl;
	EV << "Scheduled after "<<(double)(bytes*8)/txrate<<"ns"<<endl;

	if (transmitState == TX_IDLE){
		transmitState=TX_SENDING;
		// Calculate the tx time
		EV << "EndTx Scheduled after "<< timereq.raw() << " time_now: "<<simTime().raw()<<endl;
		scheduleAt(simTime()+timereq, txEnd);
		EV << " Sending..."<<endl;
		send(msg, "lowerLayerOut");
	}
	else{
		error ("OLTMacCtl_NP: Packet Arrived from higher layer"
				"while we where in transmission. This should never happen"
				"(normally).");
	}
}


void OLTMacCtl_NP::handleTxEnd(){
	transmitState=TX_IDLE;
	if (tmp_queue.isEmpty()) {
		queue_mod->requestPacket();
		return;
	}

	doTransmit(dynamic_cast<cMessage *>(tmp_queue.pop()) );
}

/*
 * Keep track of the clock register
 */
void OLTMacCtl_NP::clockSync(){
	clock_reg=MPCPTools::simTimeToNS16();
}


// TOOLS
cModule * OLTMacCtl_NP::getNeighbourOnGate(const char * g){
	return gate(g)->getNextGate()->getOwnerModule();
}
