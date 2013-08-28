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

#ifndef __VLANNETCONFIG_H__
#define __VLANNETCONFIG_H__

#include <map>
#include <stack>
#include <string>
#include <stdlib.h>
#include <omnetpp.h>
#include <iostream>
#include "FlatNetworkConfigurator.h"
#include "InterfaceEntry.h"
#include "InterfaceTable.h"
#include "IPv4InterfaceData.h"
#include "IPvXAddressResolver.h"

/**
 * Struct to keep information about each VLAN
 */
typedef struct _vlan_info_entry{
	uint8_t numHosts;
	std::stack<uint32_t> nextAvailIP;

	friend std::ostream & operator<<(std::ostream & out, const struct _vlan_info_entry &vie){
		IPv4Address ip(vie.nextAvailIP.top());
		out<<"#hosts="<<(int)vie.numHosts<<" NextIP="<<ip;
		return out;
	}
} VlanInfoEntry;

typedef std::map<uint16_t, VlanInfoEntry> VlanIPInfo;

/**
 * A NetworkConfigurator to handle vlans on initialization.
 * Not many parameters available... What it actually does is
 * to take the 10.x.x.0/24 subnet for each vlan. 10.0.0.0/24
 * is always going to be the real device. the x.x is replaced
 * by the vlan ID. DrawBack: This means that each vlan is
 * allowed 254 machines(hosts+network devices).
 */
class VlanNetConfig : public FlatNetworkConfigurator
{
  public:
	uint16_t getVlanFromIfName(std::string ifname);
	uint32_t getNetAddrForVlan(uint16_t vlan);

	/**
	 * Handle Hosts that are dynamically removed
	 */
	virtual void hostRemoved(cModule * h);
	/**
	 * Handle Hosts that are dynamically added
	 */
	virtual void hostAdded(cModule * h);

  protected:
	uint32 networkAddress;
	uint32 netmask;


	VlanIPInfo vlanInfo;
    virtual void initialize(int stage);

    virtual void assignAddresses(cTopology& topo, NodeInfoVector& nodeInfo);
    virtual void setDisplayString(cTopology& topo, NodeInfoVector& nodeInfo);
    virtual void initializeNextAvailableIPz();





};

#endif
