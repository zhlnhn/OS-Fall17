
#!/usr/bin/python
"""

CSCI-4210 Operating System
Project 1
Boning Dong and Sam Atkinson

The program take a input filename and output filename.
Then, simulate the CPU scheduling for the processes given
in the input file.
The key step of the simulation will be printed through
stdout, and the summary of the statistics will be stored in
a text file with the given output filename.
FCFS, SJF and RR will be simulated.

"""

import sys
from simulation import *


# Print out the summary of certain algorithm
def summary(result, func_name):
    # Initiate all the variables
    total_burst_num = 0
    avg_burst = 0
    avg_wait = 0
    avg_turn = 0
    c_switches = result[2] / 2
    preemptions = 0
    # Calculate average CPU burst time etc.
    for p in result[1]:
        total_burst_num += p.num_bursts
        avg_burst += p.cpu_burst_time * p.num_bursts
        avg_wait += p.wait_t
        avg_turn += p.end_t - p.initial_arrival_time -\
            p.io_time * (p.num_bursts - 1)
        preemptions += p.prempt_num
    avg_burst /= float(total_burst_num)
    avg_wait /= float(total_burst_num)
    avg_turn /= float(total_burst_num)
    # Output the solution in a string
    out_str = "Algorithm %s" % (func_name)
    out_str += "\n-- average CPU burst time: %3.2f ms" % (avg_burst)
    out_str += "\n-- average wait time: %3.2f ms" % (avg_wait)
    out_str += "\n-- average turnaround time: %3.2f ms" % (avg_turn)
    out_str += "\n-- total number of context switches: %d" % (c_switches)
    out_str += "\n-- total number of preemptions: %d\n" % (preemptions)
    return out_str


# Algorithm class for FCFS
class FCFS:
    def __init__(self):
        self.name = "FCFS"

    # Leave the process list as it original order (arrival time)
    def sort(self, p_list):
        pass

    # Set the first one in the sorted list as running process
    def __call__(self, p_list, curp):
        if curp[0] is None and len(p_list) != 0:
            curp[0] = p_list.pop(0)
            curp[0].sch_burst_time = curp[0].time_left()
        return None


# Algorithm class for SJF
class SJF:
    def __init__(self):
        self.name = "SJF"

    # Sort according to CPU burst time
    def sort(self, p_list):
        p_list.sort(key=lambda process: process.cpu_burst_time)

    # Set the first one in the sorted list as running process
    def __call__(self, p_list, curp):
        if curp[0] is None and len(p_list) != 0:
            curp[0] = p_list.pop(0)
            curp[0].sch_burst_time = curp[0].time_left()
        return None


# Algorithm class for RR
class RR:
    def __init__(self):
        self.name = "RR"
        self.t_slice = 70       # The aforementioned time slice

    # Leave the process list as it original order (arrival time)
    def sort(self, p_list):
        pass

    def __call__(self, p_list, curp):
        if curp[0] is None and len(p_list) != 0:
            curp[0] = p_list.pop(0)
            curp[0].sch_burst_time = min(self.t_slice, curp[0].time_left())
            return None
        elif curp[0] is not None and curp[0].cur_burst_time != \
                curp[0].last_burst_time and (curp[0].cur_burst_time \
                % self.t_slice) == 0:
            if len(p_list) == 0:
                curp[0].sch_burst_time = min(self.t_slice, curp[0].time_left())
                return "Time slice expired; no preemption because ready" \
                    " queue is empty"
            else:
                curp[0].last_burst_time = curp[0].cur_burst_time
                curp[0].prempt_num += 1
                p_list.append(curp[0])
                curp[0] = None
                return "Time slice expired; process %s preempted with %dms" \
                    " to go" % (p_list[-1], p_list[-1].time_left())


if __name__ == "__main__":
    # Argument input error handling
    if len(sys.argv) < 3:
        sys.stderr.write("ERROR: Invalid arguments\n")
        sys.stderr.write("USAGE: ./main.py <input-file> <output-file>\n")
        sys.exit()

    # Read the input and output filename
    input_file_str = sys.argv[-2]
    output_file_str = sys.argv[-1]

    # Read the input file
    p_list = []
    f = open(input_file_str, "r")
    for line in f:
        # Skip comments
        if line[0] == "#":
            pass
        # Read each process
        else:
            process_info = line.split("|")
            if len(process_info) != 5:
                sys.stderr.write("ERROR: Invalid input file format\n")
                sys.exit()
            p_list.append(Process(process_info[0], process_info[1],
                                  process_info[2], process_info[3],
                                  process_info[4]))
    f.close()

    # Run each algorithm
    FCFS_result = summary(simulation(FCFS, p_list), "FCFS")
    print ("")
    SJF_result  = summary(simulation(SJF, p_list), "SJF")
    print ("")
    RR_result   = summary(simulation(RR, p_list), "RR")

    # Write simulation results to file
    out_f = open(output_file_str, "w+")
    out_f.write(FCFS_result)
    out_f.write(SJF_result)
    out_f.write(RR_result)
    out_f.close()
