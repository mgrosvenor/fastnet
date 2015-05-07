#!/usr/bin/python
# -*- coding: utf-8 -*-
# Simple script which takes a file with one packet latency (expressed as a
# signed integer) per line and plots a trivial histogram.

import math
import sys, re
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from utils import *
from matplotlib import pylab
from scipy.stats import scoreatpercentile

pkt_size = 1500
train_length = 50

# @author: Aaron Blankstein, with modifications by Malte Schwarzkopf
class boxplotter(object):
    def __init__(self, median, top, bottom, whisk_top=None,
                 whisk_bottom=None, extreme_top=None):
        self.median = median
        self.top = top
        self.bott = bottom
        self.whisk_top = whisk_top
        self.whisk_bott = whisk_bottom
        self.extreme_top = extreme_top
    def draw_on(self, ax, index, box_color = "blue",
                median_color = "red", whisker_color = "black"):
        width = .7
        w2 = width / 2
        ax.broken_barh([(index - w2, width)],
                       (self.bott,self.top - self.bott),
                       facecolor="white",edgecolor=box_color)
        ax.broken_barh([(index - w2, width)],
                       (self.median,0),
                       facecolor="white", edgecolor=median_color)
        if self.whisk_top is not None:
            ax.broken_barh([(index - w2, width)],
                           (self.whisk_top,0),
                           facecolor="white", edgecolor=whisker_color)
            ax.broken_barh([(index , 0)],
                           (self.whisk_top, self.top-self.whisk_top),
                           edgecolor=box_color,linestyle="dashed")
        if self.whisk_bott is not None:
            ax.broken_barh([(index - w2, width)],
                           (self.whisk_bott,0),
                           facecolor="white", edgecolor=whisker_color)
            ax.broken_barh([(index , 0)],
                           (self.whisk_bott,self.bott-self.whisk_bott),
                           edgecolor=box_color,linestyle="dashed")
        #if self.extreme_top is not None:
        #    ax.scatter([index], [self.extreme_top], marker='*')

def percentile_box_plot(ax, data, indexer=None, box_top=75,
                        box_bottom=25,whisker_top=99,whisker_bottom=1):
    if indexer is None:
        indexed_data = zip(range(1,len(data)+1), data)
    else:
        indexed_data = [(indexer(datum), datum) for datum in data]
    def get_whisk(vector, w):
        if w is None:
            return None
        return scoreatpercentile(vector, w)

    for index, x in indexed_data:
        bp = boxplotter(scoreatpercentile(x, 50),
                        scoreatpercentile(x, box_top),
                        scoreatpercentile(x, box_bottom),
                        get_whisk(x, whisker_top),
                        get_whisk(x, whisker_bottom),
                        scoreatpercentile(x, 100))
        bp.draw_on(ax, index)


#def worst_case_approx(setups, trainlength, plength):
#  base_worst = 0.588
#  packet_time = (plength + 24.0) * 8.0 / 10.0 / 1000.0
#  tmp = [x * (packet_time * trainlength) for x in setups]
#  return [x + base_worst for x in tmp]

######################################
if len(sys.argv) < 2:
  print "usage: plot_switch_experiment.py <RD | WR> <input dir1> <input1 label> " \
        "<input dir2> <input2 label> ... <output file>"
  sys.exit(1)

paper_mode = False

if paper_mode:
  set_paper_rcs()

# arg processing
if (len(sys.argv) - 1) % 2 == 0:
  # odd number of args, have output name
  outname = sys.argv[-1]
  print "Output name specified: %s" % (outname)
else:
  print "Please specify an output name!"
  sys.exit(1)

typ = sys.argv[1]
if typ != "RD" and typ != "WR":
    print "Please make sure the type is RD (read) or WR (write)"

inputdirs = []
labels = []
for i in range(2, len(sys.argv)-2, 2):
  inputdirs.append(sys.argv[i])
  labels.append(sys.argv[i+1])


# parsing
data = []
for indir in inputdirs:
  negs_ignored = 0
  parse_errors = 0
  ds = []
  count = 0
  print "Working on %s" % indir
  for line in open(indir):
  #for line in open(indir).readlines():
    if line.strip() == "":
      continue

    val = 0
    try:
        (line_typ,idx,cycles,us,gpbs) = line.split(" ")
        cycles = float(cycles)
        val = 1514 * 8 / cycles

	#val = float(cycles)# / 1000.0  
    except:
        parse_errors += 1
  
    if line_typ != typ:
      continue

    if val > 0:
      ds.append(val)
    else:
      negs_ignored += 1
    
#    if count > 1000 * 1000:
#	    break 
#    count += 1
  data.append(ds)

  print "Ignored %d negative latency values!" % (negs_ignored)
  print "Parse errors = %i" % (parse_errors)
  print "Data vals count = %i" % len(ds)

# plotting
#fig = plt.figure(figsize=(2.33,1.66))
#plt.rc("font", size=7.0)
fig, ax = plt.subplots()
pos = np.array(range(len(data)))+1
bp = percentile_box_plot(ax, data)

# worst-case analytical approximation
#plt.plot(range(1, len(data)+1),
#         worst_case_approx(range(0, len(data)), train_length, pkt_size),
#         ':', color='r', label="modelled worst case", lw=1.0)

plt.legend(loc=2, frameon=False)

ax.set_xlabel('Size of packet buffer')
ax.set_ylabel('Cycles')
plt.ylim(ymin=0) #, ymax=2500)
#lables = [ "%4.3fMB" % (x * 2048.0 / 1024.0 / 1024) for x in  [0, 10, 50, 100, 500, 1000, 5000, 10000, 50000, 100000, 500000, 1000000, 5000000] ] 
#plt.xticks( range(0, len(inputdirs)), lables + lables[1:] + lables[1:], rotation=75) 
plt.xticks( range(0,len(inputdirs)), [""] + labels, rotation=90) 


#plt.setp(bp['whiskers'], color='k',  linestyle='-' )
#plt.setp(bp['fliers'], markersize=3.0)
plt.savefig(outname, format="pdf", bbox_inches='tight')
