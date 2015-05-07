#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>

#include "../deps/chaste/chaste.h"
#include "../deps/chaste/options/options.h"

#include "packet_types.h"
#include <netinet/in.h>

#include <sys/mman.h>

#include "../deps/chaste/data_structs/array/array.h"

#define xstr(a) #a
#define str(a) xstr(a)




#ifndef externdefs
    #define PTYPE volatile eth_ip_udp_head_t
    //#define PTYPE volatile eth6464_64ip64_64ud p_head_t
    #define USE_ETHER 0 //Process packets with ethernet headers
    #define USE_NBO 0 //Process packets with network byte ordering
    #define DO_COPY 1
    #define READ_DATA 1
    #define DTYPE i64
    #define PSIZE 2048
    #define DO_PREFETCH 1
#endif

typedef struct{
    char data[PSIZE];
}  __attribute__((__packed__)) pbuf_t;


#define MAGIC_SRC_MAC        1
#define MAGIC_DST_MAC        1
#define MAGIC_ETHER_TYPE_NBO 1
#define MAGIC_IPV4_VER       1
#define MAGIC_IPHL           1
#define MAGIC_UDP_PROTO      1
#define MAGIC_IP_CSUM        1
#define MAGIC_DST_IP         1
#define MAGIC_DST_PORT       1
#define MAGIC_UDP_CSDUM      1
#define MAGIC_ECN            1
#define MAGIC_DSCP           1
#define MAGIC_IP_ID          1
#define MAGIC_FRAG_OFF_FLAGS 1
#define MAGIC_IP_TTL         1
#define MAGIC_IP_HDR_CSUM    1
#define MAGIC_SRC_IP         1
#define MAGIC_SRC_PORT       1
#define MAGIC_DATA           1



USE_CH_LOGGER(CH_LOG_LVL_INFO,true,ch_log_tostderr,NULL);
USE_CH_OPTIONS;

static struct {
	//Logging options
	bool log_no_colour;
	bool log_stdout;
	bool log_stderr;
	bool log_syslog;
	char* log_filename;
	i64 log_verbosity;

	//Application settings
	i64 packet_count;
	i64 packet_size;
	i64 iterations;
} options;

static struct {
	uint64_t ipv4_bytes;
	uint64_t ipv4_packets;
	uint64_t udp_bytes;
	uint64_t udp_packets;
} stats = {0};


#define SEC2US (1000 * 1000)
#define SEC2NS (1000 * 1000 * 1000)
static uint64_t cpu_frequency_hz    = 0;
static ch_array_t* packets = NULL;
static ch_array_t* cystats = NULL;
static ch_array_t* appstats = NULL;
static i64 cystats_idx = 0;
volatile char buff[PSIZE];


static __inline__  uint64_t get_cycles(void)
{
//    unsigned int eax, ebx, ecx, edx;
//    __asm__("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (0x80000007));

     uint32_t a, d;
     __asm__("rdtsc" : "=a" (a), "=d" (d));

     return (((uint64_t)a) | (((uint64_t)d) << 32));
}



static void init_data();
static int run_sim();

