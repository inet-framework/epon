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

#ifndef __PON_SPLITTER_H__
#define __PON_SPLITTER_H__

#include <omnetpp.h>
#include "MPCPTools.h"

// Since this is a small file we will have main page config here
/**
 * \mainpage EPON for OMNeT++ (4.0 - 4.1)
 *
 * \section DESCRIPTION
 *
 * This is a basic implementation of Ethernet Passive Optical Network. OLT and ONU modules
 * are defined and they both support one or multiple LLIDs. MPCP protocol has been implemented
 * on OLT and ONU models to assign LLIDs dynamically. The devices are implemented in a structured
 * way per network Layer and basic goal is to keep them extensible. Different Queue Management,
 * Relay and traffic classification modules should be easily integrated into the existing devices
 * using the same MAC and MAC Control Layers.
 *
 * \image html "../images/screenshot_small.jpeg" "Preview" width=500px
 *
 * \section BASIC FEATURES
 *
 * \subsection PON_Splitter
 *
 * Simple passive splitting. On collision, the module discards the second frame while the
 * first one has already been transmitted. Also it counts upstream and downstream collisions. This should
 * actually never happen, but is really useful if you want to change the TDMA functions.
 *
 *
 * \subsection a MPCP Protocol implementation
 *
 * The current implementation varies a bit from the RFCs on the registration process. It currently
 * send the GATE message AFTER the ONU acknowledgment, while the drafts say that the GATE should be
 * sent right after the REGISTER message. This way we avoid allocating resources for an ONU that may
 * not send ACK message later. Actually it just simplifies the upstream bandwidth allocation.
 *
 * \subsection Layers
 *
 * - <b>MAC</b>: A copy of the INET MAC layer with some changes. First it sends messages of type EtherFrameWithLLID,
 * which are actually Ethernet frames but in the preamble they include the LLID. So EPON_PREAMBLE_BYTES
 * is now (7 - 2) and the frame structure is changed. These frames encapsulate the real Ethernet ones.
 * Second, when the interface is IDLE and wakes-up it _will_ wait for IFG time before transmitting (instead
 * of transmitting directly as in the original implementation)
 *
 * - <b>MacCtl</b>: is used to handle time registers on both ONU and OLT. It is also responsible for
 * transmitting to the lower layer at proper time. Currently it seems to work :-) but if you don't want
 * to mess up, try not to touch it... I have spent days in order to achieve the TDMA. IF you see collisions
 * in the Splitter module it means that something went wrong. From release 0.8b, this module is an
 * interface, and a polling-based base class is provided (see bellow on Extending).
 *
 * - <b>Q_mgmt</b>: These modules are responsible for queue management. A simple Round Robin implementation is provided.
 * Bandwidth allocation should be calculated here. This module is extensible and a module interface has been used
 * in the model. So you can replace it with your own module having different functionality.
 *
 * - <b>EPON_Relay</b>: This module implements a switch between the MAC interface and the PON interface. It takes under
 * consideration the LLIDs on the PON network and the VLANs on the Ethernet one. This module is extensible
 * and a module interface has been used in the model. So you can replace it with your own module having
 * different functionality.
 *
 * \section EXTENDING
 *
 * Some features have been added lately in order to make extending the EPON easier. Now both polling-based
 * and non-polling MacCtl modules are included. For polling setups use the ONUMacCtl_P while for non-polling
 * use the ONUMacCtl_NP (see example .ned files).
 *
 * For implementing new algorithms, for polling create a new module that will extend the OLTQPerLLiDBase_P
 * class. For non-polling you should extend the OLT_QPL_RR class. IN both cases overriding the DoUpstreamDBA
 * method should be sufficient. More complicated algorithms (like Multi-Tread Polling or Two-Stage DBA) will
 * require more changes (more methods need to be overridden)
 *
 *
 * \section INSTALLATION
 *
 * \subsection Dependencies
 * -# OMNet++ versions 4.0 or 4.1 (http: *omnetpp.org/omnetpp/cat_view/17-downloads/1-omnet-releases/4-older-versions)
 * -# INET or INET/MANET FrameWork for the above versions (http: *inet.omnetpp.org/index.php?n=Main.Download)
 * -# This version depends on a smaller project for 802.1Q vlans that comes in the same archive.
 *
 *
 * Installation should be straight forward. OMNet++ 4.0 and INET framework are required. Steps:
 * - Open Omnet++ IDE
 * - Import and compile the INET project
 * - Extract the PON_<version>.tar.gz file
 * - Import Vlans project by selecting the Vlans folder of the extracted archive
 *      - Setup the Project References to point to your INET
 *      - Compile
 * - Import EPON project by selecting the PON folder of the extracted archive
 *      - Setup the Project References to: Vlans and INET
 * - Add the PON project to the project references of your work (optional - if you are building in an external proj./folder)
 * - Now everything should work...
 *
 *
 * \subsection Troubleshooting
 * Some common errors, I usually receive emails for, are listed bellow:
 *
 * 1. "inetmanet/src/linklayer/contract/Ieee802Ctrl_m.h:83:5: error: expected identifier before numeric constant"
 * There are some illegal characters in the file Ieee802Ctrl.msg and specifically in the "enum EtherType". Solutions:
 *    - Fast 'n' Dirty: Delete the enum if you do not need it!
 *    - Decent: Open the file (.msg) with a HEX editor and remove the weird characters
 *
 * 2. "Conversion from 'cPar' to 'uint64_t' is ambiguous"
 * Some casting is needed i.e: datarateLimit=(uint64_t)par("datarateLimit");
 *
 *
 * \section NOTES
 *
 * Tested on:
 * - omnetpp-4.1<br>
 * - omnetpp-4.0p1 <br>
 *
 * With many inetmanet versions but...:
 * <b>Latest INET versions will not work (v2). The same for OMNet++ 4.2!</b>. You need to get OMNet++ 4.1 and compatible INET!
 * Porting will not be done (not now at least - Any ported code is more than welcome)
 *
 * For any comments or corrections email me at bodozoglou@gmail.com
 *
 * I hope it works for you...<br>
 * Andreas
 *
 */

/**
 * PON_Splitter is a fairly simple module to forward
 * frames from ONU side to OLT side and the opposite.
 * This module thought allow you to see the collision
 * counters, which are really useful if you change the
 * timing and synchronization functions.
 */
class PON_Splitter : public cSimpleModule
{
  protected:
    int ports;          // number of ports
    uint32_t col_upstream;		// # of collisions Up Stream
    uint32_t col_downstream;		// # of collisions Down Stream
    long numMessages;   // number of messages handled
    cMessage * previousMsg;
    uint32_t haltOn;

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    virtual void printUpStreamDebug(cMessage * msg);
};

#endif
