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

package epon.linklayer.epon.common;

//
// A module to keep track of multi-service
// scenarios. Here we register the services
// available, the priority of each service and
// any other needed information. Since we are working 
// with VLANs - we map traffic from various 
// technologies into different vlans - we are
// going to register this too. Finally for the access
// network we have a per client/flow configuration.
// This is the MSR (Maximum Sustained Rate) used mainly
// in WiMax modules.
//
simple ServiceConfig
{
    parameters:
        string services = default("DATA");		// Service Names (whatever you like here)
        string srvPrior = default("1.0");		// Service priority. Total sum must be 1
        string vlanMap = default("0"); 			// Vlan map
        string msrPerFlow = default("1000000"); // MSR per flow, default 1Mbps (-1 = Unlimited)
        string mrrPerFlow = default("1000000"); // MRR per flow, default 1Mbps (-1 = Unlimited)
        
        @display("i=block/classifier");
}
