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

#ifndef __PON_ONUMACCTL_P_H_
#define __PON_ONUMACCTL_P_H_

#include <omnetpp.h>
#include "ONUMacCtlBase.h"
#include "ONUQPerLLiDBase.h"

#define SEND_REPORT 201
/**
 * TODO - Generated class
 */
class ONUMacCtl_P : public ONUMacCtlBase
{
  public:
	ONUMacCtl_P();
	virtual ~ONUMacCtl_P();

  protected:
	// queue as ONUQPerLLiD
	ONUQPerLLiDBase * qpl;

	// Schedule the REPORT at the end of the timeslot
	cMessage * sendReportMsg;

    virtual void initialize();
	virtual void startTxOnPON();

	// Message handlers
    virtual void handleMessage(cMessage *msg);
    virtual void processFrameFromHigherLayer(cMessage *msg);
    //virtual void processFrameFromMAC(cMessage *msg);
    virtual void processMPCP(EthernetIIFrame *frame );

    // Handle Clock every time you get a message
    virtual void clockSync();

    // Schedule startTxMsg based on the last MPCP GATE
    virtual void scheduleStartTxMsgFromRegister();

    // Each time we get a GATE we also schedule our report
    virtual void sendReport();
    virtual void scheduleReport();


};

#endif
