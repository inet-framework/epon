/*
 * ONUTableEntry.h
 *
 *  Created on: Apr 11, 2010
 *      Author: urban
 */

#ifndef ONUTABLEENTRY_H_
#define ONUTABLEENTRY_H_

#include <vector>
#include "MACAddress.h"

using std::vector;

/**
 * A struct to hold the start register and the length register
 * for each ONU in the ONUTable.
 */
typedef
struct commitedTime{
	uint32_t start;
	uint32_t length;
} CommitedTime;

/**
 * This class holds a single ONUTable entry...
 */
class ONUTableEntry : public cObject{
public:
	ONUTableEntry();
	ONUTableEntry(const ONUTableEntry & en);
	virtual ~ONUTableEntry();

protected:
	MACAddress id;
	uint16_t RTT;
	vector<uint16_t> LLIDs;
	CommitedTime comTimes;

public: // TODO: do getters/setters
	// NEW 0.8b: The requested bandwidth for 8 queues per ONU (ns16)
	uint32_t req[8];
	// NEW 0.8b: The total requested bandwidth (ns16)
	uint32_t totalReq;

public:
	void setId(MACAddress &mac);
	void setRTT(uint32_t rtt);
	int addLLID(uint16_t llid);
	void removeLLID(int idx);
	void setComTime(CommitedTime t);

	MACAddress getId();
	uint16_t getRTT();
	uint16_t getLLID(int idx);
	vector<uint16_t> & getLLIDs() const;
	int getLLIDsNum();
	CommitedTime getComTime();

	bool isValid();

	// Methods to override from cObject
	virtual cObject * dup();
	virtual std::string info();

	ONUTableEntry& operator=(const ONUTableEntry &en);
	friend std::ostream & operator<<(std::ostream &out, const ONUTableEntry &en);
};

#endif /* ONUTABLEENTRY_H_ */
