#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "../common.h"

#define PORT 11092

struct client_params {
    int sock;
    in_port_t port;
    struct in_addr s_addr;
};

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
int noduriConectate = 0;
struct node *noduri = NULL;
int sd;

void cerereListaNoduri(int sock)
{
    struct msggetnod_t msggetnod;
    recv(sock, &msggetnod, sizeof(struct msggetnod_t), 0);

    if (msggetnod.mtype != 2)
        return;

    int n = 0;
    struct node* i;
    struct msgnodlst_t msgnodlst;
    msgnodlst.mtype = 3;

    for (i = noduri; i != NULL; i = i->next)
        msgnodlst.addr[n++] = i->nodeInfo;

    short size = sizeof(char) + noduriConectate * sizeof(struct sockaddr_in);
    size = htons(size);
    send(sock, &size, sizeof(short), 0);
    send(sock, &msgnodlst, ntohs(size), 0);
    close(sock);
}

void inregistrareNod(struct client_params param)
{
    struct msgreg_t msgreg;
    short port;
    int sock = param.sock;
    recv(sock, &msgreg, sizeof(struct msgreg_t), 0);

    if (msgreg.mtype != 1)
        return;

    if  (noduriConectate < 10)
    {
        pthread_mutex_lock(&mut);

        struct sockaddr_in * s = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        memset(s, 0, sizeof(struct sockaddr_in));
        s->sin_port = msgreg.rvport;
        s->sin_family = AF_INET;
        memcpy(&s->sin_addr.s_addr, &param.s_addr, sizeof(s->sin_addr.s_addr));
        struct node* nod = (struct node*)malloc(sizeof(struct node));
        memcpy(&nod->nodeInfo,s,sizeof(struct sockaddr_in));
        if (noduri == NULL)
        {
            noduri = nod;
        }
        else
        {
            nod->next = noduri;
            noduri = nod;
        }

        noduriConectate++;

        pthread_mutex_unlock(&mut);

        printf("S-a inregistrat un nod care ruleaza pe portul %d\n", ntohs(msgreg.rvport));
    }

}

void * service (void * p)
{
    struct client_params param = *(struct client_params*)p;
    int ad = param.sock;
    short n;
    recv (ad, &n, sizeof(short), 0);
    n = ntohs(n);
    switch (n)
    {
        case 1: // Cerere lista de noduri
            cerereListaNoduri(ad);
            break;
        case 4: // Inregistrare
            inregistrareNod(param);
            break;
    }
    close(ad);
    pthread_exit(0);
}

void endSignal(int sig)
{
    close(sd);
    printf("----------\nServer oprit\n");
    exit(0);
}

int main()
{
    int ad, lac, port;
    pthread_t t;
    struct sockaddr_in server, client;
    port = PORT;

    signal(SIGINT, endSignal);

    sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd < 0)
    {
        printf("cannot open socket\n");
        return 1;
    }

    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port =htons(port);

    if (bind(sd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        printf("cannot bind port\n");
        return 2;
    }

    listen(sd, 5);
    printf("Wait at: %d ... \n", port);

    lac =  sizeof( (struct sockaddr *) &client);
    for ( ; ;)
    {
        ad = accept(sd, (struct sockaddr*)&client, (socklen_t *)&lac);
        if (ad < 0)
        {
            printf("cannot accept connection");
            return 3;
        }
        struct client_params *p = (struct client_params *)malloc(sizeof(struct client_params));
        p->sock = ad;
        p->port = client.sin_port;
        p->s_addr = client.sin_addr;
        pthread_create(&t, NULL, service, p);
    }

    return 0;
}
