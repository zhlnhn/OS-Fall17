import copy
import heapq as hp
from collections import defaultdict

class Memory(object):

    def __init__(self, frames):
        self.mem = ['.'] * frames
        self.last_placed_index = 0

    def __str__(self, frames_per_line=32):
        out_str = '=' * frames_per_line
        for i in range(len(self.mem)//frames_per_line):
            out_str += '\n' + ''.join(self.mem[frames_per_line * i:
                                               frames_per_line * (i+1)])
        out_str += '\n' + '=' * frames_per_line
        return out_str

    def page_table(self):
        Dict=defaultdict(list)
        out_str='PAGE TABLE [page,frame]:\n'
        for i in range(len(self.mem)):
            if self.mem[i]!='.':
                Dict[self.mem[i]].append(i)
        table=sorted(Dict.items())
        for proc,pages in table:
            out_str+=proc+': '
            count = 0
            for i in range(len(pages)):
                out_str+='[%d,%d] '%(i,pages[i])
                count+=1
                if count >9:
                    count =0
                    out_str=out_str[:-1]
                    out_str+='\n'
            out_str=out_str[:-1]
            out_str+='\n'
        out_str=out_str[:-1]
        print(out_str)

    # Place process in next free partition relative to most recently
    #  placed process
    def next_fit(self, process):
        partitions = self.find_free()
        for i in range(len(self.mem)):
            i += self.last_placed_index
            if i >= len(self.mem):
                i -= len(self.mem)
            for partition in partitions:
                if partition[0] <= i and \
                   i + process.req_frames <= partition[0] + partition[1]:
                    self.place(process, i)
                    return partition[0]
        return -1

    # Place process in smallest free partition in which process fits
    def best_fit(self, process):
        smallest_partition = (float("inf"), float("inf"))
        partitions = self.find_free()
        for partition in partitions:
            if process.req_frames <= partition[1] and \
               partition[1] < smallest_partition[1]:
                smallest_partition = partition
        if smallest_partition[0] < float("inf"):
            self.place(process, smallest_partition[0])
            return smallest_partition[0]
        else:
            return -1


    def first_fit(self, process):
        partitions = self.find_free()
        for i in range(len(self.mem)):
            for partition in partitions:
                if partition[0] <= i and \
                   i + process.req_frames <= partition[0] + partition[1]:
                    self.place(process, i)
                    return partition[0]
        return -1

    # Defragment memory
    def defragment(self):
        time = 0
        moved_processes = set()
        last_placed = 0
        for i in range(len(self.mem)):
            if self.mem[i] != '.':
                if last_placed != i:
                    self.mem[last_placed] = self.mem[i]
                    if self.mem[i] not in moved_processes:
                        moved_processes.add(self.mem[i])
                    self.mem[i] = '.'
                    time += 1
                last_placed += 1

        moved_processes = list(moved_processes)
        moved_processes.sort()
        return time, moved_processes


    # Place process at index
    def place(self, process, index):
        for i in range(process.req_frames):
            self.mem[index+i] = process.name
        self.last_placed_index = index + process.req_frames

    # Place process using non-contiguous management
    def non_contiguous(self, process):
        if self.free_space() < process.req_frames:
            return -1
        placed_frames = 0
        for i in range(len(self.mem)):
            if self.mem[i] == '.' and placed_frames < process.req_frames:
                self.mem[i] = process.name
                placed_frames += 1
        return 0

    # Remove process from memory
    def remove(self, process):
        for i in range(len(self.mem)):
            if self.mem[i] == process:
                self.mem[i] = '.'

    # Check total free space (for defragmentation)
    def free_space(self):
        free_space = 0
        for frame in self.mem:
            if frame == '.':
                free_space += 1
        return free_space

    # Find free partitions
    def find_free(self):
        free_partitions = []
        partition_num = -1
        partition_start = -1
        partition_len = 0
        in_partition = False
        i = 0
        for frame in self.mem:
            if in_partition:
                if frame == '.':
                    partition_len += 1
                if frame != '.' or i+1 == len(self.mem):
                    free_partitions.append((partition_start, partition_len))
                    partition_start = -1
                    partition_len = 0
                    in_partition = False
            if not in_partition:
                if frame == '.':
                    in_partition = True
                    partition_num += 1
                    partition_len += 1
                    partition_start = i
            i += 1
        return free_partitions

# Head used to schedule time (from Project 1)
class Time_Schedule(object):

	def __init__(self):
		self.sched_t = []

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


# The main simulation function
def phy_simulation(name, proc):
	pros = copy.deepcopy(proc)

	# Initialize variables
	t = 0
	running = []
	t_sch = Time_Schedule()
	mem = Memory(256)
	# Define the type of algorithm
	if name == "next":
		name = "Contiguous -- Next-Fit"
		algo = mem.next_fit
	elif name == "best":
		name = "Contiguous -- Best-Fit"
		algo = mem.best_fit
	elif name == "first":
		name = "Contiguous -- First-Fit"
		algo = mem.first_fit
	elif name == "non":
		name = "Non-contiguous"
		algo = mem.non_contiguous

	# Add processes arrive time to time schedule
	for i in pros:
		for j in i.sched:
			t_sch.add(j[0])

	print ("time %dms: Simulator started (%s)" % (t, name))

	while True:
		i = 0
		# Remove processes from the frame
		for n in range(len(running)):
			if running[i][1] == t:
				mem.remove(running[i][0])
				print ("time %dms: Process %s removed:" % (t, running[i][0]))
				running.pop(i)
				print (mem)
				if name == "Non-contiguous":
					mem.page_table()
			else:
				i += 1

		# Add processes to the frame
		for i in range(len(pros)):
			# It is the time for the arrival of a process
			if len(pros[i].sched) != 0 and pros[i].sched[0][0] == t:
				print ("time %dms: Process %s arrived (requires %d frames)" %\
					(t, pros[i].name, pros[i].req_frames))
				# Store the run time
				brust_t = pros[i].sched[0][1]
				pros[i].sched.pop(0)
				# If there is not enough free space
				if pros[i].req_frames > mem.free_space():
					print ("time %dms: Cannot place process %s -- skipped!" %\
						(t, pros[i].name))
					print (mem)
					if name == "Non-contiguous":
						#print 'test'
						mem.page_table()
				else:
					# If defragmentation is needed
					if algo(pros[i]) == -1:
						print("time %dms: Cannot place process %s -- starting " \
							"defragmentation" % (t, pros[i].name))
						# Do the defragmentation
						dt, nlist = mem.defragment()
						# Delay the time for everything for defragmentation
						t += dt
						for j in range(len(running)):
							running[j][1] += dt
						for j in range(len(t_sch)):
							t_sch.sched_t[j] += dt
						for j in range(len(pros)):
							for k in range(len(pros[j].sched)):
								pros[j].sched[k][0] += dt
						# Generate the string list of moved processes
						temp = ""
						for j in range(len(nlist)):
							temp += nlist[j]
							if j != len(nlist) - 1:
								temp += ", "
						print("time %dms: Defragmentation complete (moved %d frames: %s)" \
							% (t, dt, temp))
						print( mem)
						if name == "Non-contiguous":
							mem.page_table()
						# Try to add process again after defragmentation
						algo(pros[i])
					# Place the process
					print("time %dms: Placed process %s:" % (t, pros[i].name))
					print(mem)
					if name == "Non-contiguous":
						#print('test')
						mem.page_table()
					t_sch.add(t + brust_t)
					running.append([pros[i].name, t + brust_t])

		# Exit the simulation if there are no more scheduled time
		if len(t_sch) == 0:
			print("time %dms: Simulator ended (%s)" % (t, name))
			break
		# Jump to the next scheduled time
		else:
			dt = t_sch.pop()
			t = dt
