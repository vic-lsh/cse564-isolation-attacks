#!/bin/python3
import sys

free_output = sys.stdin.readlines()
avail_list = [x for x in free_output[1].split(" ") if x]
#print(avail_idx)
avail = avail_list[6]
avail = int(avail)
#sys.stderr.write("Parsed avail: " + str(avail) + "\n")
print(avail * 3 // 1024 // 1024)
print(int(avail * .8 / 2))
