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

#include "VlanARP.h"
#include "ARPPacket_m.h"

Define_Module(VlanARP);

void VlanARP::initialize(int stage){
	// Avoid Cleaning Global Cache
	if (stage!=4) return;

	ARP::initialize(stage);
}

void VlanARP::sendPacketToNIC(cMessage *msg, InterfaceEntry *ie, const MACAddress& macAddress)
{
    // add control info with MAC address
    VlanIeee802Ctrl *controlInfo = new VlanIeee802Ctrl();
    controlInfo->setDest(macAddress);
    controlInfo->setIfName(ie->getFullName());
    msg->setControlInfo(controlInfo);
    // Ensure BC MAC
    if (dynamic_cast<ARPPacket *>(msg)){
    	(dynamic_cast<ARPPacket *>(msg))->setDestMACAddress(MACAddress::BROADCAST_ADDRESS);
    }

    // send out
    // send(msg, nicOutBaseGateId + ie->getNetworkLayerGateIndex());
    sendDirect(msg, getParentModule(), "ifOut",
                                  ie->getNetworkLayerGateIndex());
}



