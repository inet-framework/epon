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

#ifndef __ONUTABLE_H__
#define __ONUTABLE_H__

#include <omnetpp.h>
#include <vector>
#include "ONUTableEntry.h"
#include "MACAddress.h"

using std::vector;

/**
 * A simple class containing a vector of ONUTableEntry
 * objects. ONUTable also provides some basic functions
 * for adding, removing and finding specific entries based
 * on MAC address or index. NOTE: DO NOT FORGET: It is NOT
 * a vector so you must not call size() BUT instead call
 * getTableSize().
 */
class ONUTable : public cSimpleModule
{
  protected:
	vector<ONUTableEntry> tbl;
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

  public:
//	ONUTable();
//	virtual ~ONUTable();
	virtual void addONU(ONUTableEntry &entry);
	virtual void removeONU(MACAddress id);
	virtual void removeONU(uint32_t idx);

	// Pointer to be able to change stuff...
	virtual ONUTableEntry * getEntry(uint32_t idx);
	virtual ONUTableEntry * getEntry(MACAddress id);
	virtual ONUTableEntry * getEntry(std::string id);
	virtual int getTableSize();

	// Tools
	virtual MACAddress getMACFromString(std::string id);
};

#endif
