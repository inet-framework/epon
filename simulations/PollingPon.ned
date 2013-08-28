package epon.simulations;
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

import epon.linklayer.epon.olt.EPON_OLT;
import epon.linklayer.epon.onu.EPON_ONU;
import epon.linklayer.epon.splitter.PON_Splitter;
import epon.linklayer.epon.common.ServiceConfig;
import epon.networklayer.autorouting.vlan.VlanNetConfig;
import epon.nodes.vlan.VlanStandardHost;
import inet.nodes.ethernet.Eth1G;
import ned.DatarateChannel;


network PollingPon
{
    @display("bgl=3");
    types:
        channel GFX extends DatarateChannel
        {
            datarate = 1Gbps;
        }
    submodules:
        pon_Splitter: PON_Splitter {
            @display("p=230,142");
        }
        epon_onu: EPON_ONU {
            @display("p=432,66");
        }
        host1: VlanStandardHost {
            @display("p=538,66");
        }
        epon_onu3: EPON_ONU {
            @display("p=432,185");
        }
        epon_onu2: EPON_ONU {
            @display("p=432,123");
        }
        host3: VlanStandardHost {
            @display("p=538,130");
        }
        host4: VlanStandardHost {
            @display("p=538,192");
        }
        Server: VlanStandardHost {
            @display("p=86,72");
        }
        epon_olt: EPON_OLT {
            @display("p=141,142");
        }
        serviceConfig: ServiceConfig {
            @display("p=221,34");
        }
        vlanNetConfig: VlanNetConfig {
            @display("p=329,33");
        }
    connections:

        // to splitter
        epon_olt.epon <--> GFX <--> pon_Splitter.portu;
        pon_Splitter.portg++ <--> GFX <--> epon_onu.epon;
        pon_Splitter.portg++ <--> GFX <--> epon_onu2.epon;
        pon_Splitter.portg++ <--> GFX <--> epon_onu3.epon;

        // Hosts
        epon_onu.ethg <--> Eth1G <--> host1.ethg++;
        epon_onu2.ethg <--> Eth1G <--> host3.ethg++;
        epon_onu3.ethg <--> Eth1G <--> host4.ethg++;
        epon_olt.ethg <--> Eth1G <--> Server.ethg++;

}
