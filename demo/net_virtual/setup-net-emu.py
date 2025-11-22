import os
import sys
import time

#======================================================================================================================#
#=> - Helpful commands -
#======================================================================================================================#
#
# - sudo ip netns exec emu-h1 ifconfig
# - sudo ip netns exec emu-r1 ifconfig
# - sudo ip netns exec emu-h2 ifconfig
# - sudo ip netns exec emu-h3 ifconfig
#
# - sudo ip netns exec emu-h1 ping -c3 10.0.20.2
# - sudo ip netns exec emu-h1 ping -c3 10.0.10.2
# - sudo ip netns exec emu-h2 ping -c3 10.0.10.1
# - sudo ip netns exec emu-h1 ping -c3 10.0.20.1
#
# - To balance counts: r1 r1 r1, h2, h2, h3, h3, h3, h3
#
#======================================================================================================================#
#=> - Helpers -
#======================================================================================================================#

def execute_command(cmd):
	stream = os.popen(cmd)
	output = stream.read()
	print("*** Command: %s" %(cmd))
	if(len(output) > 0):
		print("*** Output: %s" %(output))

#======================================================================================================================#
#=> - Main -
#======================================================================================================================#

steps_to_run = ["1", "2", "topo", "route"]
if len(sys.argv) > 1:
	steps_to_run = sys.argv[1:]

if "1" in steps_to_run or "topo" in steps_to_run:
	print("\n*** Spawning new nodes")
	
	execute_command("sysctl -w net.ipv4.tcp_rmem='16384 16777216 268435456'")
	execute_command("sysctl -w net.ipv4.tcp_wmem='16384 16777216 268435456'")
	execute_command("sysctl -w net.ipv4.tcp_mem='16384 16777216 268435456'")
	execute_command("sysctl -w net.ipv4.tcp_timestamps=0")
	#execute_command("sysctl -w net.core.rmem_default=268435456")
	#execute_command("sysctl -w net.core.rmem_max=268435456")
	#execute_command("sysctl -w net.core.wmem_default=268435456")
	#execute_command("sysctl -w net.core.wmem_max=268435456")
	execute_command("sysctl net.ipv4.tcp_congestion_control=bbr")

	print("\n*** Creating namespace: emu-h1")
	execute_command("ip netns add emu-h1")
	execute_command("ip netns exec emu-h1 sysctl -w net.ipv4.ip_forward=1")

	print("\n*** Creating namespace: emu-r1")
	execute_command("ip netns add emu-r1")
	execute_command("ip netns exec emu-r1 sysctl -w net.ipv4.ip_forward=1")

	print("\n*** Creating namespace: emu-h2")
	execute_command("ip netns add emu-h2")
	execute_command("ip netns exec emu-h2 sysctl -w net.ipv4.ip_forward=1")

	print("\n*** Creating namespace: emu-h3")
	execute_command("ip netns add emu-h3")
	execute_command("ip netns exec emu-h3 sysctl -w net.ipv4.ip_forward=1")

	print("\n*** Creating intf: h1-dev-r1 (in emu-h1)")
	execute_command("ip link add h1-dev-r1 type veth peer name r1-dev-h1")
	execute_command("ip link set h1-dev-r1 netns emu-h1")
	execute_command("ip link set r1-dev-h1 netns emu-r1")
	execute_command("ip netns exec emu-h1 ip addr add 10.0.10.1 dev h1-dev-r1")
	execute_command("ip netns exec emu-r1 ip addr add 10.0.10.2 dev r1-dev-h1")
	execute_command("ip netns exec emu-h1 ifconfig h1-dev-r1 hw ether 00:00:10:00:10:01")
	execute_command("ip netns exec emu-r1 ifconfig r1-dev-h1 hw ether 00:00:10:00:10:02")
	execute_command("ip netns exec emu-h1 ip link set dev h1-dev-r1 up")
	execute_command("ip netns exec emu-r1 ip link set dev r1-dev-h1 up")
	execute_command("ip netns exec emu-h1 ifconfig h1-dev-r1 netmask 255.255.255.0")
	execute_command("ip netns exec emu-r1 ifconfig r1-dev-h1 netmask 255.255.255.0")

	print("\n*** Creating intf: r1-dev-h2 (in emu-r1)")
	execute_command("ip link add r1-dev-h2 type veth peer name h2-dev-r1")
	execute_command("ip link set r1-dev-h2 netns emu-r1")
	execute_command("ip link set h2-dev-r1 netns emu-h2")
	execute_command("ip netns exec emu-r1 ip addr add 10.0.20.1 dev r1-dev-h2")
	execute_command("ip netns exec emu-h2 ip addr add 10.0.20.2 dev h2-dev-r1")
	execute_command("ip netns exec emu-r1 ifconfig r1-dev-h2 hw ether 00:00:10:00:20:01")
	execute_command("ip netns exec emu-h2 ifconfig h2-dev-r1 hw ether 00:00:10:00:20:02")
	execute_command("ip netns exec emu-r1 ip link set dev r1-dev-h2 up")
	execute_command("ip netns exec emu-h2 ip link set dev h2-dev-r1 up")
	execute_command("ip netns exec emu-r1 ifconfig r1-dev-h2 netmask 255.255.255.0")
	execute_command("ip netns exec emu-h2 ifconfig h2-dev-r1 netmask 255.255.255.0")

	print("\n*** Creating intf: r1-dev-h3 (in emu-r1)")
	execute_command("ip link add r1-dev-h3 type veth peer name h3-dev-r1")
	execute_command("ip link set r1-dev-h3 netns emu-r1")
	execute_command("ip link set h3-dev-r1 netns emu-h3")
	execute_command("ip netns exec emu-r1 ip addr add 10.0.30.1 dev r1-dev-h3")
	execute_command("ip netns exec emu-h3 ip addr add 10.0.30.2 dev h3-dev-r1")
	execute_command("ip netns exec emu-r1 ifconfig r1-dev-h3 hw ether 00:00:10:00:30:01")
	execute_command("ip netns exec emu-h3 ifconfig h3-dev-r1 hw ether 00:00:10:00:30:02")
	execute_command("ip netns exec emu-r1 ip link set dev r1-dev-h3 up")
	execute_command("ip netns exec emu-h3 ip link set dev h3-dev-r1 up")
	execute_command("ip netns exec emu-r1 ifconfig r1-dev-h3 netmask 255.255.255.0")
	execute_command("ip netns exec emu-h3 ifconfig h3-dev-r1 netmask 255.255.255.0")

	print("\n*** Setting up TC-Netem on interfaces")
	execute_command("ip netns exec emu-h1 tc qdisc replace dev h1-dev-r1 root netem rate 10mbit delay 10ms limit 500 loss 0.0")
	execute_command("ip netns exec emu-r1 tc qdisc replace dev r1-dev-h1 root netem rate 10mbit delay 10ms limit 500 loss 0.0")
	execute_command("ip netns exec emu-r1 tc qdisc replace dev r1-dev-h2 root netem rate 10mbit delay 10ms limit 500 loss 0.0")
	execute_command("ip netns exec emu-h2 tc qdisc replace dev h2-dev-r1 root netem rate 10mbit delay 10ms limit 500 loss 0.0")
	execute_command("ip netns exec emu-r1 tc qdisc replace dev r1-dev-h3 root netem rate 10mbit delay 30ms limit 500 loss 0.0")
	execute_command("ip netns exec emu-h3 tc qdisc replace dev h3-dev-r1 root netem rate 10mbit delay 30ms limit 500 loss 0.0")

