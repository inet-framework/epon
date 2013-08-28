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

import inet.linklayer.ethernet.EthernetInterface;
import epon.linklayer.ethernet.vlan.VlanEtherMAC;

package epon.linklayer.epon.onu;
//
// TODO auto-generated module
//
module EPON_ONU
{
    parameters:
        @node();
        @display("i=device/modem");
        string relayUnitType = default("EPON_ONU_relayDefault");
    gates:
        inout ethg;
        inout epon;

    submodules:
        onu_port: ONU_Port {
            @display("p=78,134");
        }
        epon_ONU_relay: <relayUnitType> like EPON_ONU_relay {
            @display("p=147,56");
        }
        mac: VlanEtherMAC {
            @display("p=212,137");
            promiscuous = true;
        }

    connections allowunconnected:
        mac.upperLayerIn <-- epon_ONU_relay.ethOut;
        mac.upperLayerOut --> epon_ONU_relay.ethIn;
        mac.phys <--> ethg;

        epon_ONU_relay.toPONout --> onu_port.netwIn;
        epon_ONU_relay.toPONin <-- onu_port.netwOut;

        onu_port.phys <--> epon;
}
