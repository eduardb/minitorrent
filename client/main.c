#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "../common.h"

struct msgnodlst_t noduri;
int nrNoduri;
char fisier[100];
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void * service (void *p)
{
    int index = *(int *)p;
    int sd, ok=1, i = 0, n;
    long offset;
    struct msggetfile_cton_t msggetfile_cton;
    struct msggetfile_ntoc_t msggetfile_ntoc;
    FILE* f;

    strcpy(msggetfile_cton.filename, fisier);
    msggetfile_cton.mtype = 4;


    f = fopen(fisier, "wb");
    do {

        sd = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(sd, (struct sockaddr*)&noduri.addr[index], sizeof(noduri.addr[index])) < 0)
        {
            printf("step %d; cannot connect to %dth node, errno %d \n", i+1, index+1, errno);
            pthread_exit(0);
        }

        offset = (index  + i * nrNoduri) * 100;
        msggetfile_cton.from = htonl(offset);
        n = sizeof(struct msggetfile_cton_t);
        n = htons(n);
        send(sd, &n, sizeof(short), 0);
        send(sd, &msggetfile_cton, ntohs(n), 0);

        recv(sd, &n, sizeof(short), 0);
        n = ntohs(n);
        recv(sd, &msggetfile_ntoc, n, 0);

        if (msggetfile_ntoc.mtype != 5)
        {
            printf("s-a primit un pachet incorect\n");
            continue;
        }
        int citit = ntohl(msggetfile_ntoc.length);

        if (citit > 0)
        {
            pthread_mutex_lock(&mut);
            fseek(f, offset, SEEK_SET);
            fwrite(msggetfile_ntoc.data,1, citit, f);
            pthread_mutex_unlock(&mut);
        }
        else
            ok = 0;
        close (sd);
        i++;
    } while (ok);
    fclose(f);
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    int sd, i, p[10];
    struct hostent *hp;
    short port, n;
    struct sockaddr_in hub;
    struct msggetnod_t msggetnod;
    pthread_t threads[10];

    if (argc < 4)
    {
        printf("Rulati cu parametrii <hup-ip> <hub-port> <file-name>");
        return 1;
    }

    hp = gethostbyname(argv[1]);
    if (hp==NULL)
    {
        printf("%s: unknown host", argv[1]);
        return 1;
    }

    port = atoi(argv[2]);

    memset((char *)&hub, 0, sizeof(hub));
    hub.sin_family = hp->h_addrtype;
    memcpy(&hub.sin_addr.s_addr, hp->h_addr, hp->h_length);
    hub.sin_port = htons(port);

    sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd < 0)
    {
        printf("cannot open socket");
        return 2;
    }

    if (connect(sd, (struct sockaddr*)&hub, (socklen_t)sizeof(hub)) < 0)
    {
        printf("cannot connect");
        return 3;
    }
    n = ntohs(sizeof(struct msggetnod_t));
    send(sd, &n, sizeof(short), 0);
    msggetnod.mtype = 2;
    send(sd, &msggetnod, sizeof(struct msggetnod_t), 0);

    recv(sd, &n, sizeof(short), 0);
    n = ntohs(n);

    recv(sd, &noduri, n, 0);

    if (noduri.mtype != 3)
    {
        printf("%c", noduri.mtype );
        return 3;
    }

    nrNoduri = (n-sizeof(char))/sizeof(struct sockaddr_in);
    printf("s-a primit o lista de %d noduri\n", nrNoduri);

    strcpy(fisier,argv[3]);

    for (i = 0; i < nrNoduri; i++)
    {
        p[i] = i;
        pthread_create(&threads[i], NULL, service, &p[i]);
    }

    for (i = 0; i < nrNoduri; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("S-a terminat de citit fisierul\n");

    return 0;
}
