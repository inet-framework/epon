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

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "EPON_mac.h"
#include "IPassiveQueue.h"
#include "NotificationBoard.h"
#include "NotifierConsts.h"
#include "EPON_CtrlInfo.h"

Define_Module(EPON_mac);

EPON_mac::EPON_mac()
{
}
EPON_mac::~EPON_mac(){
	for (int i=0; i<txQueue.innerQueue->length(); i++)
		delete txQueue.innerQueue->pop();
}

void EPON_mac::initialize()
{
    EtherMACBase::initialize();
    duplexMode = true;
    beginSendFrames();
}

void EPON_mac::initializeFlags()
{
    EtherMACBase::initializeFlags();

    duplexMode = true;
    physInGate->setDeliverOnReceptionStart(false);
}

void EPON_mac::handleMessage(cMessage *msg)
{
    if (channelsDiffer)
        readChannelParameters(true);

    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else if (msg->getArrivalGate() == upperLayerInGate)
        processFrameFromUpperLayer(check_and_cast<EtherFrame *>(msg));
    else if (msg->getArrivalGate() == physInGate ||
             msg->getArrivalGate() == gate("direct"))
        processMsgFromNetwork(check_and_cast<cPacket *>(msg));
    else
        throw cRuntimeError("Message received from unknown gate!");

    if (ev.isGUI())
        updateDisplayString();
}

void EPON_mac::handleSelfMessage(cMessage *msg)
{
    EV << "Self-message " << msg << " received\n";

    if (msg == endTxMsg)
        handleEndTxPeriod();
    else if (msg == endIFGMsg)
        handleEndIFGPeriod();
    else if (msg == endPauseMsg)
        handleEndPausePeriod();
    else
        throw cRuntimeError("Unknown self message received!");
}
/**
 * When Tx starts we have already added Logical
 * Link Identifier.
 */
void EPON_mac::startFrameTransmission()
{
    ASSERT(curTxFrame);
    EV << "Transmitting a copy of frame " << curTxFrame << endl;

    EtherFrameWithLLID *frame = (EtherFrameWithLLID *) curTxFrame->dup();  // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()

    //if (frame->getSrc().isUnspecified())
    //   frame->setSrc(address);

    if (frame->getByteLength() < curEtherDescr->frameMinBytes)
        frame->setByteLength(curEtherDescr->frameMinBytes);

    // add preamble and SFD (Starting Frame Delimiter), then send out
    frame->addByteLength(PREAMBLE_BYTES+SFD_BYTES);

    // send
    EV << "Starting transmission of " << frame << endl;
    send(frame, physOutGate);

    scheduleAt(transmissionChannel->getTransmissionFinishTime(), endTxMsg);
    transmitState = TRANSMITTING_STATE;

}

