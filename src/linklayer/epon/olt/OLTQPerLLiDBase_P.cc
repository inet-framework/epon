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

#include "OLTQPerLLiDBase_P.h"

Define_Module(OLTQPerLLiDBase_P);

OLTQPerLLiDBase_P::OLTQPerLLiDBase_P(){
	sendGateMsg = new cMessage("sendGateMsg", SEND_GATE);
}
OLTQPerLLiDBase_P::~OLTQPerLLiDBase_P(){
	cancelAndDelete(sendGateMsg);
}

void OLTQPerLLiDBase_P::initialize()
{
    // Init base
	OLT_QPL_RR::initialize();

	// Get our parameters
	wMax = par("wMax").doubleValue();
	fixedWin = par("fixedWin").doubleValue();

	pollerRunning=false;
	nextONU=0;
	expTxTime=simTime();

	WATCH(nextONU);
	WATCH(pollerRunning);
	WATCH(expTxTime);
}

void OLTQPerLLiDBase_P::handleMessage(cMessage *msg)
{
    if (msg->getKind() == SEND_GATE){
    	sendSingleGate();
    	return;
    }

    // Call base...
    OLT_QPL_RR::handleMessage(msg);
}

void OLTQPerLLiDBase_P::SendGateUpdates(){
	EV<<"** OLTQPerLLiDBase_P::SendGateUpdates CANCELED **"<<endl;
}

void OLTQPerLLiDBase_P::sendSingleGate(){
	MPCPGate *gt = new MPCPGate();
	gt->setName("MPCPGate");
	gt->setEtherType(MPCP_TYPE);
	gt->setOpcode(MPCP_GATE);
	MPCPTools::setGateLen(*gt, 1);


	gt->setSlotTime(slotLength);
	gt->setSlotsNum(slotNumber);

	gt->setDest(onutbl->getEntry(nextONU)->getId());
	gt->setStartTime(0, onutbl->getEntry(nextONU)->getComTime().start);
	gt->setDuration(0, onutbl->getEntry(nextONU)->getComTime().length);

	// Header + List + (start + Len) + slotNum + slotLen
	gt->setByteLength(MPCP_HEADER_LEN+MPCP_LIST_LEN+MPCP_TIMERS_LEN+MPCP_SLOTINFO_LEN);


	// Send directly
	send(gt, "lowerLayerOut");

	// Now update the nextONU and call for DBA again...
	nextONU=(nextONU+1)%onutbl->getTableSize();
	// Change it to force DBA to run...
	pollerRunning = false;
	DoUpstreamDBA();

}

void OLTQPerLLiDBase_P::DoUpstreamDBA(){
	// Check we are not running...
	if (pollerRunning) return;

	if (onutbl->getTableSize()==0){
		EV<<"No ONUs registered... returning"<<endl;
		pollerRunning=false;
		return;
	}

	pollerRunning=true;

	// Check that no ONU has been removed (not happening usually)
	if (nextONU>=onutbl->getTableSize())
		nextONU=0;
	EV<< "GATE for ONU idx: "<<nextONU<<endl;

	// Set the allocation for this ONU
	ONUTableEntry * en = onutbl->getEntry(nextONU);

	CommitedTime ct;
	// Convert length from ms to ns16
	ct.length=(fixedWin*pow(10,6))/16;
	// Calc the start time (update expected time 1st)
	if (expTxTime==0 || expTxTime<simTime()) expTxTime=simTime();
	// guard time 5us (Kramer used it with T_max=2ms in "IPACT: A Dynamic Protocol for an
	// Ethernet PON (EPON)")
	simtime_t guard = 0.000005;
	// Start...
	ct.start = MPCPTools::simTimeToNS16((expTxTime+guard).raw());
	EV << "GATE MESSAGE CREATED: start="<<ct.start<<", len="<<ct.length<<endl;

	en->setComTime(ct);

	// Now, schedule the GATE message (TODO: RTT here currently
	// we under-utilize the channel)
	simtime_t gateTime = expTxTime;
	EV << "  Scheduled at expTime="<<expTxTime<<endl;
	scheduleAt(gateTime,sendGateMsg);

	// Now change/update the next expected finish time
	expTxTime = expTxTime+guard+(fixedWin/1000);
	EV << "  Next GATE at expTime="<<expTxTime<<endl;

	// Leave the nextONU to be pointing to the entry we send the
	// GATE to... it will be updated later.
}
