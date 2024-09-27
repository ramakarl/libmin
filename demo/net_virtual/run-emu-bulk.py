import os
import sys
import time

ref_time = time.time()
duration = 10
port = 21000
paths = ["app-tcp-rx", "app-tcp-tx"]
addr = ["10.0.20.2", "10.0.10.1"]
addr = ["10.0.20.2", "10.0.20.2"]
intfs = ["h2-dev-r1", "h1-dev-r1"]
contexts = ["ip netns exec emu-h2", "ip netns exec emu-h1"]
timeout = "timeout -k %s %s" %(str(duration), str(duration))

os.system("./compile")

for c, i in zip(contexts, intfs): 
  cmd = "tcpdump -i %s -w ../%s.pcap -U" %(i, i)
  run_command = "%s %s xterm -hold -e '%s' &" %(c, timeout, cmd)
  print("Running TCP dump on host interface ..." )
  print("Command: '%s'\n" %(run_command))
  os.system(run_command)
  
for p, a, c in zip(paths, addr, contexts): 
  p = "%s %f %s %d %d %s" %(p, ref_time, "bbr", duration - 1, port, a)
  run_command = "%s xterm -hold -e './%s' &" %(c, p)
  print("Running server executable in a new xterm window...")
  print("Command: '%s'\n" %(run_command))
  os.system(run_command)
  time.sleep(1)

for p in paths:
	if os.path.exists(p):
		os.remove(p)
