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

#include "EPON_OLT_vlanBridgeRelay.h"

Define_Module(EPON_OLT_vlanBridgeRelay);

void EPON_OLT_vlanBridgeRelay::initialize(){
	EPON_OLT_relayDefault::initialize();
}


void EPON_OLT_vlanBridgeRelay::handleFromLAN(EtherFrame *frame){
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


	// Do MAPINGS HERE (vlan or anything other to LLID)
	// currently use the default

	/*
	 *  NOTE: The following code works... but is logically wrong.
	 *  It is bases on the EtherSwitch code... but we don't have
	 *  multiple Ethernet (copper) interfaces yet (Increases complexity)
	 *
	 *  On extended versions you could only bridge vlan <-> llid.
	 *  (vlan zero (0) -> BC)
	 */

	// No vlan -> BC or NO services defined
	if (vlan == 0 || serviceList == NULL){
		EV << "OLT_vlan Relay: No vlan info"<<endl;
		frame->setControlInfo(new EPON_LLidCtrlInfo(LLID_EPON_BC) );
	}else{

		std::vector<uint16_t> srvLLiDs;
		srvLLiDs = findLLiDsForServiceIndex(
				findVlanServiceIndex(vlan)
				);
		mac_llid tmp_ml;
		tmp_ml.mac = frame->getDest();

		// Scan the MAC-Address table for this ML
		for (int i=0; i<(int)srvLLiDs.size(); i++){
			tmp_ml.llid=srvLLiDs[i];
			if (getPortForAddress(tmp_ml) == "toPONout"){
				EtherFrame * frame_dup = frame->dup();
				frame_dup->setControlInfo(new EPON_LLidCtrlInfo(tmp_ml.llid) );
				send(frame_dup, "toPONout");
			}
		}

		// Delete the original frame and return
		delete frame;
		return;
	}

	send(frame, "toPONout");

}

int EPON_OLT_vlanBridgeRelay::findVlanServiceIndex(uint16_t vlan){
	if (!serviceList) return -1;

	for (uint8_t i=0; i<serviceList->size(); i++){
		if (serviceList->at(i).vlan == vlan)
			return i;
	}

	return -1;
}
std::vector<uint16_t> EPON_OLT_vlanBridgeRelay::findLLiDsForServiceIndex(int index){

	std::vector<uint16_t> res;

	for (int i=0; i<onutbl->getTableSize(); i++){
		// Probably not useful... but...
		if (onutbl->getEntry(i)->getLLIDsNum() < index) continue;

		res.push_back(onutbl->getEntry(i)->getLLID(index));
	}

	return res;
}





