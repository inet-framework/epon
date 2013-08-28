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

#include "ONUMacCtl_NP.h"

Define_Module(ONUMacCtl_NP);

ONUMacCtl_NP::ONUMacCtl_NP(){
	startTxMsg=0;
	stopTxMsg=0;
}

ONUMacCtl_NP::~ONUMacCtl_NP(){

	cancelAndDelete(startTxMsg);
	cancelAndDelete(stopTxMsg);

}

void ONUMacCtl_NP::initialize()
{
	// Do the common stuff...
	ONUMacCtlBase::initialize();

	stopTxMsg = new cMessage("stopTxMsg", STOPTXMSG);

}

void ONUMacCtl_NP::handleMessage(cMessage *msg)
{

	// Update clock from simTime
	// The clock MUST be in sync
	// when stop is scheduled. Stop
	// is called only when entering IDLE
	clockSync();

	// Self Message
	if (msg->isSelfMessage())
	{
		EV << "Self-message " << msg << " received\n";

		if (msg == startTxMsg){
			if (transmitState == TX_IDLE) {std::cout<<*msg<<endl; delete msg; error("?"); }
			startTxOnPON();
		}
		else if (msg == stopTxMsg)
			handleStopTxPeriod();
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
		EV << "ONUMacCtl_NP: Message came FROM THE WRONG DIRRECTION???? Dropping\n";
		delete msg;
	}

}

void ONUMacCtl_NP::processFrameFromHigherLayer(cMessage *msg){
	EV << "ONUMacCtl_NP: Outgoing message, forwarding...\n";
	numFramesFromHL++;

	// Pre-Configuration ... Check and send only MPCP
	EV << "ONUMacCtl_NP: Packet " << msg << " arrived from higher layers, sending\n";
	if (start_reg == 0 ){
		EV<<"ONUMacCtl_NP: Frame arrived in pre-conf stage "<<endl;

		// Send only if it is an MPCP message
		EthernetIIFrame * frame = dynamic_cast<EthernetIIFrame *>(msg);
		if (frame && frame->getEtherType() == MPCP_TYPE){
			send(msg, "lowerLayerOut");
			return;
		}

		EV<<"ONUMacCtl_NP: DROPPING "<<endl;
		delete msg;
		return;
	}

	// Check for a WAKE UP message
	if (msg->getKind() == WAKEUPMSG){
		EV << "Wake UP message received..." << endl;
		// Discard it...
		delete msg;
	}else{
		// ENQUEUE
		EV << "Queuing message..." << endl;
		tmp_queue.insert(msg);
	}


	// Re-schedule tx period
	// Check if we are idle, if yes start transmission
	if (transmitState == TX_IDLE){
		EV << "ONUMacCtl_NP: CHANGING STATE FROM _IDLE_ TO _ON_\n";
		// Cancel Previous DeadLine
		cancelEvent(stopTxMsg);
		startTxOnPON();
	}
	// Check if we are asleep
	else if (transmitState == TX_SLEEP){
		// IF start_reg is in the future just reschedule startTX
		// ELSE shift the clock the lost slots...
		if (clock_reg < start_reg){
			cancelEvent(startTxMsg);
			scheduleStartTxPeriod();
		}else if (clock_reg >= start_reg && clock_reg < start_reg + len_reg){
			cancelEvent(stopTxMsg);
			startTxOnPON();
		}else {
			// Super Slot ...
			uint64_t slotTime16ns = (double)slotLength/16*slotNumber;
			uint32_t lostSlots = ceil((double)(clock_reg - start_reg)/slotTime16ns);
			shiftStartXSlots(lostSlots);
		}
	}



}



