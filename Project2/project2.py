import simulation
import sys

class Process(object):

    def __init__(self, line):
        self.name = line[0]
        self.req_frames = int(line[1])
        self.num_bursts = len(line)-2
        self.sched = []
        for i in range(len(line)-2):
            times = line[2+i].split('/')
            times[0] = int(times[0])
            times[1] = int(times[1])
            self.sched.append(times)


if __name__ == "__main__":
    # Argument input error handling
    if len(sys.argv) < 2:
        sys.stderr.write("ERROR: Invalid arguments\n")
        sys.stderr.write("USAGE: ./project2.py <processes-input-file> ")
        sys.exit()

    # Get filenames
    processes_file_str = sys.argv[1]

    # Read input file
    in_f = open(processes_file_str, 'r')
    procs = []
    #in_f.readline()
    for line in in_f:
        line_str = line.strip('\n')
        line_list = line_str.split(' ')
        procs.append(Process(line_list))
    in_f.close()

    simulation.phy_simulation("next", procs)
    print('')
    simulation.phy_simulation("first", procs)
    print('')
    simulation.phy_simulation("best", procs)
    print('')
    simulation.phy_simulation("non", procs)
