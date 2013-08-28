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

#include "ONUTable.h"


Define_Module(ONUTable);

//ONUTable::~ONUTable(){
//	tbl.clear();
//}
//ONUTable::ONUTable(){}

void ONUTable::initialize()
{
	WATCH_VECTOR(tbl);
}

void ONUTable::handleMessage(cMessage *msg)
{
	// TODO - Generated method body
	delete msg;
}

void ONUTable::addONU(ONUTableEntry &entry){

	if (tbl.size() == 0 ) {
		tbl.push_back(entry);
		return;
	}

	for (uint32_t i=0; i<tbl.size(); i++){
		// Do not add the same onu 2 times
		if (getEntry(i)->getId() == entry.getId()) {
			*getEntry(i) = entry;
			return;
		}
	}

	// If HERE, entry is new
	tbl.push_back(entry);

}
void ONUTable::removeONU(MACAddress id){
	for (uint32_t i=0; i<tbl.size(); i++){
		if (getEntry(i)->getId() == id) {
			tbl.erase(tbl.begin()+i);
			return;
		}
	}
}

void ONUTable::removeONU(uint32_t idx){
	if (idx<0 || idx>=tbl.size()) return;
	tbl.erase(tbl.begin()+idx);
}

ONUTableEntry * ONUTable::getEntry(uint32_t idx){
	if (idx<0 || idx>=tbl.size()) return NULL;
	return &tbl[idx];
}

ONUTableEntry * ONUTable::getEntry(MACAddress id){
	for (uint32_t i=0; i<tbl.size(); i++){
		if (getEntry(i)->getId() == id) {
			return &tbl[i];
		}
	}
	return NULL;
}

ONUTableEntry * ONUTable::getEntry(std::string id){
	for (uint32_t i=0; i<tbl.size(); i++){
		if (getEntry(i)->getId().str() == id) {
			return &tbl[i];
		}
	}
	return NULL;
}

int ONUTable::getTableSize(){return tbl.size();}


// Tools
/**
 * Note: The entry may be null which leads to SEGFAULT
 * This happens usually if the ONU we expect is not
 * registered...
 */
MACAddress ONUTable::getMACFromString(std::string id){
	if (getEntry(id))
		return getEntry(id)->getId();
	else
		return MACAddress::UNSPECIFIED_ADDRESS;
}