void ONUMacCtl_NP::processMPCP(EthernetIIFrame *frame ){
	EV << "ONUMacCtl_NP: MPCP Frame processing\n";
	MPCP * mpcp = check_and_cast<MPCP *>(frame);


	// DONT... DONT EVEN THINK OF IT
//	EV << "ONUMacCtl_NP: Updating our clock from: "<<clock_reg<<" to ";
//	clock_reg=mpcp->getTs();
//	EV << clock_reg << endl;

	switch (mpcp->getOpcode())
	{

		case MPCP_GATE:
		{
			MPCPGate * gate = check_and_cast<MPCPGate *>(frame);
			EV << "ONUMacCtl_NP: Type is MPCP_GATE\n";

			if (gate->getListLen() == 0) {
				EV << "ONUMacCtl_NP: !!! NO ALLOCATION FOR US :-( (bitches...)";
				break;
			}


			// Update with 1st alloc
			start_reg = gate->getStartTime(0);
			len_reg = gate->getDuration(0);
			slotLength = gate->getSlotTime();
			slotNumber = gate->getSlotsNum();


			/**
			 *  NOTE: Announced time IS NOT ALWAYS ON THE FUTURE
			 *  IF we want the announced times to be on the future
			 *  then the DBA algorithms MUST CONSIDER RTT. For simplicity
			 *  we do accept to loose some slots...
			 */
			if ( numGates == 0){
				EV << "ONUMacCtl_NP: MPCP_GATE arrived, IT IS INITIAL - MAC: "<<frame->getDest()<<endl;
				// Cancel ALL an scheduled TX
				cancelEvent(startTxMsg);
				cancelEvent(stopTxMsg);
				scheduleStartTxPeriod();
			}else{

				if (clock_reg>start_reg + len_reg){
					// Time assigned is in past
					EV << "ONUMacCtl_NP: MPCP_GATE arrived, calculating lost slots"<<endl;

					// Super Slot ...
					uint64_t slotTime16ns = (double)slotLength/16*slotNumber;
					uint32_t lostSlots = ceil((double)(clock_reg - start_reg)/slotTime16ns);
					shiftStartXSlots(lostSlots);
				}else if (clock_reg>=start_reg && clock_reg < start_reg + len_reg){
					EV << "ONUMacCtl_NP: MPCP_GATE arrived, it is our time"<<endl;
					// Now is our time
					scheduleStopTxPeriod();
					startTxOnPON();
				}else{
					// Time assigned is in future
					EV << "ONUMacCtl_NP: MPCP_GATE arrived, no lost slots"<<endl;
					scheduleStartTxPeriod();
				}
			}


			numGates++;
			break;
		}
		default:
			break;
	};

}


void ONUMacCtl_NP::scheduleStartTxPeriod(){

	EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM ?? TO _OFF_\n";
	transmitState = TX_OFF;


	/*
	 *  start_reg shifted clock_reg did not
	 *  HANDLE AND RETURN.
	 *
	 *  NOTE: 	This should occur only on shifted registers.
	 *  		MPCP could have the same result but we are fixing
	 *  		it by shifting the clock on receive.
	 */
	if (start_reg!=0 && (uint64_t)start_reg+len_reg<clock_reg) {
		EV<<"ONUMacCtl_NP: Start_reg shifted clock_reg did not"<<endl;
		dumpReg();
		//error("start_reg+len_reg<clock_reg ");


		simtime_t nextTx;

		nextTx.setRaw(simTime().raw() + // NOW
				MPCPTools::ns16ToSimTime(MPCP_CLOCK_MAX - clock_reg + // Remaining (not shifted clock)
						start_reg
					));
		EV<<"...."<<(MPCP_CLOCK_MAX - clock_reg +start_reg)<<endl;
		EV<<"scheduleStartTxPeriod: "<<nextTx<<endl;

		cancelEvent(startTxMsg);
		scheduleAt(nextTx, startTxMsg);
		return;
	}

	simtime_t nextTx;
	// Start the next moment...
	nextTx.setRaw(simTime().raw());

	// If start time is ahead sim time Start now
	// clock_reg == MPCPTools::simTimeToNS16()....
	if (start_reg>=clock_reg){
		nextTx.setRaw(simTime().raw()+								//Now (NOTE: not the clock_reg)
					  MPCPTools::ns16ToSimTime(start_reg-clock_reg) //Remaining time to Start
					 );
		EV << "ONUMacCtl_NP: Start scheduled. Clock: "<<clock_reg<<" Start: "<<start_reg<<endl;
	}else{
		EV << "ONUMacCtl_NP: Starting NOW. Clock: "<<clock_reg<<" Start: "<<start_reg<<endl;
	}


	cancelEvent(startTxMsg);
	scheduleAt(nextTx, startTxMsg);
}


