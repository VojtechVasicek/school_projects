#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <ctype.h>

#include "../shared/base32.h"
#include "../shared/packet.h"
#include "./dns_receiver_events.h"
    
#define PORT    53
#define MAXLINE 1024
#define SEGMENT 60

/**
 * Tato struktura skladuje argumenty
 * 
 * @param base_host jméno host adresy
 * @param dst_filepath cesta k cílovému souboru
 */
struct args {
    char base_host[64];
    char dst_filepath[4096];
};

/**
 * Tato metoda zpracovává argumenty.
 *
 * @param argc počet argumentů
 * @param argv řetězec argumentů
 */
struct args parse_args(int argc, char* argv[]){
    struct args s;
    if(argc != 3){
        printf("Please enter 2 parameters");
        exit(1);
    }
    strcpy(s.base_host, argv[1]);
    strcpy(s.dst_filepath, argv[2]);
    return s;
}

/**
 * Tato metoda tiskne dekódovaná data do souboru
 *
 * @param encoded všechna zakódovaná data
 * @param len délka zakódovaných dat
 * @param filepath cesta k souboru kam se mají data po dekódování zapsat
 */
int print_to_file(char *encoded, int len, char *filepath){
    uint8_t payload_buf[len];
    base32_decode((const uint8_t *)encoded, payload_buf, len);
    printf("%s\n", encoded);
    printf("%s\n", payload_buf);
    struct dns_payload *payload = (struct dns_payload *)payload_buf;
    
    FILE *fout = fopen(filepath, "a+b");
    fseek(fout, (long int)payload->data, 0);
    fwrite(payload_buf, 1, strlen((const char *)payload_buf), fout);
    fclose(fout);
    strcpy(filepath, "");
    return strlen((const char *)payload_buf);
}

/**
 * Tato funkce zpracovává argumenty, inicializuje socket, přijímá data, odesílá odpovědi a volá další funkce
 * 
 * @param arguments struktura obsahující argumenty
*/
void runServer(struct args arguments){
      int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 
        
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
        
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
        
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    long unsigned int len;
    
    len = sizeof(cliaddr);
    char *encoded, *host;
    int length = 1;
    char dst_filepath[4096];
    while(1) {
        recvfrom(sockfd, (char *)buffer, MAXLINE,  
        MSG_WAITALL, (struct sockaddr *) &cliaddr, 
        &len);
        dns_receiver__on_transfer_init(&cliaddr.sin_addr);
        char cliaddrstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), cliaddrstr, INET_ADDRSTRLEN);
        struct dns_header *header = (struct dns_header *)buffer;
        struct dns_query *query = malloc(sizeof(buffer) + sizeof(struct dns_header)); 
        host = extractDnsQuery(buffer, query);
        int retcode = 0;
        if(strcmp(host, arguments.base_host) != 0){
            retcode = 1;
        }
        if(isdigit(query->segment[0][0])){
            char **as = str_split(query->segment[0], '|');
            length = atoi(as[0]);
            strcpy(dst_filepath, arguments.dst_filepath);
            strcat(dst_filepath, as[1]);
            
            encoded = malloc(length);
        }
        else if(query->segment[0][0] == '-'){
            int l = print_to_file(encoded, strlen(encoded), dst_filepath);
            dns_receiver__on_transfer_completed(dst_filepath, l);
        }
        else {
            int k = 1;
            for (size_t j = 0; j < strlen(host); j++){
                if(host[j] == '.'){
                    k++;
                }
            }
            strncat((char *)encoded, query->segment[0], 1024);
            char dnsquery[MAXLINE];
            strcpy(dnsquery, query->segment[0]);
            for(size_t i = 1; i < query->num_segments; i++){
                strcat(dnsquery, ".");
                strcat(dnsquery, query->segment[i]);
            }
            dns_receiver__on_query_parsed(dst_filepath, dnsquery);
            dns_receiver__on_chunk_received(&cliaddr.sin_addr, dst_filepath, header->id, sizeof(query->segment[0]));

        }
        struct dns_packet *packet = getResponsePacket(query, header->id, MAXLINE, cliaddrstr, retcode);
        sendto(sockfd, packet->dns_buf, packet->buf_size, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        free(query);
        
    }
    free(host);
    free(encoded);
}

/**
 * Main
 *
 */
int main(int argc, char* argv[]) {
    struct args arguments = parse_args(argc, argv);
    runServer(arguments);
    return 0; 
}