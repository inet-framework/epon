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

#include "PON_Splitter.h"

Define_Module(PON_Splitter);

void PON_Splitter::initialize()
{
    numMessages = 0;
    col_downstream = 0;
    col_upstream = 0;
    previousMsg = 0;
    WATCH(numMessages);
    WATCH(col_downstream);
    WATCH(col_upstream);

    ports = gateSize("portg");

    haltOn = par("haltOn");

    // Put the gates on instant transmission
    // (or suffer from collisions)
    gate("portu$i")->setDeliverOnReceptionStart(true);
    for (int i=0; i<ports; i++)
    	gate("portg$i",i)->setDeliverOnReceptionStart(true);

    // Display analogy
    char tmp[20];
    sprintf(tmp,"1:%d",ports);
    getDisplayString().setTagArg("t",0,tmp);
}

void PON_Splitter::handleMessage(cMessage *msg)
{
	// Handle frame sent down from the network entity: send out on every other port
	cGate *ingate = msg->getArrivalGate();
	EV << "Frame " << msg << " arrived on port " << ingate->getName() << "...\n";


	if (ingate->getId() ==  gate( "portu$i")->getId()){
		EV << "sending to clients\n";
		// DownStream
		if (ports==0)
		{
			delete msg;
			return;
		}

		for (int i=0; i<ports; i++)
		{
			// if it is the last port to send, do NOT duplicate
			bool isLast = (ingate->getIndex() ==ports-1) ? (i==ports-2) : (i==ports-1);
			// Check if we have 1 port to ONU...
			if (ports==1) isLast=true;
			cMessage *msg2 = isLast ? msg : (cMessage*) msg->dup();

			if (gate("portg$o",i)->getTransmissionChannel()->isBusy() ) {
				EV << "PON_Splitter: DOWNSTREAM COLLISION (#: "<<col_downstream<<")" << endl;
				EV << "PON_Splitter: Tx Finish time: " <<
						gate("portg$o",i)->getTransmissionChannel()->getTransmissionFinishTime()
						<<endl;
				EV << "PON_Splitter: Diff: " <<
						gate("portg$o",i)->getTransmissionChannel()->getTransmissionFinishTime() -simTime()
						<<endl;
				col_downstream++;
				delete msg2;
				delete msg;
				return;
			} else {
				send(msg2,"portg$o",i);
			}
		}
	}
	else{
		EV << "sending to UpLink\n";
		if (gate("portu$o")->getTransmissionChannel()->isBusy() ) {

			printUpStreamDebug(msg);
			col_upstream++;

			if (col_upstream >= haltOn)
				error("Simulation HALTED DUE TO COLLISIONS ON THE SPLITTER... "
						"To disable this error change the 'haltOn' parameter...");
			delete msg;
			// Notify if this could be a registration message
			if (simTime()<1)
				opp_error("Registration message collision... \n - You can increase the registration interval\n - RNG may also fail");
			return;
		}else
			send(msg,"portu$o");
	}


	// Here only if message sent
	previousMsg=msg;
	numMessages++;

}

void PON_Splitter::printUpStreamDebug(cMessage * msg){
	EV << "PON_Splitter: UPSTREAM COLLISION (#: "<<col_upstream<<")" << endl;
	EV << "PON_Splitter: Tx Finish time: " <<
			gate("portu$o")->getTransmissionChannel()->getTransmissionFinishTime() <<
			" simTime: "<<simTime()<<endl;
	EV << "PON_Splitter: Diff: " <<
			gate("portu$o")->getTransmissionChannel()->getTransmissionFinishTime() -simTime()<<endl;

	EV << "PON_Splitter: Message Info:" << endl<<
			"\tFirst : "<<previousMsg->getArrivalGate()->getIndex()
			<<" Time: "<<MPCPTools::simTimeToNS16(previousMsg->getSendingTime().raw())<<endl<<
			"\tSecond: "<<msg->getArrivalGate()->getIndex()
			<<" Time: "<<MPCPTools::simTimeToNS16(msg->getSendingTime().raw());
}

void PON_Splitter::finish ()
{
    simtime_t t = simTime();
    recordScalar("simulated time", t);
    recordScalar("messages handled", numMessages);
    recordScalar("downstream collisions", col_downstream);
    recordScalar("upstream collisions", col_upstream);
    if (t>0) {
        recordScalar("messages/sec", numMessages/t);
        recordScalar("downstream collisions/sec", col_downstream/t);
        recordScalar("upstream collisions/sec", col_upstream/t);
    }
}
