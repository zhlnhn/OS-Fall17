import copy
import heapq as hp


# A class contains all the information and statistic for processes
class Process(object):

    def __init__(self, proc_id, initial_arrival_time, cpu_burst_time,
                 num_bursts, io_time):
        self.proc_id = proc_id

        self.arrival_time = int(initial_arrival_time)
        self.initial_arrival_time = int(initial_arrival_time)

        self.cur_burst_time = 0
        self.last_burst_time = 0
        self.sch_burst_time = 0
        self.cpu_burst_time = int(cpu_burst_time)
        self.left_num = int(num_bursts)
        self.num_bursts = int(num_bursts)

        self.io_time = int(io_time)

        self.wait_t = 0
        self.end_t = -1
        self.prempt_num = 0

    def __str__(self):
        return self.proc_id

    # The order of the process will depend on arrival time
    def __lt__(self, b):
        return self.arrival_time < b.arrival_time

    # The processes are the same if they have same id
    def __eq__(self, b):
        return self.proc_id == b.proc_id

    # Return a list of information of the process
    def return_info(self):
        outstr = "proc_id: " + str(self.proc_id)
        outstr += "\ninitial_arrival_time: " + str(self.initial_arrival_time)
        outstr += "\ncpu_burst_time: " + str(self.cpu_burst_time)
        outstr += "\nnum_bursts: " + str(self.num_bursts)
        outstr += "\nio_time: " + str(self.io_time)
        outstr += "\n"
        return outstr

    # Get the time left for current process
    def time_left(self):
        return self.cpu_burst_time - self.cur_burst_time


# A heap that used to schedule time
class Time_Schedule(object):

    def __init__(self):
        self.sched_t = []

    def __str__(self):
        return str(self.sched_t)

    # Add to heap if not in the heap
    def add(self, t):
        if t not in self.sched_t:
            hp.heappush(self.sched_t, t)

    # Pop the next value
    def pop(self):
        if len(self.sched_t) > 0:
            return hp.heappop(self.sched_t)
        else:
            return None

    def __len__(self):
        return len(self.sched_t)


def simulation(func, p_list):

    # m = 1                   # Not being used in this program
                            # The number of processors available
    t_cs = 8                # The context switch time

    # Helper function that print the queue
    def printq(procs):
        if len(procs) == 0:
            return "[Q empty]"
        else:
            s = "[Q"
            for i in procs:
                s += " " + str(i)
            s += "]"
            return s

    t = 0
    algo = func()
    sched_t = Time_Schedule()       # The list of time scheduled for simulation
    num_cs = 0                      # The number of context switches

    curp = [None]                   # The running process
    ready = []                      # The list of ready processes
    block = []                      # The list of blocked processes
    procs = copy.deepcopy(p_list)   # The list of unready processes
    ended = []                      # The list of ended processes

    # Print the start of the simulation
    print ("time %dms: Simulator started for %s %s" % (t, algo.name, printq(
        ready)))

    # Sort the list of processes by initial arrive time
    procs.sort()
    # Schedule simulation for all the arrive of processes
    for i in procs:
        sched_t.add(i.initial_arrival_time)

    # Avoid 0 ms in schedule
    sched_t.add(0)
    sched_t.pop()

    # Set context switch
    context_switch = -1
    # Start the body of simulation
    while True:
        # =
        # Process context switch
        if context_switch == t:
            context_switch = -1
            num_cs += 1
            if curp[0] is not None:
                sched_t.add(t + curp[0].sch_burst_time)
                curp[0].sch_burst_time = 0
                print ("time %dms: Process %s started using the CPU %s" \
                    % (t, curp[0], printq(ready)))

        # =
        # Handle the process that finishes the current burst
        if curp[0] is not None and \
                curp[0].cur_burst_time == curp[0].cpu_burst_time:
            curp[0].left_num -= 1
            # Terminated the process after all cpu burst
            if curp[0].left_num == 0:
                curp[0].end_t = t
                ended.append(curp[0])
                print ("time %dms: Process %s terminated %s" % (t, curp[0],
                    printq(ready)))
            # If more to go, add to IO block
            else:
                print ("time %dms: Process %s completed a CPU burst; %d to go" \
                    " %s" % (t, curp[0], curp[0].left_num, printq(ready)))

                end_t = t + curp[0].io_time+4
                curp[0].arrival_time = end_t
                curp[0].cur_burst_time = 0
                curp[0].last_burst_time = 0
                block.append(curp[0])
                sched_t.add(end_t)
                block.sort()
                print ("time %dms: Process %s blocked on I/O until time %dms" \
                    " %s" % (t, curp[0], end_t, printq(ready)))

            # Reset current process
            curp[0] = None
            # Start context switch
            context_switch = t + t_cs / 2
            sched_t.add(context_switch)

        # =
        # Add processes to the queue for there initial arrive
        for i in range(len(procs)):
            if procs[0].initial_arrival_time == t:
                temp = procs.pop(0)
                ready.append(temp)
                algo.sort(ready)
                print ("time %dms: Process %s arrived %s" % (t, temp,
                    printq(ready)))
            else:
                break
        # Add processes to the queue after IO block
        for i in range(len(block)):
            if block[0].arrival_time == t:
                temp = block.pop(0)
                ready.append(temp)
                algo.sort(ready)
                print ("time %dms: Process %s completed I/O %s" % (t, temp,
                    printq(ready)))
            else:
                break

        # =
        # Run the algorithm while not context switch
        oldp = curp[0]
        if context_switch == -1:
            printstr = algo(ready, curp)
            # Print the message from the algorithm
            if printstr is not None:
                print ("time %dms: %s %s" % (t, printstr, printq(ready)))
            # Do context switch if current process changed
            if curp[0] != oldp:
                context_switch = t + t_cs / 2
                sched_t.add(context_switch)
            # Add the new scheduled burst time
            elif curp[0] is not None and curp[0].sch_burst_time != 0:
                sched_t.add(t + curp[0].sch_burst_time)
                curp[0].sch_burst_time = 0

        # =
        # Finish the simulation if there is no scheduled time
        if len(sched_t) == 0:
            break

        # =
        # Change time to next scheduled time
        dif_t = sched_t.pop() - t

        #
        for i in range(len(ready)):
            # Fix the wait time difference for context switch
            # WARNING: This might cause the individual wait time wrong
            if i == 0 and context_switch != -1 and curp[0] is None:
                pass
            else:
                ready[i].wait_t += dif_t

        if context_switch == -1 and curp[0] is not None:
            curp[0].cur_burst_time += dif_t

        t += dif_t
        print("****",t)

    # Print the end of the simulation
    print ("time %dms: Simulator ended for %s" % (t, algo.name))

    return t, ended, num_cs