if "2" in steps_to_run or "rotue" in steps_to_run:
	print("\n*** Spawning new routing")
	execute_command("echo 200 RT_TABLE_DELIM >> /etc/iproute2/rt_tables")
	execute_command("ip netns exec emu-h1 arp -i h1-dev-r1 -s 10.0.20.2 00:00:10:00:10:02")
	execute_command("ip netns exec emu-h1 ip route add 10.0.20.0/24 via 10.0.10.1 dev h1-dev-r1")
	execute_command("ip netns exec emu-h1 arp -i h1-dev-r1 -s 10.0.30.2 00:00:10:00:10:02")
	execute_command("ip netns exec emu-h1 ip route add 10.0.30.0/24 via 10.0.10.1 dev h1-dev-r1")
	execute_command("ip netns exec emu-h2 arp -i h2-dev-r1 -s 10.0.10.1 00:00:10:00:20:01")
	execute_command("ip netns exec emu-h2 ip route add 10.0.10.0/24 via 10.0.20.2 dev h2-dev-r1")
	execute_command("ip netns exec emu-h3 arp -i h3-dev-r1 -s 10.0.10.1 00:00:10:00:30:01")
	execute_command("ip netns exec emu-h3 ip route add 10.0.10.0/24 via 10.0.30.2 dev h3-dev-r1")
	execute_command("ip netns exec emu-r1 arp -i r1-dev-h1 -s 10.0.10.1 00:00:10:00:10:01")
	execute_command("ip netns exec emu-r1 ip route add 10.0.10.0/24 via 10.0.10.2 dev r1-dev-h1")
	execute_command("ip netns exec emu-r1 arp -i r1-dev-h2 -s 10.0.20.2 00:00:10:00:20:02")
	execute_command("ip netns exec emu-r1 ip route add 10.0.20.0/24 via 10.0.20.1 dev r1-dev-h2")
	execute_command("ip netns exec emu-r1 arp -i r1-dev-h3 -s 10.0.30.2 00:00:10:00:30:02")
	execute_command("ip netns exec emu-r1 ip route add 10.0.30.0/24 via 10.0.30.1 dev r1-dev-h3")

if "3" in steps_to_run or "clean" in steps_to_run:
	print("\n*** Deleting nodes")
	execute_command("ip netns del emu-h1")
	execute_command("ip netns del emu-r1")
	execute_command("ip netns del emu-h2")
	execute_command("ip netns del emu-h3")

#======================================================================================================================#
#=> - End -
#======================================================================================================================#
