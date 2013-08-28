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

#include "ONU_QPL_RR.h"

Define_Module(ONU_QPL_RR);

void ONU_QPL_RR::initialize(int stage){
	ONUQPerLLiDBase::initialize(stage);
}




void ONU_QPL_RR::requestPacket(){
	Enter_Method("requestPacket()");

	bool found=false;
	// if the current Q is empty find the next one
	if (pon_queues[nextQIndex].isEmpty()){
		for (int i=(nextQIndex+1)%pon_queues.size(); i!=nextQIndex; i=((i+1)%pon_queues.size())){
			if (!pon_queues[i].isEmpty()) {
				nextQIndex=i;
				found=true;
				break;
			}
		}
	// If not empty.. we have a frame
	}else found=true;


	// if all empty return NULL
	if (!found){
		allQsEmpty = true;
		return;
	}


	// Send the next message to lower layer out
	cPacket * msg = (cPacket *)pon_queues[nextQIndex].pop();


	// Point to the next Q (Round Robin)
	nextQIndex=(nextQIndex+1)%pon_queues.size();
	// Check is it was the last one...
	checkIfAllEmpty();


	send(msg,"lowerLayerOut");
}


