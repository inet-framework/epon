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

#include "VlanEtherMAC.h"
#include "EtherFrame_m.h"

Define_Module(VlanEtherMAC);

using namespace std;

// ONLY TO CHANGE THE MAX ALLOWED FRAME
void VlanEtherMAC::processFrameFromUpperLayer(EtherFrame *frame)
{
	// Copied from EtherMAC.cc ----------------------------------------------------------------------------
    if (frame->getByteLength() < MIN_ETHERNET_FRAME_BYTES)
        frame->setByteLength(MIN_ETHERNET_FRAME_BYTES);  // "padding"

    frame->setFrameByteLength(frame->getByteLength());

    EV << "Received frame from upper layer: " << frame << endl;

    emit(packetReceivedFromUpperSignal, frame);

    if (frame->getDest().equals(address))
    {
        error("Logic error: frame %s from higher layer has local MAC address as dest (%s)",
                frame->getFullName(), frame->getDest().str().c_str());
    }

    if (frame->getByteLength() > MAX_ETHERNET_FRAME_D1Q)
    {
        error("Packet from higher layer (%d bytes) exceeds maximum Ethernet frame size (%d)",
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
        fillIFGIfInBurst();
    }
    else
    {
        if (txQueue.innerQueue->isFull())
            error("txQueue length exceeds %d -- this is probably due to "
                  "a bogus app model generating excessive traffic "
                  "(or if this is normal, increase txQueueLimit!)",
                  txQueue.innerQueue->getQueueLimit());

        // store frame and possibly begin transmitting
        EV << "Frame " << frame << " arrived from higher layer, enqueueing\n";
        txQueue.innerQueue->insertFrame(frame);

        if (!curTxFrame && !txQueue.innerQueue->empty())
            curTxFrame = (EtherFrame*)txQueue.innerQueue->pop();
    }

    if ((duplexMode || receiveState == RX_IDLE_STATE) && transmitState == TX_IDLE_STATE)
    {
        EV << "No incoming carrier signals detected, frame clear to send\n";
        startFrameTransmission();
    }
}

/**
 * Override to clone packet name to the duplicated frame
 */
void VlanEtherMAC::startFrameTransmission()
{
    ASSERT(curTxFrame);

    EV << "Transmitting a copy of frame " << curTxFrame << endl;

    EtherFrame *frame = curTxFrame->dup();
    frame->setName(curTxFrame->getName());

    if (frame->getSrc().isUnspecified())
        frame->setSrc(address);

    bool inBurst = frameBursting && framesSentInBurst;
    int64 minFrameLength = duplexMode ? curEtherDescr->frameMinBytes : (inBurst ? curEtherDescr->frameInBurstMinBytes : curEtherDescr->halfDuplexFrameMinBytes);

    if (frame->getByteLength() < minFrameLength)
        frame->setByteLength(minFrameLength);

    // add preamble and SFD (Starting Frame Delimiter), then send out
    frame->addByteLength(PREAMBLE_BYTES+SFD_BYTES);

    if (ev.isGUI())
        updateConnectionColor(TRANSMITTING_STATE);

    currentSendPkTreeID = frame->getTreeId();
    send(frame, physOutGate);

    // check for collisions (there might be an ongoing reception which we don't know about, see below)
    if (!duplexMode && receiveState != RX_IDLE_STATE)
    {
        // During the IFG period the hardware cannot listen to the channel,
        // so it might happen that receptions have begun during the IFG,
        // and even collisions might be in progress.
        //
        // But we don't know of any ongoing transmission so we blindly
        // start transmitting, immediately collide and send a jam signal.
        //
        EV << "startFrameTransmission(): sending JAM signal.\n";
        printState();

        sendJamSignal();
        // numConcurrentRxTransmissions stays the same: +1 transmission, -1 jam

        if (receiveState == RECEIVING_STATE)
        {
            delete frameBeingReceived;
            frameBeingReceived = NULL;

            numCollisions++;
            emit(collisionSignal, 1L);
        }
        // go to collision state
        receiveState = RX_COLLISION_STATE;
    }
    else
    {
        // no collision
        scheduleEndTxPeriod(frame);

        // only count transmissions in totalSuccessfulRxTxTime if channel is half-duplex
        if (!duplexMode)
            channelBusySince = simTime();
    }
}


