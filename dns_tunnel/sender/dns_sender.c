#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h> 
#include <stdint.h>
#include <assert.h>

#include "../shared/base32.h"
#include "../shared/packet.h"
#include "./dns_sender_events.h"

#define PORT    53
#define SEGMENT 60
#define MAXLINE 1024


/**
 * Struktura argumentů
 * 
 * @param dns_ip ip adresa serveru
 * @param base_host jméno host adresy
 * @param dst_filepath jméno cílového souboru
 * @param src_filepath cesta k souboru s daty k odeslání
 */
struct args {
    char dns_ip[15];
    char base_host[64];
    char dst_filepath[4096];
    char src_filepath[4096];
};

/**
 * Tato metoda zpracovává argumenty.
 *
 * @param argc počet argumentů
 * @param argv řetězec argumentů
 */
struct args parse_args(int argc, char* argv[]){
    struct args s;
    
    if(argc < 3){
        printf("Please enter at least 2 parameters");
        exit(1);
    }
    if(argc > 6){
        printf("You entered too many parameters");
        exit(1);
    }
    if(strcmp(argv[1], "-u") == 0){
        strcpy(s.dns_ip, argv[2]);
        strcpy(s.base_host, argv[3]);
        strcpy(s.dst_filepath, argv[4]);
        if(argc == 6){
            strcpy(s.src_filepath, argv[5]);
        }
        else {
            strcpy(s.src_filepath, "stdin");
        }
    }
    else {
        FILE *in_file = fopen("/etc/resolv.conf", "rb");
        if (!in_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        struct stat sb;
        if (stat("/etc/resolv.conf", &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        char *file_contents = malloc(sb.st_size);

        while (fscanf(in_file, "%[^\n] ", file_contents) != EOF) {
            if(strstr(file_contents, "nameserver") != NULL){
                for(size_t i = 11; i <= strlen(file_contents); i++){
                    s.dns_ip[i - 11] = file_contents[i];    
                }
                break;
            }
        }

        fclose(in_file);

        strcpy(s.base_host, argv[1]);
        strcpy(s.dst_filepath, argv[2]);
        if (argc == 4){
            strcpy(s.src_filepath, argv[3]);
        }
        else {
            strcpy(s.src_filepath, "stdin");
        }
        free(file_contents);
    }
    
    return s;
}

/**
 * Tato metoda načítá data ze souboru nebo stdinu.
 *
 * @param arguments struktura obsahující všechny zpracované argumenty
 */
char* loadData(struct args arguments){
    if(strcmp(arguments.src_filepath, "stdin") == 0){
        char *buffer = malloc(65536);
        char *buffer1 = malloc(65536);
        FILE *f = stdin;
        while(fgets(buffer1, sizeof(buffer), f) != NULL){strcat(buffer, buffer1);}
        free(buffer1);
        return buffer;
    }
    else {
        FILE *f;
        if ((f = fopen(arguments.src_filepath, "r"))) {
            fseek(f, 0, SEEK_END);
            int lengthOfFile = ftell(f);
            rewind(f);
            char *buffer = malloc(lengthOfFile);
            fread(buffer, lengthOfFile, 1, f);
            fclose(f);
            
            return buffer;
        } else {
            printf("file doesn't exist");
            return NULL;
        }
    }
}


/**
 * Tato metoda se volá pro odeslání dat. Je to hlavní pracovní metoda, ostatní se volají z ní. Provádí
 * inicializaci socketů i odesílání dat.
 *
 * @param arguments struktura obsahující všechny zpracované argumenty
 */
int sendData(struct args arguments){
    int sockfd;
    char buffer[MAXLINE];
    char* data = loadData(arguments);
    struct sockaddr_in     servaddr; 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    char base_host[strlen(arguments.base_host)];
    strcpy(base_host, arguments.base_host);
    char **host = str_split(arguments.base_host, '.');
    char result[4 * strlen(data)];
    printf("%s\n", data);
    base32_encode((const uint8_t *)data, strlen(data), (uint8_t *)result, 4*strlen(data));
    int length = strlen(result);
    int size = sizeof(result);
    char str[200];
    sprintf( str,"%d", size);
    strcat(str, "|");
    strcat(str, arguments.dst_filepath);
    int i = 0;
    struct dns_packet *packet = getQueryPacket(str, i, host, MAXLINE);
    dns_sender__on_transfer_init((struct in_addr *)arguments.dns_ip);
    sendto(sockfd, packet->dns_buf, packet->buf_size, 
    MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
        sizeof(servaddr));
    long unsigned int len;
    len = sizeof(servaddr);
    recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
    i++;
    do{
        char *start = &result[(i - 1) * SEGMENT];
        char *substr = (char *)calloc(1, SEGMENT);
        memcpy(substr, start, SEGMENT);
        int id = htons(getpid() + i);
        char chunk_encoded[strlen(str) + 1 + strlen(base_host)];
        strcpy(chunk_encoded, substr);
        strcat(chunk_encoded, ".");
        strcat(chunk_encoded, base_host);
        dns_sender__on_chunk_encoded(arguments.dst_filepath, id, chunk_encoded);
        struct dns_packet *packet = getQueryPacket(substr, id, host, MAXLINE);
        dns_sender__on_transfer_init((struct in_addr *)arguments.dns_ip);
        sendto(sockfd, packet->dns_buf, packet->buf_size, 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                sizeof(servaddr));
        dns_sender__on_chunk_sent((struct in_addr *)arguments.dns_ip, arguments.dst_filepath, id, strlen(substr));
        recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
        struct dns_header *header = (struct dns_header *)buffer;
        if(header->rcode == 1){
            printf("Base_host doenst match base_host on server.\n");
            exit(1);
        }
        else if(header->rcode == 2){
            printf("Wrong packet format.\n");
            exit(2);
        }
        i++;
        free(substr);
    }while((length - ((i - 1) * SEGMENT) > 0));
    packet = getQueryPacket("-", i, host, MAXLINE);
    dns_sender__on_transfer_init((struct in_addr *)arguments.dns_ip);
    sendto(sockfd, packet->dns_buf, packet->buf_size, 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr));
    dns_sender__on_transfer_completed(arguments.dst_filepath, strlen(data));
    recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    free(packet);
    close(sockfd);
    free(data);
    return 0;
}

/**
 * Main, volá ostatní metody

 */
int main(int argc, char* argv[]) {
    struct args arguments;
    arguments = parse_args(argc, argv);
    sendData(arguments);
    return 0; 
}