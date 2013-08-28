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

#include "VlanNetConfig.h"

Define_Module(VlanNetConfig);

using namespace std;

void VlanNetConfig::initialize(int stage)
{
	if (stage==0){
	 // assign IP addresses
	 networkAddress = IPv4Address("10.0.0.0").getInt();
	 netmask = IPv4Address("255.255.255.0").getInt();
	}

	FlatNetworkConfigurator::initialize(stage);

	if (stage==numInitStages()-1){
		// Initialize Stacks
		initializeNextAvailableIPz();

		WATCH_MAP(vlanInfo);
	}
}

uint16_t VlanNetConfig::getVlanFromIfName(std::string ifname){
	uint subif = ifname.find('.');
	uint16_t vlan=0;
	if (subif!=string::npos){
		vlan = atoi(ifname.substr(subif+1).c_str());
	}

	return vlan;
}
uint32_t VlanNetConfig::getNetAddrForVlan(uint16_t vlan){
	uint32_t vl = vlan&0x00FFL;
	uint32_t vh = vlan>>8;
	vh=vh<<8*2;
	vl=vl<<8;
	return (networkAddress|vh|vl);
}

void VlanNetConfig::assignAddresses(cTopology& topo, NodeInfoVector& nodeInfo){

    int numIPNodes = 0;
    for (int i=0; i<topo.getNumNodes(); i++)
    {
        // skip bus types
        if (!nodeInfo[i].isIPNode)
            continue;

        uint32 addr = networkAddress | uint32(++numIPNodes);
        if (numIPNodes==255) error("To many nodes, we are out of IPs");
        nodeInfo[i].address.set(addr);

        // find interface table and assign address to all (non-loopback) interfaces
        IInterfaceTable *ift = nodeInfo[i].ift;
        for (int k=0; k<ift->getNumInterfaces(); k++)
        {
            InterfaceEntry *ie = ift->getInterface(k);
            if (ie->isLoopback()) continue;

			uint16_t vlan = getVlanFromIfName( ie->getFullName() );


			VlanIPInfo::iterator it = vlanInfo.find(vlan);
			if (it == vlanInfo.end()){
				vlanInfo[vlan].numHosts = 1;
			}else{
				if (vlanInfo[vlan].numHosts == 254)
					error("netmask too large, not enough addresses for all nodes "
							"for vlan %d", vlan);
				vlanInfo[vlan].numHosts++;
			}

			addr=getNetAddrForVlan(vlan)|vlanInfo[vlan].numHosts;

			ie->ipv4Data()->setIPAddress(IPv4Address(addr));
			ie->ipv4Data()->setNetmask(IPv4Address(netmask));
			// full address must match for local delivery

        }
    }
}

void VlanNetConfig::initializeNextAvailableIPz(){
	VlanIPInfo::iterator it = vlanInfo.begin();
	for (;it!=vlanInfo.end(); ++it){
		// It means we do not have more ips... but we actually don't
		// need more at this stage. The error occurs on host addition
		if (it->second.numHosts == 254) continue;

		uint32_t nip = getNetAddrForVlan(it->first)|(it->second.numHosts+1);
		EV << "Next Available IP for vlan "<<it->first<<" is: "<<IPv4Address(nip)<<endl;
		it->second.nextAvailIP.push(nip);
	}
}

void VlanNetConfig::hostRemoved(cModule * h){
	Enter_Method_Silent("hostRemoved");
	IInterfaceTable * ift=IPvXAddressResolver().interfaceTableOf(h);
	for (int k=0; k<ift->getNumInterfaces(); k++)
	{
		InterfaceEntry *ie = ift->getInterface(k);
		if (ie->isLoopback()) continue;

		uint16_t vlan = getVlanFromIfName( ie->getFullName() );
		IPv4Address freedIP = ie->ipv4Data()->getIPAddress();
		vlanInfo[vlan].nextAvailIP.push(freedIP.getInt());
		vlanInfo[vlan].numHosts--;
	}
}

void VlanNetConfig::hostAdded(cModule * h){
	Enter_Method_Silent("hostAdded");
	IInterfaceTable * ift=IPvXAddressResolver().interfaceTableOf(h);
	for (int k=0; k<ift->getNumInterfaces(); k++)
	{
		InterfaceEntry *ie = ift->getInterface(k);
		if (ie->isLoopback()) continue;

		uint16_t vlan = getVlanFromIfName( ie->getFullName() );

		// Check that there is at least 1
		if (vlanInfo[vlan].nextAvailIP.size() == 0)
			error("No more IPs for vlan %d", vlan);

		// Assign it
		uint32_t nip = vlanInfo[vlan].nextAvailIP.top();
		vlanInfo[vlan].nextAvailIP.pop();
		ie->ipv4Data()->setIPAddress(IPv4Address(nip));
		ie->ipv4Data()->setNetmask(IPv4Address(netmask));
		vlanInfo[vlan].numHosts++;

		// Now, if the stack is empty it means that we pulled the
		// IP with the maximum number (last one assigned on initialization).
		// We have to push the next ip (++) into the stack (if available)
		if (vlanInfo[vlan].nextAvailIP.size() == 0){
			if (vlanInfo[vlan].numHosts == 254) continue; // Skip if no more IPs

			nip = getNetAddrForVlan(vlan)|(vlanInfo[vlan].numHosts+1);
			vlanInfo[vlan].nextAvailIP.push(nip);
		}
	}
}

void VlanNetConfig::setDisplayString(cTopology& topo, NodeInfoVector& nodeInfo)
{
    int numIPNodes = 0;
    for (int i=0; i<topo.getNumNodes(); i++)
        if (nodeInfo[i].isIPNode)
            numIPNodes++;

    // update display string
    char buf[100];
    sprintf(buf, "%d IP nodes\nin %d vlans\n(%d non-IP nodes)", numIPNodes, (int)vlanInfo.size(),topo.getNumNodes()-numIPNodes);
    getDisplayString().setTagArg("t",0,buf);
}


