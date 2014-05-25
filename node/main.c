#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "../common.h"

// localhost 11092 /home/edy/workspace/l9/data

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void * service (void * p)
{
    int ad = *(int *)p;
    struct msggetfile_ntoc_t msggetfile_ntoc;
    struct msggetfile_cton_t msggetfile_cton;
    long citit;
    short n;
    FILE *f;
    recv (ad, &n, sizeof(short), 0);
    n = ntohs(n);

    recv(ad, &msggetfile_cton, n, 0);

    if (msggetfile_cton.mtype != 4)
    {
        printf("mesaj incorect\n");
        close(ad);
        pthread_exit(0);
    }
    pthread_mutex_lock(&mut);

    f = fopen(msggetfile_cton.filename, "rb");
    fseek(f, ntohl(msggetfile_cton.from), SEEK_SET);

    msggetfile_ntoc.mtype = 5;
    citit = fread(msggetfile_ntoc.data, 1, 100, f);
    msggetfile_ntoc.data[citit] = 0;
    msggetfile_ntoc.length  = htonl(citit);
    fclose(f);

    pthread_mutex_unlock(&mut);

    n = htons(sizeof(struct msggetfile_ntoc_t));
    send(ad, &n, sizeof(short), 0);

    send(ad, &msggetfile_ntoc, ntohs(n), 0);

    close(ad);

    pthread_exit(0);
}

void segvSgn(int sig)
{
    printf("%d", errno);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Rulati cu parametrii <hup-ip> <hub-port> <data-dir>\n");
        return 1;
    }
    signal(SIGSEGV, segvSgn);
    struct hostent *hp;
    struct sockaddr_in hub, client, node;
    short port,n, initialPort = 11292, i;
    int sd, sd2,ad,lac;
    pthread_t t;

    chdir(argv[3]);

    hp = gethostbyname(argv[1]);
    if (hp==NULL)
    {
        printf("%s: unknown host\n", argv[1]);
        return 1;
    }

    port = atoi(argv[2]);

    memset((char *)&hub, 0, sizeof(hub));
    hub.sin_family = hp->h_addrtype;
    memcpy(&hub.sin_addr.s_addr, hp->h_addr, hp->h_length);
    hub.sin_port = htons(port);


    sd = socket(AF_INET, SOCK_STREAM, 0);
    sd2 = socket(AF_INET, SOCK_STREAM, 0);

    if (sd < 0)
    {
        printf("cannot open socket\n");
        return 2;
    }

    /* bind any port number */
    node.sin_family = AF_INET;
    node.sin_addr.s_addr = htonl(INADDR_ANY);

    for (i=0;i<20;i++)
    {
        node.sin_port = htons(initialPort);
        if (bind(sd, (struct sockaddr*)&node, sizeof(node)) < 0)
        {
            printf("cannot bind port %d \n", initialPort);
            //return 3;
        }
        else
            break;
        initialPort++;
    }

    printf("se conecteaza la hub prin portul %d... \n", initialPort);

    if (connect(sd2, (struct sockaddr*)&hub, (socklen_t)sizeof(hub)) < 0)
    {
        printf("cannot connect");
        return 4;
    }

    n = htons(sizeof(struct msgreg_t));

    if (send(sd2, &n, sizeof(short), 0) < 0)
    {
        printf("cannot send data size\n");
        return 5;
    }

    struct msgreg_t reg;
    reg.mtype = 1;
    reg.rvport = node.sin_port;

    if (send(sd2, &reg, sizeof(struct msgreg_t), 0) < 0)
    {
        printf("cannot send data\n");
        return 6;
    }

    close(sd2);

    listen(sd, 5);
    printf("Wait at: %d ... \n", ntohs(node.sin_port));

    lac =  sizeof( (struct sockaddr *) &client);
    for ( ; ;)
    {
        ad = accept(sd, (struct sockaddr*)&client, (socklen_t *)&lac);
        if (ad < 0)
        {
            printf("cannot accept connection");
            return 3;
        }
        pthread_create(&t, NULL, service, &ad);
    }


    return 0;
}