void EPON_mac::processFrameFromUpperLayer(EtherFrame *frame)
{
    if (frame->getByteLength() < MIN_ETHERNET_FRAME_BYTES)
        frame->setByteLength(MIN_ETHERNET_FRAME_BYTES);  // "padding"

    frame->setFrameByteLength(frame->getByteLength());

    EV << "Received frame from upper layer: " << frame << endl;

    emit(packetReceivedFromUpperSignal, frame);

    if (frame->getDest().equals(address))
    {
        error("logic error: frame %s from higher layer has local MAC address as dest (%s)",
                frame->getFullName(), frame->getDest().str().c_str());
    }

    if (frame->getByteLength() > MAX_ETHERNET_FRAME_D1Q)
    {
        error("packet from higher layer (%d bytes) exceeds maximum Ethernet frame size (%d)",
                (int)(frame->getByteLength()), MAX_ETHERNET_FRAME_D1Q);
    }

    if (!connected || disabled)
    {
        EV << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping packet " << frame << endl;
        emit(dropPkFromHLIfaceDownSignal, frame);
        numDroppedPkFromHLIfaceDown++;
        delete frame;

        requestNextFrameFromExtQueue();
        return;
    }

    // fill in src address if not set
    if (frame->getSrc().isUnspecified())
        frame->setSrc(address);

    bool isPauseFrame = (dynamic_cast<EtherPauseFrame*>(frame) != NULL);

    if (!isPauseFrame)
    {
        numFramesFromHL++;
        emit(rxPkFromHLSignal, frame);
    }

    if (txQueue.extQueue)
    {
        ASSERT(curTxFrame == NULL);
        curTxFrame = frame;
    }
    else
    {
        if (txQueue.innerQueue->isFull())
            error("txQueue length exceeds %d -- this is probably due to "
                  "a bogus app model generating excessive traffic "
                  "(or if this is normal, increase txQueueLimit!)",
                  txQueue.innerQueue->getQueueLimit());
        // store frame and possibly begin transmitting
        EV << "Frame " << frame << " arrived from higher layers, enqueueing\n";

        EPON_LLidCtrlInfo *nfo=dynamic_cast<EPON_LLidCtrlInfo *>(frame->getControlInfo());
        EtherFrameWithLLID * llid_eth = new EtherFrameWithLLID();
        llid_eth->setName(frame->getName());

        if (nfo != NULL){
            EV << "Control Info FOUND"<<endl;
            // Convert to EtherFrameWithLLID

            uint16_t tmpLlid = nfo->llid;
            // Relay didn't knew the llid...
            if (tmpLlid == -1)
                tmpLlid=LLID_EPON_BC;
            llid_eth->setLlid(tmpLlid);
        }else{
            // MPCP Frames may not have LLIDs
            // Encapsulate BC
            uint16_t tmpLlid = LLID_EPON_BC;
            llid_eth->setLlid(tmpLlid);
        }


        /*
         * YOU HAVE TO MANUALLY SET THE BYTE LENGTH...
         */
        llid_eth->addByteLength(2);     // add for the 2 bytes in the preamble
        frame->addByteLength(EPON_PREAMBLE_BYTES+SFD_BYTES); // add the rest
        llid_eth->encapsulate(frame);
        //frame=NULL;

        // store frame and possibly begin transmitting
        EV << "Packet " << llid_eth << " arrived from higher layers, enqueueing\n";
        EV << "After enc Size " << llid_eth->getByteLength()<< " \n";
        EV << "Orig. Size " << frame->getByteLength()<< " \n";

        txQueue.innerQueue->insertFrame(llid_eth);

        if (!curTxFrame && !txQueue.innerQueue->empty())
            curTxFrame = (EtherFrame*)txQueue.innerQueue->pop();
    }

    if (transmitState == TX_IDLE_STATE)
        scheduleEndIFGPeriod();
}

void EPON_mac::processMsgFromNetwork(cPacket *msg)
{
    EV << "Received frame from network: " << msg << endl;

    if (!connected || disabled)
    {
        EV << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping msg " << msg << endl;
        if (dynamic_cast<EtherFrame *>(msg))    // do not count JAM and IFG packets
        {
            emit(dropPkIfaceDownSignal, msg);
            numDroppedIfaceDown++;
        }
        delete msg;

        return;
    }

    EtherFrame *ethframe = dynamic_cast<EtherFrame *>(msg);
    EtherFrameWithLLID * ethllid= dynamic_cast<EtherFrameWithLLID *>(msg);

    if ( ethframe== NULL && ethllid== NULL){
        error("EPON_mac: Unrecognized Frame... Shit happens");
    }

    // IF we have EPON frame with LLID de-capsulate
    if (ethllid){
        ethframe = check_and_cast<EtherFrame *>(ethllid->decapsulate());
        delete ethllid->decapsulate();
        // Add Control Information to it
        EV << "IT HAD LLID INFO ... : "<<ethllid->getLlid()<<endl;
        if (ethframe->getControlInfo()==NULL)
        ethframe->setControlInfo(new EPON_LLidCtrlInfo(ethllid->getLlid()) );
        // Add original pre-amble.. to be used from the base class...
        ethframe->addByteLength(-EPON_PREAMBLE_BYTES);
        ethframe->addByteLength(PREAMBLE_BYTES);
        // Get rid of the original
        delete msg;
    }

    if (!dropFrameNotForUs(ethframe))
    {
        if (dynamic_cast<EtherPauseFrame*>(ethframe) != NULL)
        {
            int pauseUnits = ((EtherPauseFrame*)ethframe)->getPauseTime();
            delete ethframe;
            numPauseFramesRcvd++;
            emit(rxPausePkUnitsSignal, pauseUnits);
            processPauseCommand(pauseUnits);
        }
        else
        {
            processReceivedDataFrame((EtherFrame *)ethframe);
        }
    }
}

void EPON_mac::handleEndIFGPeriod()
{
    if (transmitState != WAIT_IFG_STATE)
        error("Not in WAIT_IFG_STATE at the end of IFG period");

    // End of IFG period, okay to transmit
    EV << "IFG elapsed" << endl;

    beginSendFrames();
}

