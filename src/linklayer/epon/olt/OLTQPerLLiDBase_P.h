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

#ifndef __PON_OLTQPERLLIDBASE_P_H_
#define __PON_OLTQPERLLIDBASE_P_H_

#include <omnetpp.h>
#include "OLT_QPL_RR.h"

// Self Messages
#define SEND_GATE	200

/**
 * Polling algorithm base. It extends the RR for its downstream queue
 * functionality and the non-polling Base
 */
class OLTQPerLLiDBase_P : public OLT_QPL_RR
{
  public:
	OLTQPerLLiDBase_P();
	virtual ~OLTQPerLLiDBase_P();
  protected:

	// Parameters
	double wMax;
	double fixedWin;


	// Self Messages
	cMessage * sendGateMsg;

	// Status of the poller
	bool pollerRunning;
	// Index of the next ONU to be polled
	int nextONU;
	// Expected TX finish time for the currently transmitting ONU
	simtime_t expTxTime;


    virtual void initialize();
    // Handle only self messages
    virtual void handleMessage(cMessage *msg);

    // DBA

    /// Send a single GATE (unicast) to an ONU
    virtual void sendSingleGate();
    /**
     *  Override to cancel default behavior and do nothing
     *  This is automatically called on ONU registration. On
     *  polling based DBAs we do not need to reschedule all the
     *  ONUs...
     */
    virtual void SendGateUpdates();

    /**
     *  Override the algorithm :)
     *
     *  In this polling example the algorithm is using the
     *  fixed service approach. It will ignore any requests
     *  and it will always allocate fixedWin time to each ONU.
     *
     *  This should be extended to implement new algorithms
     */
    virtual void DoUpstreamDBA();


};

#endif
