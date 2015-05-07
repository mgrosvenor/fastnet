/*
 * packet_types.h
 *
 *  Created on: May 30, 2014
 *      Author: mgrosvenor
 */

#ifndef PACKET_TYPES_H_
#define PACKET_TYPES_H_

//Basic packet structure
typedef struct{
	uint8_t  dst_mac_raw[6];
	uint8_t  src_mac_raw[6];
	uint16_t eth_type;
	uint8_t  ihl        : 4;
	uint8_t  ver        : 4;
	uint8_t  ecn        : 2;
	uint8_t  dscp       : 6;
	uint16_t total_len;
	uint16_t id;
	uint16_t frag_off_flags;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t hdr_csum;
	uint32_t src_ip;
	uint32_t dst_ip;
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t udp_len;
	uint16_t udp_csum;
} __attribute__((__packed__)) eth_ip_udp_head_t ;


//Structure without ethernet header
typedef struct{
	uint8_t  ihl        : 4;
	uint8_t  ver        : 4;
	uint8_t  ecn        : 2;
	uint8_t  dscp       : 6;
	uint16_t total_len;
	uint16_t id;
	uint16_t frag_off_flags;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t hdr_csum;
	uint32_t src_ip;
	uint32_t dst_ip;
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t udp_len;
	uint16_t udp_csum;
} __attribute__((__packed__)) ip_udp_head_t ;

//Structure IP header 32bit
typedef struct{
	uint8_t  padding[2];
	uint8_t  dst_mac_raw[6];
	uint8_t  src_mac_raw[6];
	uint16_t eth_type;
	uint8_t  ihl        : 4;
	uint8_t  ver        : 4;
	uint8_t  ecn        : 2;
	uint8_t  dscp       : 6;
	uint16_t total_len;
	uint16_t id;
	uint16_t frag_off_flags;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t hdr_csum;
	uint32_t src_ip;
	uint32_t dst_ip;
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t udp_len;
	uint16_t udp_csum;
} __attribute__((__packed__)) eth_32ip_udp_head_t ;

//Structure IP header 64bit aligned
typedef struct{
	uint8_t  padding[10];
	uint8_t  dst_mac_raw[6];
	uint8_t  src_mac_raw[6];
	uint16_t eth_type;
	uint8_t  ihl        : 4;
	uint8_t  ver        : 4;
	uint8_t  ecn        : 2;
	uint8_t  dscp       : 6;
	uint16_t total_len;
	uint16_t id;
	uint16_t frag_off_flags;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t hdr_csum;
	uint32_t src_ip;
	uint32_t dst_ip;
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t udp_len;
	uint16_t udp_csum;
} __attribute__((__packed__)) eth_64ip_udp_head_t ;

//Ethernet compatible - Ethertype 64bit aligned, IP 64bit aligned, 64bit sized fields
typedef struct{
	uint8_t  padding[4];
	uint8_t  dst_mac_raw[6];
	uint8_t  src_mac_raw[6];
	uint16_t eth_type;
	uint8_t padding2[6];
	uint64_t  ihl;
	uint64_t  ver;
	uint64_t  ecn;
	uint64_t  dscp;
	uint64_t total_len;
	uint64_t id;
	uint64_t frag_off_flags;
	uint64_t  ttl;
	uint64_t  protocol;
	uint64_t hdr_csum;
	uint64_t src_ip;
	uint64_t dst_ip;
	uint64_t src_port;
	uint64_t dst_port;
	uint64_t udp_len;
	uint64_t udp_csum;
} __attribute__((__packed__)) eth64_64ip64_64udp_head_t ;

//Whole stack
typedef struct{
	uint64_t dst_mac_raw;
	uint64_t src_mac_raw;
	uint64_t eth_type;
	uint64_t ihl;
	uint64_t ver;
	uint64_t ecn;
	uint64_t dscp;
	uint64_t total_len;
	uint64_t id;
	uint64_t frag_off_flags;
	uint64_t ttl;
	uint64_t protocol;
	uint64_t hdr_csum;
	uint64_t src_ip;
	uint64_t dst_ip;
	uint64_t src_port;
	uint64_t dst_port;
	uint64_t udp_len;
	uint64_t udp_csum;
} __attribute__((__packed__)) eth6464_64ip64_64udp_head_t ;


#endif /* PACKET_TYPES_H_ */
