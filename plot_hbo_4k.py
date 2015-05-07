#! /usr/bin/python

import math
import sys
import os
import subprocess


PTYPES = [  "eth_ip_udp_head_t", "eth_32ip_udp_head_t", "eth_64ip_udp_head_t", "eth64_64ip64_64udp_head_t", "eth6464_64ip64_64udp_head_t" ]

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
i = 4096
#for i in range(4096,4096 * 2 + 1, 4096):
typ= 0
for PTYPE in PTYPES:

    packet_count = i 
    outdir  = "experiments/layout_hbo_4k"
    test_id = "%02i-%010i" % (typ,packet_count)
    args.append( "%s/%s.stats %4.2fMB-%02i" % (outdir, test_id, packet_count * 2048 / 1024.0 / 1024.0, typ )) 
    typ += 1

cmd = "./plot_fast_net.py RD %s layout-hbo-4k-rd.pdf" % (" ".join(args) )
print cmd
run_proc(cmd,False)

cmd = "./plot_fast_net.py WR %s layout-hbo-4k-wr.pdf" % (" ".join(args) )
print cmd
run_proc(cmd,True)

