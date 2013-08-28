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

#include "EPON_ONU_relayDefault.h"

Define_Module(EPON_ONU_relayDefault);

void EPON_ONU_relayDefault::initialize()
{
	VlanMACRelayUnitBase::initialize();

	if (dynamic_cast<ServiceConfig *>( findModuleUp("serviceConfig")) != NULL){
		serviceList = &(dynamic_cast<ServiceConfig *>( findModuleUp("serviceConfig"))->srvs);
	}

	numProcessedFrames = numDroppedFrames = 0;
	WATCH(numProcessedFrames);
	WATCH(numDroppedFrames);
	WATCH_VECTOR(llids);
}

void EPON_ONU_relayDefault::handleMessage(cMessage *msg)
{

	// Self Message
	if (msg->isSelfMessage())
	{
		EV << "Self-message " << msg << " received\n";
		error("Unknown self message received!");
		return;
	}

	// Network Message
	cGate *ingate = msg->getArrivalGate();
	EV << "Frame " << msg << " arrived on port " << ingate->getName() << "...\n";

	EtherFrame *frame = check_and_cast<EtherFrame *>(msg);

	if (ingate->getId() ==  gate( "toPONin")->getId()){
		handleFromPON(frame);
	}
	else if (ingate->getId() ==  gate( "ethIn")->getId()){
		handleFromLAN(frame);
	}
	else{
		EV << "Message received in UNKNOWN PORT\n";
		return;
	}

}

void EPON_ONU_relayDefault::handleFromPON(EtherFrame *frame){
	mac_llid ml;
	ml.mac = frame->getSrc();
	ml.llid = -1;

	//Check frame preamble (dist between ONUs ONLY)
	EPON_LLidCtrlInfo * nfo = dynamic_cast<EPON_LLidCtrlInfo *>(frame->removeControlInfo());
	if (nfo!=NULL){
		if (nfo->llid != LLID_EPON_BC) ml.llid = nfo->llid;
	}
	updateTableWithAddress(ml, "toPONout");

	// If it is a register update the LLIDs table
	MPCPRegister * reg = dynamic_cast<MPCPRegister *>(frame);
	if (reg){
		//EV<< "Assigned LLIDs:" <<LLID_num<<endl;
		for (uint8_t i=0; i<reg->getLLIDsArraySize(); i++){
			// 0xFFF = 4095 intrand -> [0-4095)
			llids.push_back(reg->getLLIDs(i));
		}

		// Do not forward further
		delete frame;
		return;
	}

	// We do not need to scan the table since we still
	// have no vlans. Each ONU is considered to be a different
	// net (subnet) so the frame must go directly to wire.
	send(frame, "ethOut");
}

void EPON_ONU_relayDefault::handleFromLAN(EtherFrame *frame){
	updateTableFromFrame(frame);

	mac_llid ml;

	ml.mac = frame->getDest();
	// Check table ... if port is the same as incoming drop
	std::string port = getPortForAddress(ml);
	if (port == "ethOut") {
		delete frame;
		return;
	}


	// Do MAPINGS HERE (vlan or anything other to LLID)
	// currently use the default

	/*
	 *  NOTE: The following code works... but is logically wrong.
	 *  It is based on the EtherSwitch code... but we don't have
	 *  multiple Ethernet (copper) interfaces yet (Increases complexity)
	 *
	 *  On extended versions you could only bridge vlan <-> llid.
	 *  (vlan zero (0) -> Default)
	 */


	// If we don't have any llid, drop
	if (llids.size() == 0 ){
		EV << "No LLiD Assigned, droping frame"<<frame<<endl;
		delete frame;
		return;
	}
	// If we have 1 llid, sent through there
	else if (llids.size() == 1 ){
		frame->setControlInfo(new EPON_LLidCtrlInfo(llids[0]) );
		send(frame, "toPONout");
		return;
	}
	// We have more than 1 assigned LLiDs
	else {
		std::vector<port_llid> vec = getLLIDsForAddress(frame->getDest());

		if (vec.size()!=0){
			// Send to the known llids only
			for (uint32_t i = 0; i<vec.size(); i++){
				EtherFrame *frame_tmp = frame->dup();

				// Get this llid and this port, duplicate and send
				port_llid tmp_pl = (port_llid)vec[i];
				// If we don't know ... lets BC
				if (tmp_pl.llid == -1) tmp_pl.llid = LLID_EPON_BC;
				frame_tmp->setControlInfo(new EPON_LLidCtrlInfo(tmp_pl.llid) );
				send(frame_tmp, tmp_pl.port.c_str());

				// If BC, we break;
				if (tmp_pl.llid == LLID_EPON_BC) break;
			}
		}else{
			// DO NOT Send to all the llids assigned
			//		for (uint32_t i=0; i<llids.size(); i++){
			//			EtherFrame *frame_tmp = frame->dup();
			//
			//			// Get this llid and this port, duplicate and send
			//			frame_tmp->setControlInfo(new EPON_LLidCtrlInfo(llids[i]) );
			//			send(frame_tmp, "toPONout");
			//		}
			// INSTEAD BROADCAST IT...
			// ENABLE THE ABOVE TO TEST PON (Generates X-times more traffic)

			/**
			 *  6-Feb-2011:
			 *  Do NOT BC... send to the default LLiD
			 */

			EtherFrame *frame_tmp = frame->dup();

			// Get this llid and this port, duplicate and send
			//frame_tmp->setControlInfo(new EPON_LLidCtrlInfo(LLID_EPON_BC) );
			send(frame_tmp, "toPONout");


			// TODO: Normally it would be sent only to the
			// LLID bridged with the vlan that the source host
			// belongs. OR alternatively traffic can be differentiated
			// on the network layer

		}
	}


	// delete the original frame
	delete frame;


}

cModule * EPON_ONU_relayDefault::findModuleUp(const char * name){
	cModule *mod = NULL;
	for (cModule *curmod=this; !mod && curmod; curmod=curmod->getParentModule())
	     mod = curmod->getSubmodule(name);
	return mod;
}

