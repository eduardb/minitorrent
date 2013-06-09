#ifndef _COMMON_H_
#define _COMMON_H_

//#include <sys/socket.h>
#include <netinet/in.h>

/*
Inregistrare:
  - Trimis de noduri spre hub
  - mtype = 1
  - rvport = portul pe care nodul accepta conexiuni de la clienti
  */
struct msgreg_t {
        char mtype;
        short rvport;
    };


/*
Cerere lista de noduri:
  - Trimis de clienti spre hub
  - mtype = 2
*/
    struct msggetnod_t {
        char mtype;
    };

/*
Lista de noduri:
  - Trimis de hub clientului care cere lista
  - mtype = 3
*/
    struct msgnodlst_t {
        char mtype;
        struct sockaddr_in addr[10];
    }; // 164

/*
Cerere parte de fisier:
  - Trimis de client unui nod
  - mtype = 4
*/
    struct msggetfile_cton_t {
        char mtype;
        char filename[100];
        long from;
    }; // 112

/*
Continut fisier:
  - Trimis de nod unui client
  - mtype = 5
  - Variabila length va contine numarul efectiv de octeti trimis. Daca
    cererea se referea la o pozitie dinafara fisierului length va fi 0.
*/
    struct msggetfile_ntoc_t {
        char mtype;
        char data[101];
        int length;
    }; // 108

struct node
{
    struct sockaddr_in nodeInfo;
    struct node* next;
};

#endif
