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

#include "ONUMacCtl_P.h"

Define_Module(ONUMacCtl_P);

ONUMacCtl_P::ONUMacCtl_P(){
	sendReportMsg = new cMessage("Send Report", SEND_REPORT);
}

ONUMacCtl_P::~ONUMacCtl_P(){
	if (startTxMsg)
		cancelAndDelete(startTxMsg);
	if (sendReportMsg)
		cancelAndDelete(sendReportMsg);
}

void ONUMacCtl_P::initialize()
{
	// Do the common stuff...
	ONUMacCtlBase::initialize();

	// That will cause the ONU not to send REPORTs...
	qpl = dynamic_cast<ONUQPerLLiDBase *>(queue_mod);

}

void ONUMacCtl_P::handleMessage(cMessage *msg)
{
	// Sync Clock
	clockSync();

	// Self Message
	if (msg->isSelfMessage())
	{
		EV << "Self-message " << msg << " received\n";

		if (msg == startTxMsg){
			if (transmitState == TX_IDLE) {std::cout<<*msg<<endl; delete msg; error("?"); }
			startTxOnPON();
		}
		else if (msg->getKind()==SEND_REPORT){
			sendReport();
		}
		else
			error("Unknown self message received!");


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
		EV << "ONUMacCtl_P: Message came FROM THE WRONG DIRRECTION???? Dropping\n";
		delete msg;
	}
}


void ONUMacCtl_P::processFrameFromHigherLayer(cMessage *msg){
	EV << "ONUMacCtl_P: Outgoing message, forwarding...\n";
	numFramesFromHL++;

	// Pre-Configuration ... Check and send only MPCP
	EV << "ONUMacCtl_P: Packet " << msg << " arrived from higher layers, sending\n";
	if (start_reg == 0 ){
		EV<<"ONUMacCtl_P: Frame arrived in pre-conf stage "<<endl;

		// Send only if it is an MPCP message
		EthernetIIFrame * frame = dynamic_cast<EthernetIIFrame *>(msg);
		if (frame && frame->getEtherType() == MPCP_TYPE){
			send(msg, "lowerLayerOut");
			return;
		}

		EV<<"ONUMacCtl_P: DROPPING "<<endl;
		delete msg;
		return;
	}

	// Check for a WAKE UP message
	if (msg->getKind() == WAKEUPMSG){
		EV << "Wake UP message received..." << endl;
		// Discard it...
		delete msg;

		// Ignore WAKE UP if we are already transmitting
		if (startTxMsg->isScheduled()) return;
	}else{
		// ENQUEUE
		EV << "Queuing message..." << endl;
		tmp_queue.insert(msg);
	}


	// Check clock and transmit only if it is our time
	dumpReg();
	if (clock_reg>=start_reg && clock_reg<start_reg+len_reg)
		startTxOnPON();



}

void ONUMacCtl_P::processMPCP(EthernetIIFrame *frame ){
	EV << "ONUMacCtl_P: MPCP Frame processing\n";
	MPCP * mpcp = check_and_cast<MPCP *>(frame);

	switch (mpcp->getOpcode())
	{

		case MPCP_GATE:
		{
			MPCPGate * gate = check_and_cast<MPCPGate *>(frame);
			EV << "ONUMacCtl_P: Type is MPCP_GATE\n";

			if (gate->getListLen() == 0) {
				EV << "ONUMacCtl_P: !!! NO ALLOCATION FOR US :-( (bitches...)";
				break;
			}

			if (gate->getListLen() == 1 &&
				gate->getDest().isBroadcast()) {

				EV << "ONUMacCtl_P: MPCP Registration! Ignoring...";
				break;
			}


			// Update with 1st alloc
			start_reg = gate->getStartTime(0);
			len_reg = gate->getDuration(0);
			slotLength = gate->getSlotTime();
			slotNumber = gate->getSlotsNum();

			// Start Tx since we have been polled...
			// DO NOT call startTxOnPON() cause the allocation
			// may be in the future...
			scheduleStartTxMsgFromRegister();


			// Schedule the report
			scheduleReport();


			numGates++;
			break;
		}
		default:
			break;
	};
}

void ONUMacCtl_P::clockSync(){
	EV << "\n\n============= CLOCK: ";
	// NOTE: Clock is in sync with the simulation clock
	// a small skew can be added here (random)


	// Reset start/stop
	uint32_t simT = MPCPTools::simTimeToNS16();

	// Update the clock
	clock_reg = simT;
	EV << clock_reg << "=======================\n";
	return;
}

/**
 * Directly called for start Messages
 * and called if a frame arrives in IDLE.
 */
