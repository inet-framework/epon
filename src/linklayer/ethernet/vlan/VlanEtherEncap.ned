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

package epon.linklayer.ethernet.vlan;

//
// TODO auto-generated module
//
simple VlanEtherEncap
{
    parameters:
        bool useSNAP = default(false);  // create EtherFrameWithSNAP frames instead of EthernetIIFrame
        @display("i=block/square");
         // Space separated vlan list (to be registered)
        string vlans = default("");
        double vlan_txrate @unit("bps") = default(100Mbps); 
    gates:
        input upperLayerIn;   // higher layer
        output upperLayerOut; // higher layer
        input lowerLayerIn;   // to Ethernet MAC
        output lowerLayerOut; // to Ethernet MAC
}
