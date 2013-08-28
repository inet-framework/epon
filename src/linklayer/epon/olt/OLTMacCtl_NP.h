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

#ifndef __OLTMacCtl_NP_H__
#define __OLTMacCtl_NP_H__

#include <omnetpp.h>
#include "EtherFrame_m.h"
#include "MPCPTools.h"			// For Clock
#include "EPON_messages_m.h"	// For adding timeStamps
#include "MPCP_codes.h"			// To Check EtherType
#include "IPassiveQueue.h"




#define TXENDMSG	100
#define WAKEUPMSG	200


// States
#define TX_SENDING			1
#define TX_IDLE				2

/**
 * OLTMacCtl_NP class controls the below MAC layer transmission times.
 * For the OLT is simpler since we continuously transmit. This class
 * has a pointer to the EPON_Q_mgmt module and it uses it as a simple
 * queue in order to get frames. Finally note that missing mac addresses
 * and LLIDs (from frames) are filled in here, cause the lower layer
 * expects them.
 */
class OLTMacCtl_NP : public cSimpleModule
{
  protected:
	// Global Clock
	uint32_t clock_reg;

	// Queues
	IPassiveQueue * queue_mod;		// Upper Layer Q
	cQueue tmp_queue;

	// Transmission states
	int transmitState;
	cMessage * txEnd;

	// Statistics
	int numFramesFromHL;
	int numFramesFromLL;


  public:
	virtual ~OLTMacCtl_NP();

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    virtual void processFrameFromHigherLayer(cMessage *msg);
    virtual void processFrameFromMAC(cMessage *msg);

    virtual void clockSync();

    // Handle TX times
    virtual void handleTxEnd();
    virtual void doTransmit(cMessage * msg);

    // TOOLS TODO: add to a different class
    virtual cModule * getNeighbourOnGate(const char * gate);
};

#endif
