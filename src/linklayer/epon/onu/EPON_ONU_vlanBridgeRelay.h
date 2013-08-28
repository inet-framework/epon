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

#ifndef __EPON_ONU_VLANBRIDGERELAY_H__
#define __EPON_ONU_VLANBRIDGERELAY_H__

#include <omnetpp.h>
#include <map>
#include "EtherFrame_m.h"
#include "VlanMACRelayUnitBase.h"
#include "EPON_ONU_relayDefault.h"
#include "EPON_CtrlInfo.h"
#include "EPON_messages_m.h"
#include "ServiceConfig.h"
#include "VlanEtherFrame_m.h"

/**
 * TODO - Generated class
 */
class EPON_ONU_vlanBridgeRelay : public EPON_ONU_relayDefault
{
  protected:
    virtual void initialize();

    virtual void handleFromLAN(EtherFrame *frame);

    virtual uint16_t findLLiDForVlan(uint16_t vlan);
};

#endif
