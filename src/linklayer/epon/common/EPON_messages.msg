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

//
// TODO generated message class
//

cplusplus {{
#include "Ethernet.h"
#include "EtherFrame_m.h"
#include "MACAddress.h"
#include <inttypes.h>
}};

class noncobject MACAddress;
class noncobject EthernetIIFrame;

packet SyncCode
{
    char code;
}

// This is the llid only. EtherFrame is going
// to be encapsulated into it.
packet EtherFrameWithLLID {
    uint16_t llid;
}

packet MPCP extends EthernetIIFrame
{
    uint16_t opcode;
    uint32_t ts;
}

packet MPCPGate extends MPCP
{
    uint8_t listLen;
    uint32_t startTime[];
    uint32_t duration[];
    uint16_t slotsNum;
    uint16_t slotTime;
}

packet MPCPReport extends MPCP
{
    uint8_t bitMap;
	uint32_t qInfo[];
}

packet MPCPRegReq extends MPCP
{
    uint8_t ptpNumReq;
}

packet MPCPRegister extends MPCP
{
    uint8_t ptpNumReg;
    uint16_t LLIDs[];
}

packet MPCPRegAck extends MPCP
{
}

