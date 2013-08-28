/*
 * MPCPTools.cpp
 *
 *  Created on: Apr 12, 2010
 *      Author: urban
 */

#include "MPCPTools.h"

MPCPTools::MPCPTools() {
	// TODO Auto-generated constructor stub

}

MPCPTools::~MPCPTools() {
	// TODO Auto-generated destructor stub
}


void MPCPTools::setGateLen(MPCPGate &gate, uint8_t len){
	gate.setListLen(len);
	gate.setStartTimeArraySize(len);
	gate.setDurationArraySize(len);
}

uint64_t MPCPTools::simTimeToNS16(){
	// NOTE: You can add skew here...
	int scale= (-9 -simTime().getScaleExp())/3;
	// Clock Granularity is 16ns
	return (simTime().raw()/(scale*1000*16))%MPCP_CLOCK_MAX;
}

uint64_t MPCPTools::simTimeToNS16(uint64_t time){
	int scale= (-9 -simTime().getScaleExp())/3;
	// Clock Granularity is 16ns
	return (time/(scale*1000*16))%MPCP_CLOCK_MAX;
}

uint64_t MPCPTools::ns16ToSimTime(uint64_t time){
	int scale= (-9 -simTime().getScaleExp())/3;
	// Clock Granularity is 16ns
	return time * (scale*1000*16);
}

uint64_t MPCPTools::nsToSimTime(uint64_t time){
	int scale= (-9 -simTime().getScaleExp())/3;
	// Clock Granularity is 16ns
	return time * (scale*1000);
}

uint64_t MPCPTools::bitsToNS16(uint64_t bits, uint8_t gigRate){
	uint64_t rate = gigRate * pow(10,9);
	// tx time in seconds
	double txtime = ((double)bits)/rate;
	// tx time in ns
	txtime*=pow(10,9);
	// ns16
	return (txtime/16);
}
uint64_t MPCPTools::bytesToNS16(uint64_t bytes, uint8_t gigRate){
	return bitsToNS16(bytes*8, gigRate);
}

uint64_t MPCPTools::ns16ToBits(uint64_t ns16, uint8_t gigRate){
	uint64_t rate = gigRate * pow(10,9);
	// ns
	uint64_t bits = 16*ns16;

	// sec
	double seconds = ((double)bits)/pow(10,9);

	// bits
	bits = seconds*rate;
	return bits;
}

uint64_t MPCPTools::ns16ToBytes(uint64_t ns16, uint8_t gigRate){
	return ns16ToBits(ns16,gigRate)/8;
}
