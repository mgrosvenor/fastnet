#! /usr/bin/python

import math
import sys
import os
import subprocess


#PTYPES = [  "eth_ip_udp_head_t", "ip_udp_head_t", "eth_32ip_udp_head_t", "eth_64ip_udp_head_t", "eth64_64ip64_64udp_head_t", "eth6464_64ip64_64udp_head_t" ]
PTYPES = [  "eth_ip_udp_head_t", "eth_32ip_udp_head_t", "eth_64ip_udp_head_t", "eth64_64ip64_64udp_head_t", "eth6464_64ip64_64udp_head_t" ]
#PTYPE        = "volatile eth_ip_udp_head_t" 
#PTYPE        = "volatile eth6464_64ip64_64udp_head_t" 


USE_ETHER    = "1"
USE_NBO      = "1"
DO_COPY      = "0"
READ_DATA    = "1"
DTYPE        = "i32"
PSIZE        = "2048"
DO_PREFETCH  = 1
packet_size  = 1514
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
    #test_id = test_id + "core=%02i" % core
    log_out("Running test...\n")
    cmd = "../bin/fast_net --packet-count=%i --packet-size=%i --iterations=%i" % (packet_count, packet_size, iterations) 
    os.chdir("pin") # HACK!!
    pin_cmd = "./pin %i \"%s\" > ../%s/%s.stats" % (core, cmd, outdir, test_id)
    run_proc(pin_cmd, wait)
    os.chdir("../")





def run_test(outdir, test_id):
    cmd = "./build.sh %s" % " ".join( [ "\"" + str(x) + "\"" for x in [PTYPE,USE_ETHER,USE_NBO,DO_COPY,READ_DATA,DTYPE,PSIZE, DO_PREFETCH]] )
    log_out("Running Compiler \"%s\"\n" % cmd)
    run_proc(cmd, True)
#    run_fast_net(4,  False, test_id)
#    run_fast_net(5,  False, test_id)
#    run_fast_net(6,  False, test_id)
#    run_fast_net(7,  False, test_id)
#    run_fast_net(8,  False, test_id)
#    run_fast_net(9,  False, test_id)
#    run_fast_net(10, False, test_id)
    run_fast_net(outdir, 7, True, test_id)
    
    #log_out("Calculating Stats\n")
    #run_proc("./do_hist.py %s/%s.stats" % (outdir,test_id), True)


outdir  = "experiments/layout"
try:
    os.makedirs(outdir)
except:
    None

for i in range(4096,4096 * 2 + 1, 4096): 
         #[1,10] + \
         #range(100,1000,200) + \
         #range(10 * 1000,100 * 1000, 20 * 1000) + \
         #range(100 * 1000, 1000 * 1000, 200 * 1000) + \
         #range(1000 * 1000, 5 * 1000 * 1000, 2000 * 1000): 
    typ= 0
    for PTYPE in PTYPES:
        
        packet_count = i 
        #packet_count = int(math.pow(2,int(math.log(packet_count,2))))
        iterations   =  max(2,samples / packet_count)
        print "Running packet_count=%i for %i iterations ptype=%s" % (packet_count, iterations, PTYPE)

        test_id = "%02i-%010i" % (typ,i)
        log     = open("%s/%s.log" % (outdir, test_id), "w")
        run_test(outdir, test_id) 

        typ += 1

        log_out("Resting for a bit\n")
        run_proc("sleep 4", True)
