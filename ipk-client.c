#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "msg.h"

char *arguments(int argc, char **argv, char *type, int *port, char **host);

int main(int argc, char **argv)
{
    int port;
    char *host = NULL;
    char *string;
    char type;
    string = arguments(argc, argv, &type, &port, &host);

    int client_socket;

    struct hostent *hp;
    struct addrinfo hint, *res = NULL;
    struct sockaddr_in *h;
    int ret;
    int check = 0;
    memset(&hint, '\0', sizeof hint);

    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    // Pomoci getaddr info zjistime jestli je adresa IPv4, IPv6 nebo neco jine
    ret = getaddrinfo(host, NULL, &hint, &res);

    // Pokud to neni IPv4 ani IPv6, muze to byt domenove jmeno 
    if (ret != 0)
    {
        hp = gethostbyname(host);

        check = 1;
        memset(&hint, 0, sizeof hint);
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

        if (hp == NULL) {
            perror("ERROR: gethostbyname");
	    exit(EXIT_FAILURE);
        }


        memset(&hint, '\0', sizeof hint);
        hint.ai_family = PF_UNSPEC;
        hint.ai_flags = AI_NUMERICHOST;

        // Pokud ani ted nedostaneme IPv4 nebo IPv6, zadany host je neplatny
        ret = getaddrinfo(inet_ntoa( *( struct in_addr*)( hp -> h_addr_list[0])), NULL, &hint, &res);
        if (ret != 0)
        {
            perror("ERROR: getaddrinfo");
            exit(EXIT_FAILURE);
        }
    }

    if (res->ai_family == AF_INET)
    {
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        if (check)
            server_address.sin_addr = *(struct in_addr*)hp->h_addr_list[0];
        else
            inet_pton(AF_INET, host, &server_address.sin_addr);
        server_address.sin_port = htons(port);

        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
        {
            perror("ERROR: socket");
            exit(EXIT_FAILURE);
        }

        if (connect(client_socket,
                  (const struct sockaddr *) &server_address,
                   sizeof(server_address)) !=0)
        {
            perror("ERROR: connect");
            exit(EXIT_FAILURE);
        }

    } else if (res->ai_family = AF_INET6)
    {
        struct sockaddr_in6 server_address;
        server_address.sin6_family = AF_INET6;
        
        server_address.sin6_addr = in6addr_any;
        server_address.sin6_port = htons(port);

        if ((client_socket = socket(AF_INET6, SOCK_STREAM, 0)) <= 0)
        {
            perror("ERROR: socket");
            exit(EXIT_FAILURE);
        }

        if (connect(client_socket,
                   (const struct sockaddr *) &server_address,
                   sizeof(server_address)) !=0)
        {
            perror("ERROR: connect");
            exit(EXIT_FAILURE);
        }
    }

    int bytestx, bytesrx;;
    char buff[1024];
    string[strlen(string)] = '\0';

    // Zasilam spravu o typu TYPE
    msg message;
    message.type = TYPE;
    message.string[0] = type;
    bytestx = send(client_socket, (char *)&message, sizeof(message), 0);
    if (bytestx < 0) {
        perror("ERROR: sendto");
    }

    // Zasilam spravu o hledanem retezci SEARCH
    message.type = SEARCH;
    memset(message.string, '\0', sizeof message.string);
    memcpy(message.string, string, sizeof(char)*strlen(string));
    bytestx = send(client_socket, (char *)&message, sizeof(message), 0);
    if (bytestx < 0) {
        perror("ERROR: sendto");
    }

    msg incoming_message;
    switch (type)
    {
        // kod pro argumenty -n a -f, vypise se jen 1 radek
        case N:
        case F:
            while(1)
            {
                memset(&incoming_message, '\0', sizeof(incoming_message));
                bytesrx = recv(client_socket, (char *)&incoming_message, sizeof(incoming_message), 0);
                if (bytesrx < 0) {
                    perror("ERROR: recvfrom");
                }

                if(incoming_message.type == DATA)
                    printf("%s\n",incoming_message.string);
                else if (incoming_message.type == END)
                    break;
                }
                break;
        // kod pro argumenty -l, vypisuje se vice informaci
        case L:
            while (1){
                memset(&incoming_message, '\0', sizeof(incoming_message));
                bytesrx = recv(client_socket,  (char *)&incoming_message, sizeof(incoming_message), 0);
                if (bytesrx < 0) {
                    perror("ERROR: recvfrom");
                }

                if(incoming_message.type == DATA)
                {
                    printf("%s\n",incoming_message.string);
                } else if (incoming_message.type == END)
                {
                    break;
                }
            }
            break;
    }
    message.type = END;
    bytestx = send(client_socket, (char *)&message, sizeof(message), 0);

    free(host);
    free(string);

    return 0;
}


char *arguments(int argc, char **argv, char *type, int *port, char **host)
{
    int c;
    char *string = NULL;
    int set = 0;
    while ((c = getopt(argc, argv, "h:p:n:f:l::")) != -1)
    {
        if (set)
        {
            perror("ERROR: wrong arguments");
            exit(EXIT_FAILURE);
        }
        switch(c)
        {
            case 'h':
                *host = malloc(sizeof(char)*strlen(optarg));
		*host[0] = '\0';
                strcat(*host,optarg);
                break;
            case 'p':
                *port = atoi(optarg);
                break;
            case 'n':
                string = malloc(sizeof(char)*(strlen(optarg)));
                string[0] = '\0';
                strcat(string, optarg);
                set = 1;
                *type = N;
                break;
            case 'f':
                string = malloc(sizeof(char)*(strlen(optarg)));
                string[0] = '\0';
                strcat(string, optarg);
                set = 1;
                *type = F;
                break;
            case 'l':
                if (argc == optind) {
                    string = malloc(sizeof(char)*2);
                    string[0] = '\0';
                } else {
                    string = malloc(sizeof(char)*(strlen(argv[optind])));
                    string[0] = '\0';
                    strcat(string, argv[optind]);
                }
                set = 1;
                *type = L;
                break;
            default:
                exit(1);
                break;
        }
    }
    return string;
}

