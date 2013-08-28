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

import inet.linklayer.ethernet.EtherMAC;
import epon.linklayer.ethernet.vlan.VlanEtherMAC;
import epon.linklayer.epon.common.*;

package epon.linklayer.epon.olt;

//
// TODO auto-generated module
//
module EPON_OLT
{
    parameters:
        @node();
        @display("i=device/router");
        string relayUnitType = default("EPON_OLT_relayDefault");
    gates:
        inout ethg;
        inout epon;
    submodules:
        epon_OLT_relay: <relayUnitType> like EPON_OLT_relay {
            @display("p=106,105");
        }
        mac: VlanEtherMAC {
            @display("p=47,185");
            promiscuous = true;
        }
        olt_if: OLT_IF {
            @display("p=165,185");
        }
        onuTable: ONUTable {
            @display("p=39,26");
        }
    connections:

        mac.upperLayerIn <-- epon_OLT_relay.ethOut;
        mac.upperLayerOut --> epon_OLT_relay.ethIn;
        mac.phys <--> ethg;

        epon_OLT_relay.toPONout --> olt_if.netwIn;
        olt_if.netwOut --> epon_OLT_relay.toPONin;

        olt_if.phys <--> epon;
}
