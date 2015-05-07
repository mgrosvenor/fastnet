#! /usr/bin/python

import sys
import os
import numpy

if len(sys.argv) < 2:
    print "Usage: do_hist <input_file>"
    sys.exit(-1)

wr_nums = []
rd_nums = []
parse_errors = 0
range_errors = 0
print "Loading data..."
line_num = 0
for line in open(sys.argv[1]):
    try:
    #WR 0 3400 0.9994 12.1195
	(typ,idx,cyc,us,gbs) = line.split(" ")
        num = float(cyc)
    except:
        print "Parse error on line %i: \"%s\"" % (line_num, line[:-1])
        parse_errors += 1
        line_num += 1
        continue
    #num *= 1000
    if num < 0.0000000000001 or num > 100000000000:
        range_errors += 1
        line_num += 1
        continue

    if typ == "WR":
        wr_nums.append(num)
    
    if typ == "RD":
        rd_nums.append(num)

    line_num += 1

def do_nums(nums):
    print "Processing data..."
    min = numpy.min(nums)
    max = numpy.max(nums)
    print min,max
    percent = [0,1,10,25,50,75,90,99,100]

    print "Total samples %i" % len(nums)
    for p in percent:
        print "%3f%% - %f" % (p, numpy.percentile(nums,p * 1.0))
        #print "%i" % (numpy.percentile(nums,p * 1.0))
    print "Range = %i" % (numpy.max(nums) - numpy.min(nums))
    print "Mean= %f" % (numpy.mean(nums))


print "Parse errors found: %i" % (parse_errors)
print "Range errors found: %i" % (range_errors)

if len(wr_nums) < 100 or len(rd_nums) < 100:
    print "Fatal Error! Not enough points (%i)" % len(wr_nums)
    print "Fatal Error! Not enough points (%i)" % len(rd_nums)
    sys.exit(-1)

print "Sorting data..."
wr_nums.sort()


print "\nWRITE RESULTS:"
do_nums(wr_nums)

print "Sorting data..."
rd_nums.sort()

print "\nREAD RESULTS:"
do_nums(rd_nums)
