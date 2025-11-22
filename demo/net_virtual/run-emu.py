import os
import sys
import time

duration = 30

# sudo ip netns exec emu-h2 ifconfig h2-dev-r1 down
# sudo ip netns exec emu-h1 ifconfig h1-dev-r1 down
# sudo ip netns exec emu-h1 ifconfig h1-dev-r1 up
# sudo ip netns exec emu-h2 ifconfig h2-dev-r1 up
# sudo python3 do_route.py 

# sudo sudo ip netns exec emu-h1 tcpkill host 10.0.20.2
# sudo sudo ip netns exec emu-h1 tcpkill host 10.0.10.1
# tcpkill host 10.0.2.20
# tcpkill host 127.0.0.1

do_server_timeout = False
if len(sys.argv) >= 2:
  do_server_timeout = True

app = "net_call_response/net_call"
app = "net_bulk_transfer/net_bulk"
#app = "net_deserialize_test/net_deserialize"

prot_server, error_server = 2, 0
prot_client, error_client = 2, 0
paths = ["../build/%s" %(app), "../build/%s" %(app)]
args = ["-s --prot %d  --error %d" %(prot_server, error_server), "--prot %d  --error %d" %(prot_client, error_client)]
addrs = ["10.0.10.1", "10.0.20.2"]
intfs = ["h2-dev-r1", "h1-dev-r1"]
contexts = ["ip netns exec emu-h2", "ip netns exec emu-h1"]

timeout = "timeout -k %s %s" %(str(duration), str(duration))
for c, i in zip(contexts, intfs): 
  cmd = "tcpdump -i %s -w ../%s.pcap -U" %(i, i)
  run_command = "%s %s xterm -hold -e '%s' &" %(c, timeout, cmd)
  print("Running TCP dump on host interface ..." )
  print("Command: '%s'" %(run_command))
  os.system(run_command)
  
for path, addr, context, arg in zip(paths, addrs, contexts, args): 
  run_command = "%s xterm -hold -e '%s %s --addr %s' &" %(context, path, arg, addr)
  print("Running server executable in a new xterm window...\n")
  print("Command: '%s'" %(run_command))
  os.system(run_command)
  time.sleep(2)