void ONUMacCtl_P::startTxOnPON(){
	/**
	 * Check to be sure...
	 * THIS SHOULD NEVER HAPPEN
	 */
	if (clock_reg>start_reg+len_reg && start_reg!=0) {
		std::cout<<"ONUMacCtl_P: SHIT: "<<simTime()<<endl;
		std::cout << "Sim(ns16): "<<MPCPTools::simTimeToNS16()<<endl;
		std::cout << "Clock: "<<clock_reg<<endl;
		std::cout << "Start: "<<start_reg<<endl;
		std::cout << "Length: "<<len_reg<<endl;
		return;
	}

	/**
	 * Check that the MAC layer is not in TX mode
	 *
	 * This happens because the 2 messages EndTransmission (EPON_mac)
	 * and startTx (ONUMacCtl) are scheduled at the same EXACT time.
	 *
	 * Some extra delay is used to avoid (internal) message collision
	 * This is set to 5*pow(10,simTime().getScaleExp()); (5*10^-12 in
	 * most cases). This reduces the collision times but do not eliminate
	 * them.
	 *
	 * Therefore, when it happens we make the MacCtl to wait for the
	 * minimum frame transmission time (64Bytes)
	 *
	 */
	if (emac->getQueueLength()>0){
		EV<<"DELAYing MAC busy: "<<simTime()<<endl;
		std::cout<<"DELAYing MAC busy: "<<simTime()<<endl;
		cancelEvent(startTxMsg);
		simtime_t delayTx = ((double)64*8)/txrate;
		simtime_t timerem;
		timerem.setRaw( MPCPTools::ns16ToSimTime( start_reg+len_reg-clock_reg ));
		if (timerem>delayTx)
			scheduleAt(delayTx+simTime(), startTxMsg);
		return;
	}



	/**
	 * Request next packet and set to IDLE state
	 */
	if (tmp_queue.isEmpty()){
		queue_mod->requestPacket();

		// MPCP period
		if (start_reg == 0){
			transmitState = TX_OFF;
			EV << "\n==ONUMacCtl_P: CHANGING STATE FROM _ON_ TO _OFF_\n";
			return;
		}

		EV << "\n==ONUMacCtl_P: CHANGING STATE FROM _ON_ TO _IDLE_ (no message in tmp queue)\n";

		return;
	}


	EV << "\n==ONUMacCtl_P: CHANGING STATE FROM "<<getStateStr()<<" TO _ON_\n";
	transmitState = TX_ON;

	uint32_t nextMsgSize =  ((cPacket *)tmp_queue.front())->getByteLength();



	// Calculate TX and remaining time
	// NOTE: Change here for more bandwidth
	if (nextMsgSize<64) nextMsgSize=64;
	uint32_t bytes=nextMsgSize+PREAMBLE_BYTES+SFD_BYTES;
	bytes+=INTERFRAME_GAP_BITS/8;

	// TODO: Add laser on/off delay
	// TODO: Make a method in MPCPTools for this... (bytes to simtime)
	simtime_t timereq = ((double)bytes*8)/txrate;
	EV << "Total Bytes: "<<bytes<<" Total bits: "<<bytes*8<<" TX RATE: "<<txrate<<endl;

	simtime_t timerem;
	timerem.setRaw( MPCPTools::ns16ToSimTime( start_reg+len_reg-clock_reg ));

	/**
	 * Before requesting data packet check if we should send REPORT
	 */
	if (qpl!=NULL){
		uint16_t repsize = qpl->getMPCPRepSize();

		simtime_t timereq_rep;
		timereq_rep.setRaw(MPCPTools::ns16ToSimTime(
							MPCPTools::bytesToNS16(repsize, 1)
						  ));
		// Total time for both packets
		simtime_t totaltime = timereq_rep+timereq;

		// only one can be send... so send the REPORT
		if (totaltime>=timerem){
			EV << "*** Total Req time (sim): "<<totaltime<<endl;
			EV << "*** Remaining time (sim): "<<timerem<<endl;
			EV << "*** Sending MPCP REPORT : "<<timereq_rep<<endl;
			send(qpl->requestMPCP_REPORT(), "lowerLayerOut");

			numFramesFromHL++;

			// no need to reschedule startTx...
			// end of time slot

			// cancel tho report sending
			if (sendReportMsg->isScheduled()) cancelEvent(sendReportMsg);
			return;
		}
	}

	EV << "*** It will take (sim): "<<timereq<<endl;
	EV << "*** We Still have (sim): "<<timerem<<endl;
	EV << "*** It will take (ns16): "<<MPCPTools::simTimeToNS16(timereq.raw())<<endl;
	EV << "*** We Still have (ns16): "<<MPCPTools::simTimeToNS16(timerem.raw())<<endl;
	dumpReg();

	// If we dont have time break
	if (timerem<timereq){
		/**
		 * Check That he allocated time is more than the frame...
		 * If it is not discard the frame, it is going to block all the
		 * rest...
		 */
		simtime_t totaltime;
		totaltime.setRaw( MPCPTools::ns16ToSimTime( len_reg ));
		if (totaltime<timereq){
			EV << "ONUMacCtl_NP: Frame is too big to FIT THE WHOLE TIME SLOT!\n";
			// Discard and continue (the code)...
			// TODO: Maybe add a counter!
			cancelAndDelete(dynamic_cast<cMessage *>(tmp_queue.pop()));
		}

		// Log fragmented time
		EV << "ONUMacCtl_NP: Fragmented TimeSlot... Frame is too big\n";
		EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM _ON_ TO _OFF_\n";
		transmitState = TX_OFF;
		fragmentedTime += timerem;

		// ON SHIFTING HERE

		return;
	}

	cPacket * msg = dynamic_cast<cPacket *>(tmp_queue.pop());
	// Check the user implementation of the Queues
	if (!msg){
		error(  "Shit... we should never reach here: "
				"UNKNOWN message... NOT A cPACKET");
		return;
	}

	// Add CURRENT TIME STAMP
	MPCP * mpcp = dynamic_cast<MPCP *>(msg);
	if (mpcp){
		mpcp->setTs(clock_reg);
	}

	// We have enough time for the frame...
	//Send
	send(msg, "lowerLayerOut");
	numFramesFromHL++;


	EV << "*** Current simTime (RAW): \t"<<simTime().raw()<<endl;
	EV << "*** Next Message simTime(RAW): \t"<<simTime().raw() + timereq.raw()<<endl;
	EV << "*** Current simTime (ns16): \t"<<MPCPTools::simTimeToNS16(simTime().raw())<<endl;
	EV << "*** Next Message simTime(ns16): \t"<<MPCPTools::simTimeToNS16(simTime().raw() + timereq.raw())<<endl;
	simtime_t nextTx;
	// LOOK at the NOTE at the top...
	nextTx = simTime() + timereq;
	nextTx+= 5*pow(10,simTime().getScaleExp());
	cancelEvent(startTxMsg);
	scheduleAt(nextTx, startTxMsg);
}

