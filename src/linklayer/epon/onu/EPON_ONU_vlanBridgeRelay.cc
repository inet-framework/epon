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

#include "EPON_ONU_vlanBridgeRelay.h"

Define_Module(EPON_ONU_vlanBridgeRelay);

void EPON_ONU_vlanBridgeRelay::initialize(){
	EPON_ONU_relayDefault::initialize();
}


void EPON_ONU_vlanBridgeRelay::handleFromLAN(EtherFrame *frame){
	updateTableFromFrame(frame);

	mac_llid ml;

	ml.mac = frame->getDest();
	// Check table ... if port is the same as incoming drop
	std::string port = getPortForAddress(ml);
	if (port == "ethOut") {
		delete frame;
		return;
	}

	// Get the vlan info
	uint16_t vlan=0;
	EthernetDot1QFrame * frame_d1q = dynamic_cast<EthernetDot1QFrame *>(frame);
	if (frame_d1q) vlan = frame_d1q->getVlanID();


	/*
	 *  NOTE: The following code works... but is logically wrong.
	 *  It is bases on the EtherSwitch code... but we don't have
	 *  multiple Ethernet (copper) interfaces yet (Increases complexity)
	 *
	 *  On extended versions you could only bridge vlan <-> llid.
	 *  (vlan zero (0) -> BC)
	 */

	// No vlan -> Default
	if (vlan == 0){
		EV << "ONU_vlan Relay: No vlan info"<<endl;
	}else{
		uint16_t llid_tmp = findLLiDForVlan(vlan);
		EV << "ONU_vlan Relay: Vlan info found, ID: "<<(int)llid_tmp<<endl;
		if (llid_tmp == 0 ) llid_tmp = LLID_EPON_BC;
		frame->setControlInfo(new EPON_LLidCtrlInfo(llid_tmp) );
	}

	send(frame, "toPONout");
	return;

}

uint16_t EPON_ONU_vlanBridgeRelay::findLLiDForVlan(uint16_t vlan){
	if (!serviceList) return LLID_EPON_BC;

	int index=-1;
	for (uint8_t i=0; i<serviceList->size(); i++){
		if (serviceList->at(i).vlan == vlan){
			index=i;
			break;
		}
	}

	if (index !=-1 && index<(int)llids.size())
		return llids[index];

	return LLID_EPON_BC;
}