void EPON_mac::handleEndTxPeriod()
{
    // we only get here if transmission has finished successfully
    if (transmitState != TRANSMITTING_STATE)
        error("End of transmission, and incorrect state detected");

    if (NULL == curTxFrame)
        error("Frame under transmission cannot be found");

    emit(packetSentToLowerSignal, curTxFrame);  //consider: emit with start time of frame

    if (dynamic_cast<EtherPauseFrame*>(curTxFrame) != NULL)
    {
        numPauseFramesSent++;
        emit(txPausePkUnitsSignal, ((EtherPauseFrame*)curTxFrame)->getPauseTime());
    }
    else
    {
        //unsigned long curBytes = curTxFrame->getFrameByteLength();
        numFramesSent++;
        //numBytesSent += curBytes;
        emit(txPkSignal, curTxFrame);
    }

    EV << "Transmission of " << curTxFrame << " successfully completed\n";
    delete curTxFrame;
    curTxFrame = NULL;
    lastTxFinishTime = simTime();
    getNextFrameFromQueue();

    if (pauseUnitsRequested > 0)
    {
        // if we received a PAUSE frame recently, go into PAUSE state
        EV << "Going to PAUSE mode for " << pauseUnitsRequested << " time units\n";

        scheduleEndPausePeriod(pauseUnitsRequested);
        pauseUnitsRequested = 0;
    }
    else
    {
        EV << "Start IFG period\n";
        scheduleEndIFGPeriod();
    }
}


void EPON_mac::finalize(){
	// Do logging
	if (!disabled)
	{
		simtime_t t = simTime();
		if (t>0)
		{
//			recordScalar("frames/sec sent", TxRateVector);
		}
	}
}

void EPON_mac::beginSendFrames()
{
    // Copied from EtherMACFullDuplex.cc
    if (curTxFrame)
    {
        // Other frames are queued, transmit next frame
        EV << "Transmit next frame in output queue\n";
        startFrameTransmission();
    }
    else
    {
        // No more frames set transmitter to idle
        transmitState = TX_IDLE_STATE;
        if (!txQueue.extQueue){
            // Output only for internal queue (we cannot be shure that there
            //are no other frames in external queue)
            EV << "No more frames to send, transmitter set to idle\n";
        }
    }
}

void EPON_mac::scheduleEndIFGPeriod()
{
    EtherIFG gap;
    transmitState = WAIT_IFG_STATE;
    scheduleAt(simTime() + transmissionChannel->calculateDuration(&gap), endIFGMsg);
}

void EPON_mac::processReceivedDataFrame(EtherFrame *frame)
{
    emit(packetReceivedFromLowerSignal, frame);

    // strip physical layer overhead (preamble, SFD) from frame
    frame->setByteLength(frame->getFrameByteLength());

    // statistics
    unsigned long curBytes = frame->getByteLength();
    numFramesReceivedOK++;
    numBytesReceivedOK += curBytes;
    emit(rxPkOkSignal, frame);

    numFramesPassedToHL++;
    emit(packetSentToUpperSignal, frame);
    // pass up to upper layer
    send(frame, "upperLayerOut");
}

void EPON_mac::processPauseCommand(int pauseUnits)
{
    if (transmitState == TX_IDLE_STATE)
    {
        EV << "PAUSE frame received, pausing for " << pauseUnitsRequested << " time units\n";
        if (pauseUnits > 0)
            scheduleEndPausePeriod(pauseUnits);
    }
    else if (transmitState == PAUSE_STATE)
    {
        EV << "PAUSE frame received, pausing for " << pauseUnitsRequested
           << " more time units from now\n";
        cancelEvent(endPauseMsg);

        if (pauseUnits > 0)
            scheduleEndPausePeriod(pauseUnits);
    }
    else
    {
        // transmitter busy -- wait until it finishes with current frame (endTx)
        // and then it'll go to PAUSE state
        EV << "PAUSE frame received, storing pause request\n";
        pauseUnitsRequested = pauseUnits;
    }
}

void EPON_mac::scheduleEndPausePeriod(int pauseUnits)
{
    // length is interpreted as 512-bit-time units
    cPacket pause;
    pause.setBitLength(pauseUnits * PAUSE_UNIT_BITS);
    simtime_t pausePeriod = transmissionChannel->calculateDuration(&pause);
    scheduleAt(simTime() + pausePeriod, endPauseMsg);
    transmitState = PAUSE_STATE;
}

void EPON_mac::handleEndPausePeriod()
{
    if (transmitState != PAUSE_STATE)
        error("End of PAUSE event occurred when not in PAUSE_STATE!");

    EV << "Pause finished, resuming transmissions\n";
    beginSendFrames();
}
