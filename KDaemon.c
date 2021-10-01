#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include <signal.h>
#include <sys/prctl.h>
#include "pacchetto.h"




int main(int argv, char**args){
    char ch;
    int i,so,*pidSO;
 prctl(PR_SET_PDEATHSIG, SIGTERM);
    
    so = atoi(args[0]);
    pidSO = malloc(so*sizeof(int));
    
    for (i=0;i<so;i++){
        pidSO[i]= atoi(args[i+1]);
    }



    while(1){
        ch=fgetc(stdin);
        if (ch==10){
            printf("genero una richiesta su tutte le Sources  ---");
                    
                    for (i=0;i<so;i++){
                        kill(pidSO[i],SIGUSR1);
                        
                    }
        }
    }
}