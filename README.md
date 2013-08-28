Forked Andreas Bontozoglou's EPON implementation from http://sourceforge.net/projects/omneteponmodule/ 
Ported to work with INET 2.2.


DESCRIPTION

This is a basic implementation of Ethernet Passive Optical Network. OLT and ONU modules 
are defined and they both support one or multiple LLIDs. MPCP protocol has been implemented 
on OLT and ONU models to assign LLIDs dynamically. The devices are implemented in a structured 
way per network Layer and basic goal is to keep them extensible. Different Queue Management, 
Relay and traffic classification modules should be easily integrated into the existing devices 
using the same MAC and MAC Control Layers. Since it is under development, many features may be 
missing. 


FEATURES

- MPCP Protocol implementation. 

The current implementation varies a bit from the RFCs on the registration process. It currently 
send the GATE message AFTER the ONU acknowledgment, while the drafts say that the GATE should be 
sent right after the REGISTER message. This way we avoid allocating resources for an ONU that may 
not send ACK message later. Actually it just simplifies the upstream bandwidth allocation.

- Layers

MAC: A copy of the inet mac layer with some changes. First it sends messages of type EtherFrameWithLLID, 
which are actually Ethernet frames but in the preamble they include the LLID. So EPON_PREAMBLE_BYTES 
is now (7 - 2) and the frame structure is changed. These frames encapsulate the real Ethernet ones. 
Second, when the interface is IDLE and wakes-up it _will_ wait for IFG time before transmitting (instead 
of transmitting directly as in the original implementation)

MacCtl Layer: is used to handle time registers on both ONU and OLT. It is also responsible for 
transmitting to the lower layer at proper time. Currently it seems to work :-) but if you don't want 
to mess up, try not to touch it... I have spent days in order to achieve the TDMA. IF you see collisions 
in the Splitter module it means that something went wrong.

Q_mgmt: This module is supposed to do queue management but currently it is not implemented. Bandwidth 
allocation should be calculated here. This module is extensible and a module interface has been used 
in the model. So you can replace it with your own module having different functionality.

EPON_Relay: This module implements a switch between MAC interface and PON interface. It takes under 
consideration the LLIDs on the pon network and the VLANs on the Ethernet one. This module is extensible 
and a module interface has been used in the model. So you can replace it with your own module having 
different functionality.

PON_Splitter: Simple passive splitting. On collision, the module discards the second frame while the 
first one has already been transmitted. Also it counts upstream and downstream collisions. This should 
actually never happen, but is really useful if you want to change the TDMA functions.


EXTENDING

Some features have been added lately in order to make extending the EPON easier. Now both polling-based 
and non-polling MacCtl modules are included. For polling setups use the ONUMacCtl_P while for non-polling
use the ONUMacCtl_NP (see example .ned files).

For implementing new algorithms, for polling create a new module that will extend the OLTQPerLLiDBase_P 
class. For non-polling you should extend the OLT_QPL_RR class. IN both cases overriding the DoUpstreamDBA
method should be sufficient. More complicated algorithms (like Multi-Tread Polling or Two-Stage DBA) will
require more changes (more methods need to be overridden) 


For any comments mail me at bodozoglou@gmail.com

Andreas
