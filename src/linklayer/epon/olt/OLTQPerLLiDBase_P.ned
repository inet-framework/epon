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
// This module overrides the Base functionality to support polling 
// implementations/algorithms
//
simple OLTQPerLLiDBase_P like OLT_Q_mgmt
{
    parameters:
        int slotLength @unit("ns") = default(3000ns);
        int slotNumber = default(128);
        int regTimeInt @unit("ms") = default(100ms);
        int regAckTimeOut @unit("ms") = default(500ms);
        int queueLimit = default(100);
        
        int datarateLimit @unit("Mbps") = default(1Mbps); // Limit the assigned bandwidth to scale down the simulation
        
        /// Polling parameters
        double wMax @unit("ms") = default(1ms); 	 // Maximum bandwidth for LIMITED Service
        double fixedWin @unit("ms") = default(0.5ms); // Maximum window for FIXED Service (500000 bits/cycle - 40 frames of 1500B)

        @display("i=block/fork");

    gates:
        input lowerLayerIn;
        output lowerLayerOut;
        input upperLayerIn;
        output upperLayerOut;
}