int main(int argc, char** argv)
{
    //Q2PC Logging
    ch_opt_addbi(CH_OPTION_FLAG,     'n', "log-no-colour",  "Turn off colour log output",     &options.log_no_colour, false);
    ch_opt_addbi(CH_OPTION_FLAG,     '0', "log-stdout", "Log to standard out", 	 	&options.log_stdout, 	false);
    ch_opt_addbi(CH_OPTION_FLAG,     '1', "log-stderr", "Log to standard error [default]", 	&options.log_stderr, 	false);
    ch_opt_addsi(CH_OPTION_OPTIONAL, 'F', "log-file",   "Log to the file supplied",	&options.log_filename, 	NULL);
    ch_opt_addii(CH_OPTION_OPTIONAL, 'v', "log-level",  "Log level verbosity (0 = lowest, 6 = highest)",  &options.log_verbosity, CH_LOG_LVL_INFO);


    ch_opt_addii(CH_OPTION_OPTIONAL, 'p', "packet-count",  "Packet count ",  &options.packet_count, 1024);
    ch_opt_addii(CH_OPTION_OPTIONAL, 's', "packet-size",   "Packet size ",  &options.packet_size, 1514);
    ch_opt_addii(CH_OPTION_OPTIONAL, 'i', "iterations",    "iterations ",  &options.iterations, 1000);

    //Parse it all up
    ch_opt_parse(argc,argv);

    if(options.packet_size > PSIZE){
        ch_log_fatal("Packet size must be less than or equal to %lu\n", PSIZE);
    }

    //Configure logging
    ch_log_settings.filename    = options.log_filename;
    ch_log_settings.use_color   = !options.log_no_colour;
    ch_log_settings.log_level   = MAX(0, MIN(options.log_verbosity, CH_LOG_LVL_DEBUG3)); //Put a bound on it

    i64 log_opt_count = 0;
    log_opt_count += options.log_stderr ?  1 : 0;
    log_opt_count += options.log_stdout ?  1 : 0;
    log_opt_count += options.log_syslog ?  1 : 0;
    log_opt_count += options.log_filename? 1 : 0;

    //Too many options selected
    if(log_opt_count > 1){
        ch_log_fatal("Q2PC: Can only log to one format at a time, you've selected the following [%s%s%s%s]\n ",
                options.log_stderr ? "std-err " : "",
                options.log_stdout ? "std-out " : "",
                options.log_syslog ? "system log " : "",
                options.log_filename ? "file out " : ""
        );
    }

    //No option selected, use the default
    if(log_opt_count == 0){
        options.log_stderr = true;
    }

    //Configure the options as desired
    if(options.log_stderr){
        ch_log_settings.output_mode = ch_log_tostderr;
    } else if(options.log_stdout){
        ch_log_settings.output_mode = ch_log_tostdout;
    } else if(options.log_syslog){
        ch_log_settings.output_mode = ch_log_tosyslog;
    } else if (options.log_filename){
        ch_log_settings.output_mode = ch_log_tofile;
    }




    uint64_t ts_start_us  = 0;
    uint64_t ts_end_us    = 0;
    uint64_t start_cycles = 0;
    uint64_t end_cycles   = 0;

    int has_invariant_tsc = 0;
    unsigned int eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (0x80000007));
    has_invariant_tsc = edx & (1 << 8);

    if(!has_invariant_tsc){
        ch_log_fatal("Cannot run on machines without an invaraint TSC. Terminating\n");
        return -1;
    }


    struct timeval ts;
    ch_log_info("Calculating CPU speed\n");
    gettimeofday(&ts,NULL);
    ts_start_us = (ts.tv_usec + ts.tv_sec * SEC2US );
    start_cycles = get_cycles();
    while(1){
        gettimeofday(&ts,NULL);
        ts_end_us = (ts.tv_usec + ts.tv_sec * SEC2US );
        if((ts_end_us - ts_start_us) >= 2 * SEC2US){
            end_cycles = get_cycles();
            break;
        }
    }
    cpu_frequency_hz = (end_cycles - start_cycles) / 2;
    ch_log_info("CPU is running at %0.2lfGhz\n", (double)cpu_frequency_hz / 1000 / 1000 / 1000);

    ch_log_info("Packet buffers are %lu\n", PSIZE);
    ch_log_info("Packet size  =%i\n", options.packet_size);
    ch_log_info("Packet count =%i\n", options.packet_count);
    ch_log_info("Iterations   =%i\n", options.iterations);

    ch_log_info("Header size is %lu, using type \"%s\"\n", sizeof(PTYPE), str(PTYPE));
    ch_log_info("Use Ethernet=%i\n", USE_ETHER);
    ch_log_info("Use NBO     =%i\n", USE_NBO);
    ch_log_info("Do copy     =%i\n", DO_COPY );
    ch_log_info("Read data   =%i\n", READ_DATA);
    ch_log_info("Do prefetch =%i\n", DO_PREFETCH);
    ch_log_info("Data size is %lu, using type \"%s\"\n", sizeof(DTYPE), str(DTYPE));


    init_data();
    return run_sim();
}





