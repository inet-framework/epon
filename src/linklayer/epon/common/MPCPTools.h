/*
 * MPCPTools.h
 *
 *  Created on: Apr 12, 2010
 *      Author: urban
 */

#ifndef MPCPTOOLS_H_
#define MPCPTOOLS_H_

#include <inttypes.h>


const uint32_t MPCP_CLOCK_MAX=		0xFFFFFFFF;

#include "EPON_messages_m.h"

/**
 * A class to handle simTime and convert it to nanoseconds clock
 * 32 bit with 16ns granularity.
 *
 * NOTE: DO NEVER COMPARE IN SIM TIME WHEN YOU ARE USING THE
 * ns16ToSimTime... NEVER. DO THE OPPOSITE COMPARE IN DEVICE
 * TIME USING simTimeToNS16(). (<- Now I don't remember why... but
 * I should be right...)
 */
class MPCPTools {
public:
	MPCPTools();
	virtual ~MPCPTools();

	static void setGateLen(MPCPGate &gate, uint8_t len);
	/*
	 * Current simTime to ns16 granularity. Circle clock
	 */
	static uint64_t simTimeToNS16();
	/*
	 * Given time to ns16 granularity. Circle clock
	 */
	static uint64_t simTimeToNS16(uint64_t time);
	/*
	 * Given time to simTime granularity
	 */
	static uint64_t ns16ToSimTime(uint64_t time);

	/*
	 * Given time (ns) to simTime granularity
	 */
	static uint64_t nsToSimTime(uint64_t time);

	/**
	 * Convert bits to ns16. Uses the line rate given in Gbps
	 */
	static uint64_t bitsToNS16(uint64_t bits, uint8_t gigRate);
	/**
	 * Convert bytes to ns16. Uses the line rate given in Gbps
	 */
	static uint64_t bytesToNS16(uint64_t bytes, uint8_t gigRate);


	/**
	 * Convert ns16 to bits. Uses the line rate given in Gbps
	 */
	static uint64_t ns16ToBits(uint64_t ns16, uint8_t gigRate);

	/**
	 * Convert ns16 to Bytes. Uses the line rate given in Gbps
	 */
	static uint64_t ns16ToBytes(uint64_t ns16, uint8_t gigRate);

};

#endif /* MPCPTOOLS_H_ */
