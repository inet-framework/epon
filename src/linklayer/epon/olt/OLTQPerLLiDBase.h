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

#ifndef __OLT_Q_MGMT_PERLLID_H__
#define __OLT_Q_MGMT_PERLLID_H__

#include <omnetpp.h>
#include <vector>
#include "ONUTable.h"
#include "EPON_messages_m.h"
#include "MPCP_codes.h"
#include "MPCPTools.h"
#include "ONUTable.h"
#include "VlanMACRelayUnitBase.h"
#include "ServiceConfig.h"
#include "IPassiveQueue.h"
#include "EPON_CtrlInfo.h"
#include "CopyableQueueCVectors.h"



// Self Messages
#define ONUPENACK	100


/**
 * Allocation per MAC-LLID. Holds all the information around a
 * queue per LLID. These include service name, priority, max. committed
 * data rate and currently sent bytes (to be used later on DBA algorithm).
 * Also this class holds all the scalar and vector parameters needed for
 * logging.
 *
 * NOTE: The cOutVector class is not copyable... which means that we cannot
 * have objects of this type in this class and use it in a vector (copy constructor
 * will be called). So what we did is to have pointers to the cOutVectors which
 * can be copied. This is done with the CopyableQueueCVectors class.
 *
 * NOTE: Call the clean() method on this class to free all the vectors...
 */
class QueuePerMacLLid : public QForContainer{
public:
	// Only the index... all the others are inherited
	mac_llid ml;
	bool isDefault;

	QueuePerMacLLid() : QForContainer(){}
	~QueuePerMacLLid(){}

	// Friends
	friend std::ostream & operator<<(std::ostream & out, QueuePerMacLLid & qml){
		out<<qml.ml<<" SrvName: "<<qml.getServiceName()<<" QueueSize: "<<qml.length();
		return out;
	}
};

/**
 * OLT_Q_mgmt_PerLLiD creates on queue per MAC-LLID tuple. Here is actually done
 * all the bandwidth allocation and timer calculation. Also this class handles the
 * ONU registration and the updates the ONU table. Thus some MPCP messages are
 * generated from here and timeouts are defined here. Now since all the time
 * management is done here... we have to define the number of time slots and the
 * time slot duration. Finally this class is aware of any services in this network
 * and thus has a pointer to the SrvList class (if any module is available in the
 * scenario else it is NULL).
 *
 * The whole idea is to extend this class and create your own bandwidth allocation
 * algorithms. The default behavior of this class is to apply round robin on the
 * queues (per frame). The only thing considered is the priority (not strictly) and
 * happens because the queues are sorted based on the service priority.
 *
 * Basic methods that would be useful to override when you extend are explained
 * below.
 */
class OLTQPerLLiDBase : public cSimpleModule, public IPassiveQueue
{
  public:
	OLTQPerLLiDBase();
	virtual ~OLTQPerLLiDBase();

  protected:

	// Pointer to the ONU table
	ONUTable * onutbl;
	vector<ONUTableEntry> temptbl;
	vector<cMessage *> pendingAck;

	// Services
	SrvList * serviceList;

	//Parameters
	double regAckTimeOut;
	uint16_t slotLength;
	uint16_t slotNumber;
	int32_t regTimeInt;
	int queueLimit;
	uint64_t datarateLimit;

	// Queues
	/// The vector holding the queues
	typedef std::vector<QueuePerMacLLid> PonQueues;
	PonQueues pon_queues;
	bool allQsEmpty;
	int nextQIndex;



    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);

    virtual void processFrameFromHigherLayer(cMessage *msg);
    virtual void processFrameFromLowerLayer(cMessage *msg);

    // MPCP Operations
    virtual void processMPCP(EthernetIIFrame *frame );
    virtual void handleRegTimeOut(cMessage *msg);
    virtual void handleMPCPReport(MPCPReport *msg);
    virtual void doOnuRegistration(MACAddress mac);


    // TOOLS TODO: add to a different class
    virtual cModule * findModuleUp(const char * name);


    /**
     * Create a new queue for an entry in the ONU table.
     * This method also calls the DoUpstreamDBA() to calculate
     * the new granted times and finally calls the SendGateUpdates()
     * to announce these to the ONUs.
     */
    virtual void createONU_Q(ONUTableEntry & en);
    virtual bool existsInPONQueues(mac_llid ml);

    /**
     * This method calculates the upstream timers
     * for each ONU in the ONUTable. This could be overridden
     * to change the default behavior. The default is fair
     * allocation per ONU.
     *
	 * In addition this method scale down the simulation
	 * data rates. If datarateLimit is set > 0 then only this
	 * is going to be allocated to the ONUs.
     *
     * FOR NON-POLLING ALGORITHMS 2 Basic RULES At the end this method should: <br>
     *  - Have set the _total_ length and start registers on the ONU table
     *  - Call SendGateUpdates() to announce to ONUs
     *
     *  FOR POLLING ALGORITHMS this method should:<br>
     *  - Be called periodically to handle the next ONU (poll)
     *  - It should schedule the MPCP GATE sending
     *  - Example implementation: sendSingleGate() notifies ONU_i and calls
     *    doUpstreamDBA for ONU i+1 (which in turn schedules the next GATE).
     *    This is the ONLINE approach...
     */
	virtual void DoUpstreamDBA();

	/**
	 * Generate and send the initial gate message. This message
	 * is used for the registration of the ONUs.
	 */
	virtual void sendMPCPGateReg();

	/**
	 * Generate MPCPGate messages announcing the transmission
	 * times to the ONUs. (For all the ONUs in the ONUTable)
	 *
	 * This method should be called at the end of DoUpstreamDBA
	 * ONLY is the algorithm is NON-POLLING (burst) BASED. Polling
	 * algorithms have to override the DoUpstreamDBA to handle
	 * both allocation and GATE/GRANDs...
	 */
	virtual void SendGateUpdates();

	// Queues
	virtual void checkIfAllEmpty();
	int getIndexForLLID(uint16_t llid);
	/**
	 * Add a new queue to the pon_queues sorted by priority
	 * Higher priority Qs go up.
	 */
	void addSortedMacLLID(QueuePerMacLLid tmp_qpml);

	/**
	 * Get the higher priority queue for the specified MAC
	 * address. Since Qs are sorted, the first match is
	 * returned
	 */
	QueuePerMacLLid * getFastestQForMac(const MACAddress & mac);

	/**
	 * Get the higher priority queue for the specified MAC
	 * address. Since Qs are sorted, the first match is
	 * returned
	 */
	QueuePerMacLLid * getFastestQForMac(const std::string & mac);
	uint16_t getFastestLLiDForMac(const std::string & mac);

	virtual bool isEmpty();

	/**
	 * Return the Super Slot length (ns)
	 */
	uint64_t getSuperSlot_ns();

	/**
	 * Return the Super Slot length (sec). Useful
	 * for calculating data rates per second.
	 */
	double getSuperSlot_sec();

    virtual int getNumPendingRequests();
    virtual void clear();
    virtual void addListener(IPassiveQueueListener *listener);
    virtual void removeListener(IPassiveQueueListener *listener);
};

#endif
