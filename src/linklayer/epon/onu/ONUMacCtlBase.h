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

#ifndef ONUMACCTLBASE_H_
#define ONUMACCTLBASE_H_

#include <omnetpp.h>
#include <string.h>
#include "EtherFrame_m.h"
#include "MPCP_codes.h"
#include "EPON_messages_m.h"
#include "MACAddress.h"
#include "MPCPTools.h"
#include "IPassiveQueue.h"
#include "EPON_mac.h"



// Signal Q module to get new frame
#define GETFRAMEMSG			200
#define WAKEUPMSG			201

// Self-message kind values
#define STARTTXMSG          100

// States
/// Laser is ON and transmitting
#define TX_ON				1
/// Laser is OFF - Not our timeslot
#define TX_OFF				2
/// Pre-MPCP state - Only during initialization
#define TX_INIT				3
/// It IS our timeslot but there are not frames to Tx
#define TX_IDLE				4
/// It may or may not be our turn BUT ALL Qs ARE EMPTY (no need to schedule startTx)
#define TX_SLEEP			5


class ONUMacCtlBase : public cSimpleModule{
protected:
	/// Tx Rate TODO: make it a parameter
	uint64_t txrate;
	/// Pointer to the mac layer for some control and use of MAC address
	EPON_mac * emac;
	// Registers are 32bit, 16ns granularity
	uint32_t clock_reg;
	uint32_t start_reg;
	uint32_t len_reg;
	int transmitState;
	int RTT;

	// Dynamic Parameters from OLT
	uint16_t slotLength;
	uint16_t slotNumber;

	// Hold the number of MPCP GATEs
	int numGates;

	// Queue
	IPassiveQueue * queue_mod;		// Upper Layer Q
	cQueue tmp_queue;

	// Statistics
	int numFramesFromHL;
	int numFramesFromLL;
	int numFramesFromHLDropped;
	simtime_t fragmentedTime;

	cMessage *startTxMsg;


public:
	ONUMacCtlBase();
	virtual ~ONUMacCtlBase();

protected:
	virtual void initialize();
	/// Log Scalars
	virtual void finish ();
	virtual void startTxOnPON()=0;

    virtual void processFrameFromMAC(cMessage *msg);
    virtual void processMPCP(EthernetIIFrame *frame) = 0;

	// TOOLS TODO: add to a different class
	virtual cModule * getNeighbourOnGate(const char * gate);

	virtual void dumpReg();
	std::string getStateStr();


};

#endif /* ONUMACCTLBASE_H_ */
