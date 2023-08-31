#include "packet.h"
#include "base32.h"


char** str_split(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    while (*tmp){
        if (a_delim == *tmp){
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;
    result = malloc(sizeof(char*) * count);
    if (result){
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token){
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}


struct dns_packet* getResponsePacket(struct dns_query *query, unsigned short id, int MAXLINE, char *ip, int retcode){
    struct dns_packet *packet = malloc(MAXLINE + sizeof(size_t));
    memset(packet->dns_buf, 0, MAXLINE);
    struct dns_header *header = (struct dns_header *)packet->dns_buf;
    header->id = id;
    header->qr = 1;
    header->aa = 0;
    header->tc = 0;
    header->ra = 0;
    if(query->type == 0x01){
        header->rcode = retcode;
        header->ans_count = htons(1);
    }
    else{
        header->rcode = 2;
        header->ans_count = 0;
    }
    header->auth_count = 0;
    header->add_count = 0;
    packet->buf_size = 60 + 18;
    if (query->type == 0x01) {
        struct dns_response_trailer *trailer = (struct dns_response_trailer *)(packet->dns_buf + 60);
        trailer->ans_type = 0xc0;
        trailer->name_offset = 0x0c;
        trailer->type = htons(0x01);
        trailer->qclass = htons(0x0001);
        trailer->ttl = htonl(300);
        trailer->rdlength = htons(4);
        inet_pton(AF_INET, ip, &trailer->rdata);
    }
    return packet;
}


struct dns_packet* getQueryPacket(char* data, int id, char** host, int MAXLINE){
    struct dns_packet *packet = malloc(MAXLINE + sizeof(size_t));
    memset(packet->dns_buf, 0, MAXLINE);
    struct dns_header *header = (struct dns_header *)packet->dns_buf;
    header->id = id;
    header->rd = 1;
    header->q_count = htons(1);
    unsigned char *dns_buf_ptr = packet->dns_buf + sizeof(struct dns_header);
    size_t count = strlen(data);
    *dns_buf_ptr = (unsigned char)count;
    memcpy(dns_buf_ptr + 1, data, count);
    
    
    dns_buf_ptr += count + 1;
    int lensum = 0;
    int j = 0;
    while(host[j] != NULL){
        if (j == 0){
            *dns_buf_ptr = (unsigned char)strlen(host[j]);
            memcpy(dns_buf_ptr + 1, host[j], strlen(host[j]));
        }
        else {
            *(dns_buf_ptr + lensum + j) = (unsigned char)strlen(host[j]);
            memcpy(dns_buf_ptr + lensum + j + 1, host[j], strlen(host[j]));
        }
        lensum += strlen(host[j]);
        j++;
    }
    lensum += j;
    *(dns_buf_ptr + lensum) = (unsigned char)0;
    *((uint16_t *)(dns_buf_ptr + lensum + 1)) = htons(1);
    *((uint16_t *)(dns_buf_ptr + lensum + 3)) = htons(1);
    packet->buf_size = dns_buf_ptr + lensum + 5 - packet->dns_buf;
    return packet;
}


char* extractDnsQuery(char *dns_buffer, struct dns_query *query) {
  char *query_ptr = dns_buffer + sizeof(struct dns_header);
  query->num_segments = 0;
  uint8_t segment_size;
  while ((segment_size = *((uint8_t *)query_ptr))) {
    if (segment_size > 63) {
        exit(1);
    }
    strncpy(query->segment[query->num_segments],
            (char *)(query_ptr + 1), segment_size);
    
    query->segment[query->num_segments][segment_size] = '\0';
    ++query->num_segments;
    query_ptr += segment_size + 1;
  }
  uint16_t *qtype_ptr = (uint16_t *)(query_ptr + 1);
  query->type = ntohs(*qtype_ptr);
  uint16_t *qclass_ptr = (uint16_t *)(query_ptr + 3);
  query->qclass = ntohs(*qclass_ptr);
  char *hostname = malloc(15 * (query->num_segments - 1));
  strcpy(hostname, query->segment[1]);
  for(size_t i = 2; i < query->num_segments; i++){
    strcat(hostname, ".");
    strcat(hostname, query->segment[i]);
  }
  return hostname;
}