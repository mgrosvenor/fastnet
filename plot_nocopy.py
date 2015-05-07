#! /usr/bin/python

import math
import sys
import os
import subprocess


def log_out(out): 
    print(out[:-1])


def run_proc(p, wait):
    if not wait: 
        pid = os.fork()
        if pid != 0:
            return

    
    proc = subprocess.Popen(p, shell=True,  stdout=subprocess.PIPE, stderr=subprocess.PIPE)    
    proc.wait()

    log_out("STDERR -- %s\n" % p)
    for line in proc.stderr:
        log_out(line)

    log_out("STDOUT -- %s\n" % p)
    for line in proc.stdout:
        log_out(line)

    if not wait:
	    sys.exit(0)


args = []
for i in [1,10]+ \
         range(100,1000,200) + \
         range(1000,10 *1000, 2000) + \
         range(10 * 1000,100 * 1000, 20 * 1000) + \
         range(100 * 1000, 1000 * 1000, 200 * 1000) + \
         range(1000 * 1000, 5 * 1000 * 1000, 2000 * 1000): 
    packet_count = i 

    outdir  = "experiments/nocopy"

    test_id = "%010i" % (packet_count)

    args.append( "%s/%s.stats %4.2fMB" % (outdir, test_id, i * 2048 / 1024.0 / 1024.0)) 


cmd = "./plot_fast_net.py RD %s nocopy-rd.pdf" % (" ".join(args) )
print cmd
run_proc(cmd,False)

cmd = "./plot_fast_net.py WR %s nocopy-wr.pdf" % (" ".join(args) )
print cmd
run_proc(cmd,True)

cmd = "./plot_fast_net.py APRD %s nocopy-aprd.pdf" % (" ".join(args) )
print cmd
run_proc(cmd,False)

cmd = "./plot_fast_net.py APWR %s nocopy-apwr.pdf" % (" ".join(args) )
print cmd
run_proc(cmd,True)

