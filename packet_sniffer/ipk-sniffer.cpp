#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <ctime>
#include <pcap/pcap.h>

bool tcp = false;   // Global variables needed in multiple functions
bool udp = false;
int nvalue = 1;

/*****************************************************************************
 *
 *  Base for function: PrintData
 *
 *  Title: Packet Sniffer Code in C using sockets | Linux
 *  Author: Silver Moon
 *  Availability: https://www.binarytides.com/packet-sniffer-code-c-linux/
 *
 ****************************************************************************/

void PrintData (const u_char * data , int Size){    // Prints the data payload
    int i , j;
    int k = 0;
    if (Size != 0){     // Prints number of bytes printed before the first line, which is 0
        printf("0x0000:");
        k += 16;
    }
    for (i=0 ; i < Size ; i++){
        if ( i!=0 && i%16==0){   // If one line of hex printing is complete
            printf("         ");
            for (j=i-16 ; j<i ; j++){
                if (isalnum(data[j]))
                    printf("%c",(unsigned char)data[j]); // If its a number or alphabet, print it
                else printf(".");   // Otherwise print a dot
            }
            printf ("\n");
            if (k < 160){       // Prints hex number of bytes printed before this line
                printf("%#06x:", k);
            }
            else if (k < 1600){
                printf("%#06x:", k);
            }
            else {
                printf("%#06x:", k);
            }
            k += 16;
        }
        if (i%16==0) printf("   ");
            printf(" %02X",(unsigned int)data[i]);
        if (i==Size-1){     // Print the last spaces
            for (j=0;j<15-i%16;j++){
              printf("   ");  // Print extra spaces
            }
            printf("         ");
            for (j=i-i%16 ; j<=i ; j++){
                if (isalnum(data[j]))
                  printf("%c",(unsigned char)data[j]);  // If its a number or alphabet, print it
                else
                  printf(".");  // Otherwise print a dot
            }
            printf("\n" );
        }
    }
}

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer){  // Processes packet and
    struct iphdr *ipheader = (struct iphdr*) (buffer + sizeof(struct ethhdr));              // prints header
    struct tcphdr *tcpheader = (struct tcphdr*) (buffer + sizeof(iphdr) + sizeof(struct ethhdr));
    struct in_addr *source = (struct in_addr*)buffer;   // Declaration of structures
    struct timeval eptime;
    unsigned short iphdrlen = ipheader->ihl * 4;
    int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof(tcphdr);
    source->s_addr = ipheader->saddr;
    gettimeofday(&eptime, NULL);    // Gets local time of capture
    time_t t = time(NULL);
    struct tm* tim = localtime(&t);
    printf("%d:%d:%d.%ld %s : %u > ", tim->tm_hour, tim->tm_min, tim->tm_sec, eptime.tv_usec, inet_ntoa(*source),
            ntohs(tcpheader->source)); // Prints first half of packet header
    struct in_addr *dest = (struct in_addr*)buffer; // Assigns destination
    dest->s_addr = ipheader->daddr;
    printf("%s : %u\n \n", inet_ntoa(*dest), ntohs(tcpheader->dest));   // Prints second half of the packet header
    PrintData(buffer + header_size, header->len - header_size); // Calls function to print data payload
    printf("\n"); // Prints empty line after the packet is processed
}

void numcheck(char *arg, const char *type){   // Function checks if the value is not null and is integer
    if (arg == NULL){   // If the value is empty, program end with error
        fprintf(stderr, "Nenalezena hodnota argumentu %s. Program se ukonci.\n", type);
        exit(1);
    }
    int len = strlen(arg);
    for (int k = 0; k < len; k++){  // Checks if all characters are digits
        if (!isdigit(arg[k])){  // If not, program ends with error
            fprintf(stderr, "Spatna hodnot argumentu %s <int>. Program se ukonci.\n", type);
            exit(1);
        }
    }
}

