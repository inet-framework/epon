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

#ifndef __ONU_Q_MGMT_PERLLID_H__
#define __ONU_Q_MGMT_PERLLID_H__

#include <omnetpp.h>
#include <ModuleAccess.h>
#include "EtherFrame_m.h"
#include "MPCP_codes.h"
#include "EPON_messages_m.h"
#include "MACAddress.h"
#include "MPCPTools.h"
#include "MPCP_codes.h"
#include "EPON_mac.h"
#include "ServiceConfig.h"
#include "IPassiveQueue.h"
#include "CopyableQueueCVectors.h"
#include "ONUMacCtl_NP.h"


// Self-Messages
#define REGTOMSG			100
#define REGSENDMSG			101

/**
 * Allocation per LLID. Holds all the information around a
 * queue per LLID. These include service name, priority, max. committed
 * data rate and currently sent bytes (to be used later on DBA algorithm).
 * Also this struct holds all the scalar and vector parameters needed for
 * logging.
 *
 * NOTE: The cOutVector class is not copy-able... which means that we cannot
 * have objects of this type in this struck and use it in a vector (copy constructor
 * will be called). So what we did is to have pointers to the cOutVectors which
 * can be copied. This is done with the CopyableQueueCVectors class.
 *
 * NOTE: Call the clean() method on this class to free all the vectors...
 */
class QueuePerLLid : public QForContainer{
public:
	uint16_t llid;
	bool isDefault;

	QueuePerLLid() : QForContainer(){}
	~QueuePerLLid(){}

	friend std::ostream & operator<<(std::ostream & out, QueuePerLLid & ql){
		out<<"llid: "<<ql.llid<<" SrvName: "<<ql.getServiceName()<<"  QueueSize: "<<ql.length()<<endl;
		out<< "DataRate Limit: "<<ql.datarate_max<<" Currently sent: "<<ql.sentbits<<endl<<
				" Incoming Rate (bps): "<<ql.vec->incomingBPS <<" OutGoing Rate:"<<ql.getTxDataRate();
		return out;
	}
};


/**
 * ONU_Q_mgmt_PerLLiD creates on queue per LLID. Here is actually done
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
 *
 * UPDATE: Registration process is now done with sendDirectly to avoid collisions
 */
class ONUQPerLLiDBase : public cSimpleModule, public IPassiveQueue
{
  public:
	  ONUQPerLLiDBase();
	  virtual ~ONUQPerLLiDBase();

  protected:

	// Parameters
	double regTimeOut;
	int numOfLLIDs;
	bool allQsEmpty;
	bool allQsBlocked;
	int nextQIndex;
	int queueLimit;
	/// Granularity of QoS limit (i.e. per second <- most common)
	double granularity;

	// Services
	SrvList * serviceList;

	/// Vector of QueuePerLLid's, vector
	/// Because we need them sorted by priority
	/// but map will sort by llid (<-unique)
	typedef std::vector<QueuePerLLid> PonQueues;
	PonQueues pon_queues;

	/// The OLT MAC module for direct delivery
	cModule * olt_mac;

	/// The Current ONU Optical MAC Address
	MACAddress opt_mac;

	// Self Messages
	cMessage *regTOMsg, * regSendMsg;

    virtual void initialize(int stage);
	virtual int numInitStages() const  {return 2;}
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
    /**
     * Insert the new message to the proper Q and notify the lower layer.
     * Notification is needed only in the case that all the queues are
     * empty. This is because MAC layer will probably be in IDLE status
     * and the start Tx period is not scheduled. By notifying the MAC
     * layer we send a WakeUp msg and it will schedule the next Tx period.
     */
    virtual void processFrameFromHigherLayer(cMessage *msg);
    virtual void processFrameFromLowerLayer(cMessage *msg);

    virtual bool checkLLIDforUs(cMessage *msg);

    // Queues
    /**
	 * Search per LLID and get the index of the queue for this
	 * LLID. Useful for en-queuing frames from the higher layers.
	 */
    virtual int getIndexForLLID(uint16_t llid);

    /**
     * Return the first LLiD flagged as default
     */
    virtual int getDefaultLLiD();
    /**
	 * Adds a new queue to the PonQueues vector in the proper
	 * order based on the service priority.
	 */
    virtual void addSortedLLID(QueuePerLLid tmp_qpllid);
    virtual void checkIfAllEmpty();

    /**
     * Get queue from service name
     */
    virtual int getIndexForService(std::string name);

    // MPCP Operation
    virtual void processMPCP(EthernetIIFrame *frame );
    virtual void sendMPCPReg();
    /**
     * Initializes the MPCP registration process and sets the
     * regTOMsg (TimeOut for the registration)
     */
    virtual void startMPCPReg(uint32_t regMaxRandomSleep);

    cModule * findModuleUp(const char * name);


public:

    /**
	 * The queue should send a packet whenever this method is invoked.
	 * If the queue is currently empty, it should send a packet when
	 * when one becomes available.
	 */
	virtual void requestPacket() = 0;

	/**
	 * Return an MPCP REPORT with the status of the queues
	 */
	virtual MPCPReport * requestMPCP_REPORT();

	/**
	 * Return the MPCP Report Size in Bytes
	 */
	virtual uint16_t getMPCPRepSize();
	virtual bool isEmpty();

    virtual int getNumPendingRequests();
    virtual void clear();
    virtual void addListener(IPassiveQueueListener *listener);
    virtual void removeListener(IPassiveQueueListener *listener);

};

#endif
