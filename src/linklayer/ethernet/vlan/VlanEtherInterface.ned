package epon.linklayer.ethernet.vlan;
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
import inet.linklayer.IWiredNic;
import inet.linklayer.IOutputQueue;
import epon.linklayer.ethernet.vlan.VlanEtherMAC;

//
// TODO auto-generated module
//
module VlanEtherInterface like IWiredNic
{
    parameters:
        @display("bgb=296,121;i=block/ifcard");
        string QMgmtUnit = default("VlanQ_mgmt_Per");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        inout phys;

    submodules:
        mac: VlanEtherMAC {
            @display("p=54,58");
            promiscuous = default(true);
        }
        encap: VlanEtherEncap {
            @display("p=148,58");
        }
        queue: <QMgmtUnit> like IOutputQueue  {
            @display("p=245,58");
        }

    connections:
        upperLayerIn --> queue.in;
        queue.out --> encap.upperLayerIn;
        upperLayerOut <-- { @display("m=n"); } <-- encap.upperLayerOut;
        encap.lowerLayerOut --> mac.upperLayerIn;
        encap.lowerLayerIn <-- mac.upperLayerOut;
        mac.phys <--> phys;

}