static void init_data()
{
    int ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    if(ret){
        ch_log_warn("Could not lock memory. %s\n", strerror(errno));
    }

    //Make a new page aligned array
    packets = ch_array_new(options.packet_count,sizeof(pbuf_t),NULL);
    if(!packets){
        ch_log_fatal("Could not allocate array memory\n");
    }
    ch_log_info("Allocated %luMB of packet memory\n", options.packet_count * sizeof(pbuf_t) / 1024 /1024);

    cystats = ch_array_new(options.packet_count * options.iterations,sizeof(i64),NULL);
    if(!cystats){
        ch_log_fatal("Could not allocate array memory\n");
    }

    appstats = ch_array_new(options.packet_count * options.iterations,sizeof(i64),NULL);
    if(!cystats){
        ch_log_fatal("Could not allocate array memory\n");
    }


    long write_counts_nbo[8] = {0}; //network byte order
    long write_counts_hbo[8] = {0}; //host byte order
    u64 cycles_min_idx = 0;
    u64 cycles_min = ~0;
    u64 cycles_max = 0;
    u64 cycles_total = 0;


    for(int iter = 0; iter < options.iterations; iter++ ){
        for(int i = 0; i < options.packet_count; i++){
           u64 cycles_start = get_cycles();
           u64 app_cycles_start = 0;
           pbuf_t* pbuf = ((pbuf_t*)packets->first + i);
           const u64 data_len = options.packet_size - sizeof(PTYPE);
           PTYPE* header = (PTYPE*)pbuf->data;

            #if(DO_PREFETCH)
               __builtin_prefetch ((pbuf_t*)packets->first + i + 1,1);
            #endif
            __builtin_prefetch ((u64*)cystats->first + cystats_idx + 1,1);



           #if(USE_ETHER)
               const int64_t src_mac = MAGIC_SRC_MAC;
               const int64_t dst_mac = MAGIC_DST_MAC;
               memcpy((void*)&header->dst_mac_raw, &src_mac,sizeof(header->dst_mac_raw));
               memcpy((void*)&header->src_mac_raw, &dst_mac,sizeof(header->src_mac_raw));
               header->eth_type = MAGIC_ETHER_TYPE_NBO; // + ( i % 250 == 0 ? 1 : 0);
           #endif

           header->ver = MAGIC_IPV4_VER;
           header->ihl = MAGIC_IPHL;
           header->ecn = MAGIC_ECN;
           header->dscp = MAGIC_DSCP;

           #if(USE_NBO)
               header->total_len = htons(64);
           #else
               header->total_len = 64;
           #endif

           header->id = MAGIC_IP_ID;
           header->frag_off_flags = MAGIC_FRAG_OFF_FLAGS;
           header->ttl = MAGIC_IP_TTL;
           header->hdr_csum = MAGIC_IP_HDR_CSUM;

           header->protocol = MAGIC_UDP_PROTO;
           header->dst_ip = MAGIC_DST_IP;
           header->src_ip = MAGIC_SRC_IP;

           header->dst_port = MAGIC_DST_PORT;
           header->src_port = MAGIC_SRC_PORT;

           #if(USE_NBO)
               header->udp_len = htons(64);
           #else
               header->udp_len = 64;
           #endif

           void* data = (void*)(header + 1);
           void* payload = data;
           #if DO_COPY
               payload = (void*)buff;
           #endif

            #if READ_DATA
                app_cycles_start = get_cycles();
                DTYPE* val = (DTYPE*)payload;

                for(unsigned int i = 0; i < data_len / sizeof(DTYPE); i++){
                    #if USE_NBO
                        #if DTYPE==i8
                            val[i] = MAGIC_DATA;
                            write_counts_nbo[sizeof(DTYPE) - 1]++;
                        #elif DTYPE==i16
                            val[i] = htons(MAGIC_DATA)
                            write_counts_nbo[sizeof(DTYPE) - 1]++;
                        #elif DTYPE==i32
                            val[i] = htonl(MAGIC_DATA)
                            write_counts_nbo[sizeof(DTYPE) - 1]++;
                        #elif DTYPE==i64
                            val[i] = bswap_64(MAGIC_DATA)
                            write_counts_nbo[sizeof(DTYPE) - 1]++;
                        #endif
                    #else
                        val[i] = MAGIC_DATA;
                        write_counts_hbo[sizeof(DTYPE) - 1]++;
                    #endif
                }


            #endif

            #if DO_COPY
                //Get the payload with a copy
                memcpy(data,(void*)buff, data_len);
            #endif

           header->udp_csum = MAGIC_UDP_CSDUM;

           u64 cycles_end = get_cycles();
           const u64 cycles = cycles_end - cycles_start;
           const u64 appcycles = cycles_end - app_cycles_start;
           cycles_min_idx = cycles < cycles_min ? cystats_idx : cycles_min_idx;

           cycles_min = cycles < cycles_min ? cycles : cycles_min;
           cycles_max = cycles > cycles_max ? cycles : cycles_max;
           cycles_total += cycles;

           *((u64*)cystats->first + cystats_idx) = cycles;
           *((u64*)appstats->first + cystats_idx) = appcycles;
           cystats_idx++;
       }
    }


    ch_log_info("WRITE_DATA count NBO1=%i NBO2=%i NBO4=%i NBO8=%i, HBO1=%i HBO2=%i HBO4=%i HBO8=%i\n",
            write_counts_nbo[1 -1] / 1000 / 1000,write_counts_nbo[2 -1]/ 1000 / 1000,write_counts_nbo[4 -1]/ 1000 / 1000,write_counts_nbo[8 -1]/ 1000 / 1000,
            write_counts_hbo[1 -1]/ 1000 / 1000,write_counts_hbo[2 -1]/ 1000 / 1000,write_counts_hbo[4 -1]/ 1000 / 1000,write_counts_hbo[8 -1]/ 1000 / 1000);

    const double micros = (double)(cycles_total) / cpu_frequency_hz * SEC2US;
    const double bytes  = (double)(options.packet_size * options.packet_count * options.iterations);
    const double rate_gbps   = bytes* 8 / micros  / 1000;
    const double cycles_p_packet = cycles_total / options.packet_count / options.iterations;
    const double micros_p_packet = micros / options.packet_count / options.iterations;


    ch_log_info("Ran for %0.2lfus (%lu kcycles) over %0.2lfMB of memory at %0.2f cycles/packet, %0.2f us/packets, %0.2lfGb/s \n", micros, cycles_total / 1000, bytes / 1024 / 1024, cycles_p_packet, micros_p_packet, rate_gbps);
    ch_log_info("Min[%li] = %li cycles, Max = %0.2li cycles\n", cycles_min_idx, cycles_min, cycles_max);
    for(int iter = 0; iter < cystats_idx; iter++ ){
        const u64* cycles       = ((u64*)cystats->first + iter);
        const double micros     = (double)(*cycles) / cpu_frequency_hz * SEC2US;
        const double rate_gbps  = options.packet_size* 8 / micros  / 1000;
        printf("WR %i %llu %0.4f %0.4f\n", iter, *cycles, micros, rate_gbps);
    }

    for(int iter = 0; iter < cystats_idx; iter++ ){
        const u64* cycles       = ((u64*)appstats->first + iter);
        const double micros     = (double)(*cycles) / cpu_frequency_hz * SEC2US;
        const double rate_gbps  = options.packet_size* 8 / micros  / 1000;
        printf("APWR %i %llu %0.4f %0.4f\n", iter, *cycles, micros, rate_gbps);
    }

}