void ONUMacCtl_P::scheduleStartTxMsgFromRegister(){
	if (startTxMsg->isScheduled())
		cancelEvent(startTxMsg);

	// Start is in the past... schedule NOW
	if (clock_reg>start_reg)
		EV << "ONUMacCtl_P: WARNING: MPCP GATE WITH START REG IN THE PAST!!!"<<endl;
	if (clock_reg>start_reg && clock_reg<start_reg+len_reg){
		scheduleAt(simTime(), startTxMsg);
		return;
	}

	// Start in the PAST: MISSED TIMESLOT!
	if (clock_reg>start_reg+len_reg){
		EV << "ONUMacCtl_P: WARNING: MISSED SLOT ... clock > start+len!!!"<<endl;
		return;
	}


	// Calc. the difference between clock and start
	uint diff = start_reg - clock_reg;
	simtime_t simoffset;
	simoffset.setRaw(MPCPTools::ns16ToSimTime(diff));

	EV<<"Scheduling StartTxMsg in future: diff="<<diff<<" simoffset="<<simoffset<<endl;
	scheduleAt(simTime()+simoffset, startTxMsg);

}

void ONUMacCtl_P::sendReport(){
	// StartTxOnPon will handle the report
	if (startTxMsg->isScheduled()) return;

	EV << "*** Sending MPCP REPORT ***"<<endl;
	dumpReg();
	// enqueue the frame (in case the Q layer is empty!)
	if (tmp_queue.length()==0)
		tmp_queue.insert(qpl->requestMPCP_REPORT());
	// trigger tx
	startTxOnPON();
	if (tmp_queue.length()==1){
		EV << "*** Removing MPCP REPORT: StartTx re-aquired it ***"<<endl;
		delete tmp_queue.pop();
	}
}

void ONUMacCtl_P::scheduleReport(){
	// Nothing we can do...
	if (qpl==NULL) return;

	// ... +12 for the IFG
	uint16_t repsize = qpl->getMPCPRepSize()+12;

	EV << "REPORT SIZE: "<<repsize<<" ns16="<<MPCPTools::bytesToNS16(repsize, 1)<<endl;
	simtime_t timereq_rep;
	timereq_rep.setRaw(MPCPTools::ns16ToSimTime(
						MPCPTools::bytesToNS16(repsize, 1)
					  ));
	simtime_t slotend;
	// Offset from now to slot end
	uint32_t sendoffset = (start_reg+len_reg)-clock_reg;
	// end of slot in sec
	slotend.setRaw(MPCPTools::ns16ToSimTime(sendoffset));
	slotend = simTime()+slotend;

	EV << "Required time: "<<timereq_rep<<endl;
	EV << "Slot ends at: "<<slotend<<endl;
	EV << "Scheduling REPORT at: "<<slotend-timereq_rep<<endl;
	EV << "NOW: "<<simTime()<<endl;
	scheduleAt(slotend-timereq_rep, sendReportMsg);

}