int main(int argc, char** argv) {
	bool n = false;     // Declaration of variables and structures.
	bool interface = false;
	bool port = false;
	int i = 1;
	int p;
	char errbuff[1000];
	char* interfacevalue = 0;
	char* fltr;
	const char* prt;
	const char* portvalue;
    const u_char* packet;
	pcap_t* handle;
	bpf_u_int32 mask;
    bpf_u_int32 net;
    struct bpf_program filter;
    struct pcap_pkthdr header;
    if (argc > 1){
        if (strcmp(argv[1], "-help") == 0){
            printf("Tento program ma za ukol sniffovat pakety podle zadanych parametru.\nPro detailnejsi popis programu"
                   " a pomoc se spustenim se obratte na dodane README.\n");
            exit(0);
        }
    }
	while (i != argc) {     // Parsing arguments
		if (strcmp(argv[i], "-i") == 0) {   // Interface argument
			if (interface == true) {    // Check if the argument was already passed
				fprintf(stderr, "Redundantni argument %s. Program se ukonci.\n", argv[i]);
				exit(1);
			}
			if (argv[i + 1] == NULL){   // If the value is empty, program end with error
			    fprintf(stderr, "Nenalezena hodnota argumentu %s. Program se ukonci.\n", argv[i]);
			    exit(1);
			}
			int len = strlen(argv[i + 1]);  // Checking if the interface contains only alphanumeric symbols
            for (int k = 0; k < len; k++) {
                if (!isalnum(argv[i + 1][k])) {
                    fprintf(stderr, "Spatna hodnot argumentu -i <string>. Program se ukonci.\n");
                    exit(1);
                }
            }
			interface = true;   // Interface was already passed
			interfacevalue = argv[i + 1];   // Assign the interface into a variable
			i++;    // Skip the argument with the name of the interface in the next cycle
		}
		else if (strcmp(argv[i], "-p") == 0) {  // Port argument
			if (port == true) { // Check if argument was already passed
				fprintf(stderr, "Redundantni argument %s. Program se ukonci.\n", argv[i]);
				exit(1);
			}
			numcheck(argv[i + 1], "-p"); // Check if the value of port contains only digits
			port = true;    // Port was already passed
            prt = "port ";  // Assign values for the eventual assignment to the filter later
            portvalue = argv[i + 1];
			i++;    // Skip the argument with the value of the port in the next cycle
		}
		else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tcp") == 0) { // Tcp only argument
			if (tcp == true) {  // Check if the argument was already passed
				fprintf(stderr, "Redundantni argument %s. Program se ukonci.\n", argv[i]);
				exit(1);
			}
			tcp = true;  // Argument was already passed
		}
		else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--udp") == 0) { // Udp only argument
			if (udp == true) {  // Check if the argument was already passed
				fprintf(stderr, "Redundantni argument %s. Program se ukonci\n", argv[i]);
				exit(1);
			}
			udp = true; // Argument is passed
		}
		else if (strcmp(argv[i], "-n") == 0) {  // Number of packets to be sniffed
			if (n == true) {    // Check if argument was already passed
				fprintf(stderr, "Redundantni argument %s. Program se ukonci.\n", argv[i]);
				exit(1);
			}
			numcheck(argv[i + 1], "-n");    // Check if the value of packets to be sniffed is only digits
			n = true;   // Argument is passed
			nvalue = atoi(argv[i + 1]); // Transform string of chars into int and assign it to variable
			i++;    // Skip the argument with the number of packets in the next cycle
		}
		else {  // If the argument is something else, the program ends with error
		    printf("Neplatny argument %s. Program se ukonci.\n", argv[i]);
		    exit(1);
		}
		i++;    // Increments the cycle
	}
	if (interfacevalue == 0){    // If interface is not assigned print all available interfaces
	    pcap_if_t **alldevs = (pcap_if_t **) malloc(100);   // Allocates memory for list of available interfaces
	    if (pcap_findalldevs(alldevs, errbuff) == 0){ // If creation of the list of available interfaces was succesfull
	        printf("Dostupna rozhrani: \n");
            while (alldevs != NULL){
                printf("%s \n", alldevs[0]->name); // Print interfaces
                if (alldevs[0]->next == NULL){  // If this is the last interface, end the cycle
                    break;
                }
                alldevs[0] = alldevs[0]->next;  // Else assign the next interface into this one
            }
	    }
	    else{   // If no interfaces are found, print this message
	        printf("Nebylo nalezeno zadne dostupne rozhrani. Program se vypne.");
	    }
	    free(alldevs);  // Free the allocated memory
	    exit(0);    // Exit the program
	}
	if (tcp && udp){    // If both tcp and udp arguments are active, we sniff both tcp and udp packets
	    tcp = false;
	    udp = false;
	}
	if (tcp || udp){    // If tcp or udp arguments are active
	    if (!port){     // If port is not required
	        fltr = (char *) malloc(5);  // Allocate space for tcp or udp argument only
	        if (tcp)    // Assign tcp
	            strcpy(fltr, "tcp");
	        else if (udp)   // Assign udp
	            strcpy(fltr, "udp");
	    } else {    // If port is required
	        fltr = (char *) malloc(strlen(prt) + 12);   // Allocate space for tcp or udp and port
	        if (tcp){   // Assign tcp and port
	            strcpy(fltr, "tcp ");
                strcat(fltr, prt);
                strcat(fltr, portvalue);
	        }
	        else if (udp){ // Assign udp and port
	            strcat(fltr, "udp ");
	            strcat(fltr, prt);
                strcat(fltr, portvalue);
	        }
	    }
	}
	else if (port){ // If only port is required
	    fltr = (char *) malloc(strlen(prt) + 8);    // Allocate space for port
	    strcpy(fltr, prt); // Assign port
	    strcat(fltr, portvalue);
	}
	else if (!port){    // If neither port nor tcp or udp is required
	    fltr = (char *) malloc(1); // Allocate memory for filter which will remain empty
	}

	if (pcap_lookupnet(interfacevalue, &net, &mask, errbuff) == -1){    // If function doesnt end with succes
	    fprintf(stderr, "Nepodarilo se ziskat netmask pro zarizeni\b\n"); // Print error message
        free(fltr);     // Free allocated memory
	    exit(1);     // End program
	}
	handle = (pcap_t *) malloc(65600);  // Allocate memory to handle
	pcap_set_snaplen(handle, 65536);   // Set maximum length of handle
	handle = pcap_open_live(interfacevalue, 65536, 1, 1000, errbuff); // Open the handle(interface) to listening
	if (handle == NULL) {   // If handle is empty, print error message and end the program
		fprintf(stderr, "Nepodarilo se otevrit zadane rozhrani\n");
        free(handle);   // Frees allocated memory
        free(fltr); // Frees allocated memory
		exit(1);
	}
	if (pcap_compile(handle, &filter, fltr, 0, net) == -1){ // If compilation fails, print error message and exit
	    fprintf(stderr, "Nepodarilo se zparsovat filtr\n");
        free(handle);   // Frees allocated memory
        free(fltr); // Frees allocated memory
	    exit(1);
	}
	if (pcap_setfilter(handle, &filter) == -1){ // If assignment of filter fails, print error message and exit
	    fprintf(stderr, "Nepodarilo se nainstalovat filtr\n");
	    free(handle);   // Frees allocated memory
	    free(fltr); // Frees allocated memory
	    exit(1);
	}
	pcap_loop(handle, nvalue, process_packet, NULL);    // Sniff packet and call process_packet function
	pcap_close(handle); // Closes the interface
	free(handle);   // Frees allocated memory
	free(fltr); // Frees allocated memory
	return 0;
}