/*
 * 			     length
 *              ------------
 * time --------|----|-----|------
 *         start^    \     ^stop
 *         		 	  \now
 */
void ONUMacCtl_NP::scheduleStopTxPeriod(){

	// You cannot calculate next time from registers...
	// Simulation time may be different (regs are shifted)
	// So calculate the remaining time...
	simtime_t stopTX;
	stopTX.setRaw( simTime().raw() + 						 // Now
			MPCPTools::ns16ToSimTime(start_reg +len_reg - clock_reg) // Remaining
	);
	dumpReg();
	EV << "Scheduled after (ns16): " << (start_reg +len_reg - clock_reg) <<endl;
	EV << "Scheduled after  (raw): " << MPCPTools::ns16ToSimTime(start_reg +len_reg - clock_reg) <<endl;

	// Just in case..
	cancelEvent(stopTxMsg);
	scheduleAt(stopTX, stopTxMsg);
}


void ONUMacCtl_NP::handleStopTxPeriod(){

	transmitState = TX_OFF;
	EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM ?? TO _OFF_\n";

	/*
	 *  Do NOT call start all the time...
	 *
	 *  WE TRIED... :-)
	 */
	if (tmp_queue.isEmpty()){
		goToSLEEP();
		// DO NOT RESCHEDULE START...
		return;
	}


	// Shift the start_reg
	shiftStart1Slot();
}

void ONUMacCtl_NP::goToIDLE(){
	transmitState = TX_IDLE;
	cancelEvent(stopTxMsg);
	cancelEvent(startTxMsg);
	scheduleStopTxPeriod();
}

void ONUMacCtl_NP::goToSLEEP(){
	EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM ?? TO _SLEEP_\n";
	queue_mod->requestPacket();
	transmitState = TX_SLEEP;
	return;
}


/**
 * Directly called for start Messages
 * and called if a frame arrives in IDLE.
 */
