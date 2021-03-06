#! /usr/bin/python

import math
import sys
import os
import subprocess


#PTYPES = [  "eth_ip_udp_head_t", "ip_udp_head_t", "eth_32ip_udp_head_t", "eth_64ip_udp_head_t", "eth64_64ip64_64udp_head_t", "eth6464_64ip64_64udp_head_t" ]
#PTYPES = [  "eth_ip_udp_head_t", "eth_32ip_udp_head_t", "eth_64ip_udp_head_t", "eth64_64ip64_64udp_head_t", "eth6464_64ip64_64udp_head_t" ]
#PTYPE        = "volatile eth_ip_udp_head_t" 
PTYPE        = "volatile eth6464_64ip64_64udp_head_t" 


USE_ETHER    = "1"
USE_NBO      = "0"
DO_COPY      = "0"
READ_DATA    = "1"
DTYPE        = "i64"
PSIZE        = "8096"
DO_PREFETCH  = 1
packet_size  = 8096
samples      = 1 * 1000 * 1000

def log_out(out): 
    print(out[:-1])
    log.write(out)


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

def run_fast_net(outdir, core, wait, test_id):
    test_id = test_id + "C%02i" % core
    log_out("Running test...\n")
    cmd = "../bin/fast_net --packet-count=%i --packet-size=%i --iterations=%i" % (packet_count, packet_size, iterations) 
    os.chdir("pin") # HACK!!
    pin_cmd = "./pin %i \"%s\" > ../%s/%s.stats" % (core, cmd, outdir, test_id)
    run_proc(pin_cmd, wait)
    os.chdir("../")





def run_test(outdir, test_id, extra):
    cmd = "./build.sh %s" % " ".join( [ "\"" + str(x) + "\"" for x in [PTYPE,USE_ETHER,USE_NBO,DO_COPY,READ_DATA,DTYPE,PSIZE, DO_PREFETCH]] )
    log_out("Running Compiler \"%s\"\n" % cmd)
    run_proc(cmd, True)

    for i in range( 6, 4 + extra - 1, 2):
        run_fast_net(outdir, i,  False, test_id)
    
    run_fast_net(outdir, 4, True, test_id)
    
    #log_out("Calculating Stats\n")
    #run_proc("./do_hist.py %s/%s.stats" % (outdir,test_id), True)


outdir  = "experiments/layout_hbo_8k_64_scale"
try:
    os.makedirs(outdir)
except:
    None

for i in range(4096,4096 * 2 + 1, 4096):
        #[1,10] + \
        # range(100,1000,200) + \
        # range(1000,10 *1000, 2000) + \
        # range(10 * 1000,100 * 1000, 20 * 1000) + \
        # range(100 * 1000, 1000 * 1000, 200 * 1000) + \
        # range(1000 * 1000, 5 * 1000 * 1000, 2000 * 1000): 
    
    for scale in range(0,6):
        
        packet_count = i 
        #packet_count = int(math.pow(2,int(math.log(packet_count,2))))
        iterations   =  max(2,samples / packet_count)
        print "Running packet_count=%i for %i iterations ptype=%s" % (packet_count, iterations, PTYPE)

        test_id = "S%02i-%010i" % (scale,i)
        log     = open("%s/%s.log" % (outdir, test_id), "w")
        run_test(outdir, test_id, scale) 


        log_out("Resting for a bit\n")
        run_proc("sleep 4", True)
