# malt

Malt stands for _muilticast alternative listener tool_. It allows subscribing to multicast using IGMP v2 (*,G) joins
as well as IGMP v3 (S,G) joins. For the latter to work IGMP v3 must be enabled on the host.

Malt also allows to receive traffic sent to the group without specifying the UDP port. This functionality requires
either sudo or CAP_NET_RAW capability set on the binary as malt open a UDP raw socket.

At the end of the session malt shows the statistics of the received packets.
