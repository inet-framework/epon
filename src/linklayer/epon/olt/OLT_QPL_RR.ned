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

package epon.linklayer.epon.olt;

//
// TODO auto-generated module
//
simple OLT_QPL_RR like OLT_Q_mgmt
{
    parameters:
        int slotLength @unit("ns") = default(3000ns);
        int slotNumber = default(128);
        int regTimeInt @unit("ms") = default(100ms);
        int regAckTimeOut @unit("ms") = default(500ms);
        int queueLimit = default(100);
        
        int datarateLimit @unit("Mbps") = default(0Mbps); // Limit the assigned bandwidth to scale down the simulation

        @display("i=block/fork");

    gates:
        input lowerLayerIn;
        output lowerLayerOut;
        input upperLayerIn;
        output upperLayerOut;
}