void ONUMacCtl_NP::startTxOnPON(){
	/**
	 * Check to be sure...
	 *
	 * Sometimes because we add 0.001 to startTx schedule, results to
	 * clock_reg = start+len +1
	 *
	 * So, shift a slot.
	 */
	if (clock_reg>start_reg+len_reg && start_reg!=0) {
		std::cout<<"ONUMacCtl_NP: SHIT: "<<simTime()<<endl;
		std::cout << "Sim(ns16): "<<MPCPTools::simTimeToNS16()<<endl;
		std::cout << "Clock: "<<clock_reg<<endl;
		std::cout << "Start: "<<start_reg<<endl;
		std::cout << "Length: "<<len_reg<<endl;
		shiftStart1Slot();
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
		cancelEvent(startTxMsg);
		simtime_t delayTx = ((double)64*8)/txrate;
		simtime_t timerem;
		timerem.setRaw( MPCPTools::ns16ToSimTime( start_reg+len_reg-clock_reg ));
		if (timerem>delayTx){
			scheduleAt(delayTx+simTime(), startTxMsg);

		}else{
			shiftStart1Slot();
		}
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
			EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM _ON_ TO _OFF_\n";
			return;
		}

		EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM _ON_ TO _IDLE_ (no message in tmp queue)\n";
		goToIDLE();

		return;
	}


	EV << "\n==ONUMacCtl_NP: CHANGING STATE FROM "<<getStateStr()<<" TO _ON_\n";
	transmitState = TX_ON;

	uint32_t nextMsgSize =  ((cPacket *)tmp_queue.front())->getByteLength();


	// Calculate TX and remaining time
	// NOTE: Change here for more bandwidth
	if (nextMsgSize<64) nextMsgSize=64;
	uint32_t bytes=nextMsgSize+PREAMBLE_BYTES+SFD_BYTES;
	bytes+=INTERFRAME_GAP_BITS/8;

	// TODO: Add laser on/off delay
	simtime_t timereq = ((double)bytes*8)/txrate;
	EV << "Total Bytes: "<<bytes<<" Total bits: "<<bytes*8<<" TX RATE: "<<txrate<<endl;

	simtime_t timerem;
	timerem.setRaw( MPCPTools::ns16ToSimTime( start_reg+len_reg-clock_reg ));

	EV << "*** It will take (sim): "<<timereq<<endl;
	EV << "*** We Still have (sim): "<<timerem<<endl;
	EV << "*** It will take (ns16): "<<MPCPTools::simTimeToNS16(timereq.raw())<<endl;
	EV << "*** We Still have (ns16): "<<MPCPTools::simTimeToNS16(timerem.raw())<<endl;
	dumpReg();

	// If we dont have time break
	if (timerem<timereq){
		/**
		 * Check That he allocated time is more than the frame...
		 * If it is not discart the frame, it is going to block all the
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

		// Shift the start_reg
		shiftStart1Slot();
		// cancel the stop message
		// (else we lose a time slot)
		cancelEvent(stopTxMsg);

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
	// LOOK at the NOTE on the begging
	nextTx = simTime() + timereq;
	nextTx+= 5*pow(10,simTime().getScaleExp());
	cancelEvent(startTxMsg);
	scheduleAt(nextTx, startTxMsg);
}


void ONUMacCtl_NP::clockSync(){

	EV << "\n\n============= CLOCK: ";
	// NOTE: Clock is in sync with the simulation clock
	// a small skew can be added here (random)


	// Reset start/stop
	uint32_t simT = MPCPTools::simTimeToNS16();


	// No clock shift
	if (simT>=clock_reg){
		// Update the clock
		clock_reg = simT;
		EV << clock_reg << "=======================\n";
		return;
	}

	/// Multi shift
	if (start_reg<simT){
		EV << "MULTI SHIFT =====================\n" <<endl;
		clock_reg = simT;
		// Super Slot ...
		uint64_t slotTime16ns = (double)slotLength/16*slotNumber;
		uint32_t lostSlots = ceil((double)(clock_reg - start_reg)/slotTime16ns);
		shiftStartXSlots(lostSlots);
		return;
	}

	// Single shift (1 reg loop)

	/**
	 * Check Slot Shift...
	 * NOTE: skew introduced
	 */

	// Reschedule the clock
	// In ANY case start reg is wrong
	transmitState = TX_OFF;
	cancelEvent(startTxMsg);
	cancelEvent(stopTxMsg);

	EV << " SHIFT =======================\n";


	// Update the clock
	clock_reg = simT;

	dumpReg();


	EV << "===========================================\n\n\n";

}



void ONUMacCtl_NP::shiftStart1Slot(){

	uint64_t slotTime16ns = ((double)slotLength/16)*slotNumber;

	EV << "Increasing Start Reg.: old=" << start_reg
		<<" + " << slotTime16ns;

	// NOTE: start_reg may OVERFLOW TOOooo...  tsouf
	start_reg = ((uint64_t)start_reg+slotTime16ns)%MPCP_CLOCK_MAX;


	EV<<" = "<<start_reg<<" Clock Now: "<<clock_reg<<endl;
	EV<<"SlotLength: "<<slotTime16ns<<endl<<endl;


	// Reschedule next TX
	scheduleStartTxPeriod();
}

void ONUMacCtl_NP::shiftStartXSlots(uint32_t X){
	uint64_t slotTime16ns = (double)slotLength/16*slotNumber;

	EV << "Increasing Start: old=" << start_reg
		<<" + " << X*slotTime16ns;

	start_reg = ((uint64_t)start_reg+X*slotTime16ns)%MPCP_CLOCK_MAX;


	EV<<" = "<<start_reg<<endl;
	EV<<"Lost Slots: "<<X<<" SlotLength: "<<slotTime16ns<<endl<<endl;

	// Reschedule next TX
	scheduleStartTxPeriod();
}





