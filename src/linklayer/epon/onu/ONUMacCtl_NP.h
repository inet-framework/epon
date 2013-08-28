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

#ifndef __ONUMACCTL_H__
#define __ONUMACCTL_H__

#include <omnetpp.h>
#include <string.h>
#include "EtherFrame_m.h"
#include "MPCP_codes.h"
#include "EPON_messages_m.h"
#include "MACAddress.h"
#include "MPCPTools.h"
#include "EPON_mac.h"
#include "IPassiveQueue.h"
#include "ONUMacCtlBase.h"

// Self-message kind values
#define STOPTXMSG			101
#define SFTSTRMSG			103






/**
 * ONUMacCtl class controls the below MAC layer transmission times. This
 * version is fairly more complicated from the OLT one because we have
 * transmit in specific times. The module can come to IDLE or SLEEP states
 * which means that the clock and the start register are left back. (The
 * clock is updated on each message reception).
 *
 * Also this class has a pointer to the EPON_Q_mgmt module and it uses it
 * as a simple queue in order to get frames. Finally note that missing mac
 * addresses and LLIDs (from frames) are filled in here, cause the lower layer
 * expects them.
 */
class ONUMacCtl_NP : public ONUMacCtlBase
{
  protected:
	// self messages
	cMessage *stopTxMsg;

  public:
	ONUMacCtl_NP();
	virtual ~ONUMacCtl_NP();

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    virtual void processFrameFromHigherLayer(cMessage *msg);
    virtual void processMPCP(EthernetIIFrame *frame );

    // Scheduling
    virtual void scheduleStartTxPeriod();
    virtual void scheduleStopTxPeriod();

    // Tx handling
    virtual void startTxOnPON();
    virtual void handleStopTxPeriod();


    // Clock Functions
    /**
     * Update the clock. Note that the clock can be
     * reseted back to 0 (zero) cause it is define
     * by MPCP as 32bit register (of 16 nanoseconds
     * granularity). This is also handled here.
     */
    virtual void clockSync();
    /**
     * This method shifts the start register for one
     * SuperSlot. SuperSlot is defined as the summation
     * of all the slot lengths.
     */
    virtual void shiftStart1Slot();
    /**
     * This method handles the start register when the
     * module wakes up from SLEEP state and it has
     * lost many SuperSlots.
     */
    virtual void shiftStartXSlots(uint32_t X);


  private:
    virtual void goToIDLE();
    virtual void goToSLEEP();

};

#endif
