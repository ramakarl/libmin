import os
import time
import sys


do_server_timeout = False
if len(sys.argv) >= 2:
  do_server_timeout = True

duration = 10
timeout = "timeout -k %s %s" %(str(duration), str(duration)) 
cmd = "tcpdump -i lo -w ../lo.pcap" 
run_command = "%s xterm -hold -e '%s' &" %(timeout, cmd)
print("Running TCP dump on host interface ..." )
print("Command: '%s'" %(run_command))
os.system(run_command)

paths = ["../build/ndserver/nd_server", "../build/ndclient/nd_client"]
for path in paths[:1]: 
  run_command = "xterm -hold -e '%s' &" %(path)
  print("Running server executable in a new xterm window...\n")
  print("Command: '%s'" %(run_command))
  os.system(run_command)
  time.sleep(2)
  
if 1:  
  run_command = "%s" %(paths[1])
  print("Running server executable in a new xterm window...\n")
  print("Command: '%s'" %(run_command))
  os.system(run_command)
  time.sleep(2)  
  
if 0:  
  run_command = "gdb %s" %(paths[1])
  os.system(run_command)
