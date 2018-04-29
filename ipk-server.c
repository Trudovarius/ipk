#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "msg.h"

void arguments(int argc, char **argv, int *port);
char *getNF(FILE *f, char *string, char type);
void getL(FILE *f, int comm_socket, char *string);
int checkPrefix(char *prefix, char *string);

int main(int argc, char **argv)
{
    int port;
    arguments(argc, argv, &port);

    int welcome_socket;
    int rc;

    struct sockaddr_in sa;
    memset(&sa,0,sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);

    struct sockaddr_in sa_client;    
    memset(&sa_client,0,sizeof(sa_client));
    int sa_client_len;


    if ((welcome_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }
    if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) <0)
    {
        perror("ERROR: bind");
        exit(EXIT_FAILURE);
    }

    if ((listen(welcome_socket, 1)) <0)
    {
        perror("ERROR: listen");
        exit(EXIT_FAILURE);
    }

    while(42)
    {
        int comm_socket = accept(welcome_socket, (struct sockaddr*)&sa_client,&sa_client_len);
        if (comm_socket > 0)
        {
            msg incoming_message;
            int res = 0;

            char type;
            char string[256];
            
            while (1)
            {
                res = recv(comm_socket, (char *)&incoming_message,sizeof(incoming_message),0);

                if (incoming_message.type == TYPE)
                {
                    type = incoming_message.string[0];
                } else if (incoming_message.type == SEARCH)
                {
                    memcpy(string, incoming_message.string, sizeof(incoming_message.string));

                    FILE *f;
                    f = fopen("/etc/passwd", "r");
                    msg message;
                    char *str;
                    if (type == N || type == F) {
                        // vyhledan
                        str = getNF(f, string,type);
                        str[strlen(str)] = '\0';
                        message.type = DATA;
                        memcpy(message.string, str,sizeof(char)*strlen(str));
                        send(comm_socket, (char *)&message, sizeof(message),0);
                        memset(message.string, '\0', sizeof message.string);
                        memset(str, '\0', sizeof(char)*strlen(str));
                        free(str);
                    } else if (type == L) {
                        getL(f, comm_socket, string);
                    } else {
                        perror("ERROR: recv unknown msg type");
                        fclose(f);
                        exit(EXIT_FAILURE);
                    }
                    message.type = END;
                    send(comm_socket, (char *)&message, sizeof(message),0);
                    fclose(f);
                } else if (incoming_message.type == END) 
                {
                    break;
                }
            }
        }
    }

    return 0;
}

// spracovani argumentu
void arguments(int argc, char **argv, int *port)
{
    int c;
    while ((c = getopt(argc, argv, "p:")) != -1)
        switch(c)
        {
            case 'p':
                *port = atoi(optarg);
                break;
            default:
                perror("ERROR: wrong arguments ./ipk-server -p port");
                exit(EXIT_FAILURE);
                break;
        }
}

// vytazeni dat s /etc/passwd pro argumenty -l
// funkce je rovnou posila klientovi
void getL(FILE *f, int comm_socket, char *string) {
    char name[32], info[256], folder[256], t1[64];
    int c1,c2;
    int i;
    msg message;

    while (fscanf(f, "%[^:]:x:%d:%d:%[^:]:%[^:]:%[^\n]\n",name,&c1,&c2,info,folder,t1) != EOF) {
        // na konci radku obcas chybi retezec (/bin/ksh.) a fscanf
        // nacte /n do jmena v dalsim radku, toto zajisti odstraneni /n
        if (name[0] == '\n') {
            memmove(&name[0], &name[1], strlen(name)-1);
            name[strlen(name)-1] = '\0';
        }


        if (checkPrefix(string, name)) {
            name[strlen(name)] = '\0';

            message.type = DATA;
            memcpy(message.string, name, sizeof(char)*strlen(name));
            send(comm_socket, (char *)&message, sizeof(message),0);
        }
        memset(info, '\0', sizeof info);
        memset(name, '\0', sizeof name);
        memset(message.string, '\0', sizeof message.string);
    }
}

// funkce overi jestli je dany prefix na zacatku daneho retezce
int checkPrefix(char *prefix, char *string) {
    for (int i = 0; i < strlen(prefix); i++) {
        if (prefix[i] != string [i])
            return 0;
    }
    return 1;
}

// vytazeni dat s /etc/passwd pro argumenty -n a -f
char *getNF(FILE *f, char *string, char type)
{
    char name[32], info[256], folder[256], t1[64]; 
    int c1,c2;
    int i;
    do {
        memset(info, '\0', sizeof info);
        memset(name, '\0', sizeof name);
        memset(folder, '\0', sizeof folder);
        i = fscanf(f, "%[^:]:x:%d:%d:%[^:]:%[^:]:%[^\n]\n",name,&c1,&c2,info,folder,t1);

        if (i == EOF)
            break;

        // na konci radku obcas chybi retezec (/bin/ksh.) a fscanf
        // nacte /n do jmena v dalsim radku, toto zajisti odstraneni /n
        if (name[0] == '\n') {
            memmove(&name[0], &name[1], strlen(name)-1);
            name[strlen(name)-1] = '\0';
        }
    } while (strcmp(string, name) != 0);
    
    char *r = NULL;
    if (type == 'n') {
        r = malloc(sizeof(char)*strlen(info)+1);
        memset(r, '\0', sizeof r);
        memcpy(r,info, strlen(info));
    } else if (type == 'f') {
        r = malloc(sizeof(char)*strlen(folder)+1);
        memset(r, '\0', sizeof r);
        memcpy(r,folder, strlen(folder));
    }
    return r;
}
