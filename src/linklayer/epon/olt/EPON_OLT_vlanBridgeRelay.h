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

#ifndef __EPON_OLT_VLANBRIDGERELAY_H__
#define __EPON_OLT_VLANBRIDGERELAY_H__

#include <omnetpp.h>
#include <map>
#include <vector>
#include "EtherFrame_m.h"
#include "VlanMACRelayUnitBase.h"
#include "EPON_OLT_relayDefault.h"
#include "EPON_CtrlInfo.h"
#include "EPON_messages_m.h"
#include "ServiceConfig.h"
#include "VlanEtherFrame_m.h"

/**
 * TODO - Generated class
 */
class EPON_OLT_vlanBridgeRelay : public EPON_OLT_relayDefault
{
  protected:
    virtual void initialize();

    virtual void handleFromLAN(EtherFrame *frame);

    /**
     * Search the service module and see the position of
     * specific vlan.
     */
    virtual int findVlanServiceIndex(uint16_t vlan);
    /**
	 * Search the ONU Table and return the llids in
	 * position "index".
	 */
    virtual std::vector<uint16_t>  findLLiDsForServiceIndex(int index);
};

#endif