static int run_sim()
{

    cystats_idx = 0;
    u64 cycles_min_idx = 0;
    u64 cycles_min = ~0;
    u64 cycles_max = 0;
    u64 cycles_total = 0;
    long result = 0;
    long read_counts_nbo[8] = {0}; //network byte order
    long read_counts_hbo[8] = {0}; //host byte order
    u64 errors = 0; 

    for(int iter = 0; iter < options.iterations; iter++ ){
        for(int i = 0; i < options.packet_count; i++){
            u64 cycles_start = get_cycles();
            u64 app_cycles_start = 0;
            pbuf_t* pbuf = ((pbuf_t*)packets->first + i);
            PTYPE* header = (PTYPE*)pbuf->data;
            const u64 data_len = options.packet_size - sizeof(PTYPE);
            #if(DO_PREFETCH)
               __builtin_prefetch ((pbuf_t*)packets->first + i + 1,0);
            #endif
            __builtin_prefetch ((u64*)cystats->first + cystats_idx + 1,1);

            #if(USE_ETHER)
                const int64_t src_mac = MAGIC_SRC_MAC;
                const int64_t dst_mac = MAGIC_DST_MAC;
                if(memcmp((void*)&header->dst_mac_raw, &src_mac,sizeof(header->dst_mac_raw))) { errors++; continue;}
                if(memcmp((void*)&header->dst_mac_raw, &dst_mac,sizeof(header->dst_mac_raw))) { errors++; continue;}

                if(header->eth_type != MAGIC_ETHER_TYPE_NBO) { errors++; continue;};
            #endif

            if(header->ver != MAGIC_IPV4_VER) { errors++; continue;};
            if(header->ihl != MAGIC_IPHL) { errors++; continue;};
            if(header->ecn != MAGIC_ECN) { errors++; continue;};
            if(header->dscp != MAGIC_DSCP) { errors++; continue;};


            stats.ipv4_packets++;
            #if(USE_NBO)
                stats.ipv4_bytes += ntohs(header->total_len);
            #else
                stats.ipv4_bytes += header->total_len;
            #endif

            if( header->id != MAGIC_IP_ID) { errors++; continue;};
            if( header->frag_off_flags != MAGIC_FRAG_OFF_FLAGS) { errors++; continue;};
            if( header->ttl != MAGIC_IP_TTL) { errors++; continue;};
            if( header->hdr_csum != MAGIC_IP_HDR_CSUM) { errors++; continue;};

            if( header->protocol != MAGIC_UDP_PROTO) { errors++; continue;};
            if( header->dst_ip != MAGIC_DST_IP) { errors++; continue;};
            if( header->src_ip != MAGIC_SRC_IP) { errors++; continue;};

            if( header->dst_port != MAGIC_DST_PORT) { errors++; continue;};
            if( header->src_port != MAGIC_SRC_PORT) { errors++; continue;};


            stats.udp_packets++;
            #if(USE_NBO)
                stats.udp_bytes += ntohs(header->udp_len);
            #else
                stats.udp_bytes += header->udp_len;
            #endif

            if(header->udp_csum != MAGIC_UDP_CSDUM) { errors++; continue;};

            void* data = (void*)(header + 1);
            void* payload = data;
            #if DO_COPY
                //Get the payload with a copy
                memcpy((void*)buff, data, data_len);
                payload = (void*)buff;
            #endif

            #if READ_DATA
                const DTYPE* val = (DTYPE*)payload;

                app_cycles_start = get_cycles();
                for(unsigned int i = 0; i < data_len / sizeof(DTYPE); i++){
                    #if USE_NBO
                        #if DTYPE==i8
                            result += val[i];
                            read_counts_nbo[sizeof(DTYPE) - 1]++;
                        #elif DTYPE==i16
                            result += ntohs(val[i]);
                            read_counts_nbo[sizeof(DTYPE) - 1]++;
                        #elif DTYPE==i32
                            result += ntohl(val[i]);
                            read_counts_nbo[sizeof(DTYPE) - 1]++;
                        #elif DTYPE==i64
                            result += bswap_64(val[i]);
                            read_counts_nbo[sizeof(DTYPE) - 1]++;
                        #endif
                    #else
                        result += val[i];
                        read_counts_hbo[sizeof(DTYPE) - 1]++;
                    #endif
                }


            #endif

            u64 cycles_end = get_cycles();
            const u64 cycles = cycles_end - cycles_start;
            const u64 appcycles = cycles_end - app_cycles_start;
            cycles_min = cycles < cycles_min ? cycles : cycles_min;
            cycles_max = cycles > cycles_max ? cycles : cycles_max;
            cycles_total += cycles;

            *((u64*)cystats->first + cystats_idx) = cycles;
            *((u64*)appstats->first + cystats_idx) = appcycles;
            cystats_idx++;

        }
    }


    //ch_log_info("READ_DATA result=%i\n", result);
    ch_log_info("READ_DATA count NBO1=%i NBO2=%i NBO4=%i NBO8=%i, HBO1=%i HBO2=%i HBO4=%i HBO8=%i\n",
            read_counts_nbo[1 -1] / 1000 / 1000,read_counts_nbo[2 -1]/ 1000 / 1000,read_counts_nbo[4 -1]/ 1000 / 1000,read_counts_nbo[8 -1]/ 1000 / 1000,
            read_counts_hbo[1 -1]/ 1000 / 1000,read_counts_hbo[2 -1]/ 1000 / 1000,read_counts_hbo[4 -1]/ 1000 / 1000,read_counts_hbo[8 -1]/ 1000 / 1000);
    ch_log_info("IPv4 count=%li, IPv4 bytes =%li, UDP count=%li, UDP bytes =%li\n", stats.ipv4_packets, stats.ipv4_bytes, stats.udp_packets, stats.udp_bytes);

    const double micros = (double)(cycles_total) / cpu_frequency_hz * SEC2US;
    const double bytes  = (double)(1514 * options.packet_count * options.iterations);
    const double rate_gbps   = bytes* 8 / micros  / 1000;
    const double cycles_p_packet = cycles_total / options.packet_count / options.iterations;
    const double micros_p_packet = micros / options.packet_count / options.iterations;


    ch_log_info("Ran for %0.2lfus (%lu kcycles) over %0.2lfMB of memory at %0.2f cycles/packet, %0.2f us/packets, %0.2lfGb/s \n", micros, cycles_total / 1000, bytes / 1024 / 1024, cycles_p_packet, micros_p_packet, rate_gbps);
    ch_log_info("Min[%li] = %li cycles, Max = %0.2li cycles\n", cycles_min_idx, cycles_min, cycles_max);
    ch_log_info("Errors=%lu\n", errors); 

    for(int iter = 0; iter < cystats_idx; iter++ ){
        const u64* cycles       = ((u64*)cystats->first + iter);
        const double micros     = (double)(*cycles) / cpu_frequency_hz * SEC2US;
        const double rate_gbps  = options.packet_size* 8 / micros  / 1000;
        printf("RD %i %llu %0.4f %0.4f\n", iter, *cycles, micros, rate_gbps);
    }

    for(int iter = 0; iter < cystats_idx; iter++ ){
        const u64* cycles       = ((u64*)appstats->first + iter);
        const double micros     = (double)(*cycles) / cpu_frequency_hz * SEC2US;
        const double rate_gbps  = options.packet_size* 8 / micros  / 1000;
        printf("APRD %i %llu %0.4f %0.4f\n", iter, *cycles, micros, rate_gbps);
    }

    return 0;
}
