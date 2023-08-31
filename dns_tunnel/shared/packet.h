#ifndef _PACKET_H
#define _PACKET_H

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h> 
#include <stdint.h>
#include <assert.h>


/**
 * Tato struktura reprezentuje header packetu
 */
struct dns_header {
	unsigned short id;

	unsigned char rd :1;
	unsigned char tc :1;
	unsigned char aa :1;
	unsigned char opcode :4;
	unsigned char qr :1;

	unsigned char rcode :4;
	unsigned char cd :1;
	unsigned char ad :1;
	unsigned char z :1;
	unsigned char ra :1;

	unsigned short q_count;
	unsigned short ans_count;
	unsigned short auth_count;
	unsigned short add_count;
};

/**
 * Tato struktura reprezentuje paket na odeslání v raw podobě
 */
struct dns_packet {
    unsigned char dns_buf[1024];
    size_t buf_size;
};

/**
 * Tato struktura reprezentuje přijatou query
 */
struct dns_query {
  size_t num_segments;
  char segment[10][64];
  uint16_t type;
  uint16_t qclass;
};

/**
 * Tato struktura reprezentuje přijatý packet
 */
struct dns_payload {
  unsigned short uuid;
  uint32_t sequence;
  uint8_t length;
  char data[1024];
};

/**
 * Tato struktura reprezentuje přídavek k response packetu
 */
struct dns_response_trailer {
  uint8_t ans_type;
  uint8_t name_offset;
  uint16_t type;
  uint16_t qclass;
  uint32_t ttl;
  uint16_t rdlength;
  uint32_t rdata;
};

/**
 * Tato metoda rozkládá string na substringy pomocí delimiteru. Metoda je převzatá z internetu, bohužel jsem ztratil link
 *
 * @param a_str string na rozložení
 * @param a_delim znak který je zlomovým bode kde se má string rozložit
 */
char** str_split(char* a_str, const char a_delim);

/**
 * Tato metoda skládá packet s dotazem na server
 *
 * @param data data co se mají odeslat na server
 * @param id id packetu a chunku dat
 * @param host hostname rozložené na části
 * @param MAXLINE maximální velikost packetu
 */
struct dns_packet* getQueryPacket(char* data, int id, char** host, int MAXLINE);

/**
 * Tato metoda skládá packet s odpovědí.
 *
 * @param query query která se bude odesílat
 * @param id id packetu a chunku dat
 * @param MAXLINE maximální délka zprávy
 * @param ip cilová ip adresa
 * @param retcode return code packetu, 0 = no error, 1 = špatné hostname, 2 = špatný typ přijatého packetu
 */
struct dns_packet* getResponsePacket(struct dns_query *query, unsigned short id, int MAXLINE, char *ip, int retcode);

/**
 * Tato metoda parsuje přijatý packet
 *
 * @param dns_buffer přijatý packet
 * @param query do této struktury se packet parsuje
 * @return metoda vrací celé jméno hosta přijetá od clienta
 */
char* extractDnsQuery(char *dns_buffer, struct dns_query *query);

#endif