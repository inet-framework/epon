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

#include "EPON_CtrlInfo.h"

EPON_LLidCtrlInfo::EPON_LLidCtrlInfo() {
	llid = 0;
}

EPON_LLidCtrlInfo::EPON_LLidCtrlInfo(int lid){
	llid = lid;
}

EPON_LLidCtrlInfo::~EPON_LLidCtrlInfo() {
	// TODO Auto-generated destructor stub
}


std::string EPON_LLidCtrlInfo::info() const{
	std::string ret="LLiD: ";
	char tos[10];
	sprintf(tos,"%d",llid);
	return ret+tos;
}
cObject * EPON_LLidCtrlInfo::dup() const{
	return new EPON_LLidCtrlInfo(llid);
}
