#define _GNU_SOURCE
#include "pacchetto.h"
#include <stdio.h>
#include <time.h>
#include <sys/prctl.h>
#include <signal.h>
/*source*/
int indice; /*serve per l'handler*/
void handle_tastiera(int signal);
int out[2];
int SO_WIDTH , SO_HEIGHT; 
cella **mappa;
int SO_i,SO_j;
int pipe;
int i,j;

int main(int argv, char**args){
   
    int *keys, key;
    
 
     /*indici della cella attuale*/
    
    int milisec;
    struct timespec req ;
    struct timespec recupero ;
    struct sigaction sc;
    prctl(PR_SET_PDEATHSIG, SIGTERM);


   milisec= 100; /* 100 = 10 al secondo 20=50 al secondo*/
   req.tv_sec = 0;
   req.tv_nsec = milisec * 1000000L;


/*parsing*/
    SO_WIDTH=atoi(args[0]);
    SO_HEIGHT=atoi(args[1]);
    pipe =atoi(args[2]);
    indice=atoi(args[4+SO_HEIGHT]);

    ind_sep(&SO_i,&SO_j,atoi(args[3]),SO_HEIGHT);

        /*mappa*/
    keys=malloc(SO_HEIGHT*sizeof(int));
    for(i=0;i<SO_HEIGHT;i++)
            keys[i]=atoi(args[4+i]);

    mappa=linking(keys,SO_HEIGHT);
    mark(keys,SO_HEIGHT);

        bzero(&sc, sizeof(sc));
        sc.sa_handler = handle_tastiera;
        sigaction(SIGUSR1, &sc, NULL);

        srand (getpid());
        i=0;
        j=0;
        out[1]= indice;
        while (1)
        {
                i=rand()%SO_HEIGHT;
                j=rand()%SO_WIDTH;
                if (i!=SO_i && j!= SO_j && mappa[i][j].occupata==0){  
                        out[0]=trasf_indice(i,j,SO_WIDTH); 
                        if(i!=-1){
                                write(pipe,&out,2*sizeof(int)); 
                                
                                req.tv_nsec = (rand()%milisec)+20* 1000000L;
                                if(nanosleep(&req, &recupero)==-1) nanosleep(&recupero,NULL);                                  
                        } 
                        else printf("errore contollato nelle source%d\n",indice);
                }
        }
}


void handle_tastiera(int signal){
        srand (getpid());
        do{
                i=rand()%SO_HEIGHT;
                j=rand()%SO_WIDTH;
        }while ((i==SO_i && j== SO_j) || mappa[i][j].occupata==1);
       

        /*ho la mia destinazione i j */
        out[0]=trasf_indice(i,j,SO_WIDTH);
        write(pipe,&out,2*sizeof(int)); /*scrivo sulla pipe la destinazione*/
        
        
        i=-1;


}