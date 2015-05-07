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
DTYPE        = "i64"
PSIZE        = "2048"
DO_PREFETCH  = 0
packet_size  = 1514
samples      = 10 * 1000 * 1000


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

def run_fast_net(core, wait, test_id):
    test_id = test_id + "core=%02i" % core
    log_out("Running test...\n")
    cmd = "../bin/fast_net --packet-count=%i --packet-size=%i --iterations=%i" % (packet_count, packet_size, iterations) 
    os.chdir("pin") # HACK!!
    pin_cmd = "./pin %i \"%s\" > ../outputs/%s.stats" % (core, cmd,test_id)
    run_proc(pin_cmd, wait)
    os.chdir("../")





def run_test(test_id):
    
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
    run_fast_net(11, True, test_id)
    
    log_out("Calculating Stats\n")
    run_proc("./do_hist.py outputs/%s.stats 1000" % (test_id), True)


#for i in [10, 50, 100, 500, 1000, 5000, 10000, 50000, 100000, 500000, 1000000, 5000000]:
#for i in [ 1000000, 5000000]: #, 100000000, 50000000]:
for NBO in range(0,1):
    for i in range(len(PTYPES) - 1,len(PTYPES)):
	PTYPE = "volatile %s" % PTYPES[i]
	USE_NBO = NBO
	packet_count = 2500 
        packet_count = int(math.pow(2,int(math.log(packet_count,2))))
	iterations =  max(3,samples / packet_count)
	print "Running packet_count=%i for %i iterations ptype=%s" % (packet_count, iterations, PTYPE)
        BO_text = "NBO"
	if NBO == 0:
	    BO_text = "HBO" 

	test_id = "packet-count-%s-%012i-%s" % (str(i),packet_count, BO_text)
        log = open("outputs/%s.log" % test_id, "w")
        run_test(test_id) 




