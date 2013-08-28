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

#include "ONUMacCtlBase.h"

ONUMacCtlBase::ONUMacCtlBase() {
	// TODO Auto-generated constructor stub

}

ONUMacCtlBase::~ONUMacCtlBase() {
	// TODO Auto-generated destructor stub
}

void ONUMacCtlBase::initialize()
{
	clock_reg=0;
	start_reg=0;
	len_reg=0;
	numGates = 0;

	// Link to MAC layer
	emac = dynamic_cast<EPON_mac *>(getNeighbourOnGate("lowerLayerOut"));
	if (!emac)
		opp_error("No MAC layer found connected under ONU MAC Control Layer");


	transmitState = TX_OFF;
	txrate=GIGABIT_ETHERNET_TXRATE;



	startTxMsg = new cMessage("startTxMsg", STARTTXMSG);


	// Init Stats
	numFramesFromHL=0;
	numFramesFromLL=0;
	numFramesFromHLDropped=0;
	fragmentedTime=0;

    WATCH(numFramesFromHL);
    WATCH(numFramesFromLL);
    WATCH(numFramesFromHLDropped);
    WATCH(fragmentedTime);

    WATCH(clock_reg);
    WATCH(start_reg);
    WATCH(len_reg);
    WATCH(transmitState);


	// MPCP
    transmitState = TX_INIT;


    // Initialize the Q mgmt module
	queue_mod = dynamic_cast<IPassiveQueue *>(getNeighbourOnGate("upperLayerOut"));
	if (!queue_mod)
		opp_error("ONUMacCtlBase: An IPassiveQueue is needed above mac control");

}


void ONUMacCtlBase::processFrameFromMAC(cMessage *msg){


	EV << "ONUMacCtlBase: Incoming message from PON\n";
	EthernetIIFrame * frame = dynamic_cast<EthernetIIFrame *>(msg);


	if (frame && frame->getEtherType() == MPCP_TYPE){

		// Check that the frame is for us...
		if (frame->getDest() != emac->getMACAddress() && !frame->getDest().isBroadcast()){
			EV << "MPCP not for us... dropping\n";
			delete frame;
			return;
		}

		processMPCP(frame );
	}


	send(msg,"upperLayerOut");
	numFramesFromLL++;
}


std::string ONUMacCtlBase::getStateStr(){
	switch (transmitState){
	case TX_ON: return "TX_ON";
	case TX_OFF: return "TX_OFF";
	case TX_INIT: return "TX_INIT";
	case TX_IDLE: return "TX_IDLE";
	case TX_SLEEP: return "TX_SLEEP";
	default: return "UNKNOWN!";
	}
}

void ONUMacCtlBase::dumpReg(){
	EV << "Sim(ns16): "<<MPCPTools::simTimeToNS16()<<endl;
	EV << "Clock: "<<clock_reg<<endl;
	EV << "Start: "<<start_reg<<endl;
	EV << "Length: "<<len_reg<<endl;
}



void ONUMacCtlBase::finish ()
{
    simtime_t t = simTime();
    recordScalar("simulated time", t);
    recordScalar("messages handled", numFramesFromHL+numFramesFromLL);
    recordScalar("Dropped Frames From HL", numFramesFromHLDropped);
    double fragBits = fragmentedTime.dbl()/(1/GIGABIT_ETHERNET_TXRATE);
    recordScalar("FragmentedBits", fragBits);
    if (t>0) {
        recordScalar("frames/sec", (numFramesFromHL+numFramesFromLL)/t);
        recordScalar("drops/sec", numFramesFromHLDropped/t);
        recordScalar("FragmentedBits/sec", fragBits/t);
    }
}

cModule * ONUMacCtlBase::getNeighbourOnGate(const char * g){
	return gate(g)->getNextGate()->getOwnerModule();
}

