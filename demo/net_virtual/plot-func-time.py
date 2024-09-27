
import matplotlib.pyplot as plt

#======================================================================================================================#
#=> - Helpers -
#======================================================================================================================#

def get_time_limits(flow_file):
	times, sizes = [], []
	with open(flow_file, "r") as ptr:
		for line in ptr:
			items = line.split(":") # 3.366:10:1204:o:0
			if len(items) < 1:
				continue
			times.append(float(items[0]))
			if len(items) < 3:
				continue
			sizes.append(int(items[2]))
	start, stop = times[0], times[-1]
	print("*** Throughput: %s Mbps" % format(8 * sum(sizes) / ((stop - start) * 1.0e6), ".2f"))
	return start, stop

def get_func_times(trace_file, start, stop):
	times, directions, funcs = [], [], []
	with open(trace_file, "r") as ptr:
		for line in ptr:
			items = line.split(":")
			if len(items) < 4:
				continue
			time = float(items[0])
			if time < start or time > stop:
				continue
			times.append(time)
			directions.append(items[1])
			funcs.append(items[3])
	return times, directions, funcs

def parse_func_times(times, directions, funcs):
	func_time_sums = {}
	entries = []
	for time, direction, func in zip(times, directions, funcs):
		func = func.strip().strip("\n")
		if direction == "i":
			entries.append((func, float(time)))
		else:
			for entry in entries:
				if entry[0] == func:
					if func not in func_time_sums:
						func_time_sums[func] = 0
					func_time_sums[func] += float(time) - entry[1]
					entries.remove(entry)
	return func_time_sums

def plot_func_times(func_times, tag, start, stop):
	func_times = dict(sorted(func_times.items(), key=lambda item: float(item[1]), reverse=True))
	print("*** Times spent in %s functions between %ss and %ss:" % (tag, format(start, ".2f"), format(stop, ".2f")))
	print("*** Time interval is %ss" % format(stop - start, ".2f"))
	for func_time in func_times: 
		print("    Time in %s == %ss" % (func_time, format(func_times[func_time], ".2f")))
		if float(func_times[func_time]) < 0.01:
			break
	print("")
	#plt.plot(times, directions, funcs)
	#plt.show()

#======================================================================================================================#
#=> - Main -
#======================================================================================================================#

if __name__ == "__main__":
	trace_file = "../trace-func-call-server"
	flow_file = "../tcp-app-rx-flow"
	start, stop = get_time_limits(flow_file)
	times, directions, funcs = get_func_times(trace_file, start, stop)
	func_time_sums = parse_func_times(times, directions, funcs)
	plot_func_times(func_time_sums, "server", start, stop)

	trace_file = "../trace-func-call-client"
	flow_file = "../tcp-app-tx-flow"
	start, stop = get_time_limits(flow_file)
	times, directions, funcs = get_func_times(trace_file, start, stop)
	func_time_sums = parse_func_times(times, directions, funcs)
	plot_func_times(func_time_sums, "client", start, stop)


#======================================================================================================================#
#=> - End -
#======================================================================================================================#
