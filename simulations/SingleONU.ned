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
import inet.nodes.ethernet.Eth1G;
import inet.nodes.ethernet.EtherHost;


network SingleONU
{
    types:
        channel GFX extends ned.DatarateChannel
        {
            datarate = 1Gbps;
        }
    submodules:
        pon_Splitter: PON_Splitter {
            @display("p=313,142");
        }
        epon_onu: EPON_ONU {
            @display("p=432,66");
        }
        host1: EtherHost {
            @display("p=538,66");
        }
        host2: EtherHost {
            @display("p=86,72");
        }
        epon_olt: EPON_OLT {
            @display("p=141,142");
        }
        serviceConfig: ServiceConfig {
            @display("p=261,48");
        }
    connections:

        // to splitter
        epon_olt.epon <--> GFX <--> pon_Splitter.portu;
        pon_Splitter.portg++ <--> GFX <--> epon_onu.epon;
        // Hosts
        epon_onu.ethg <--> Eth1G <--> host1.ethg;
        epon_olt.ethg <--> Eth1G <--> host2.ethg;

}
