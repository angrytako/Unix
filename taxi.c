#define _GNU_SOURCE
#include "pacchetto.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>

/*SEM_UNDO*/
void handle_alarm(int signal);
void handle_interrupt(int signal);
void handle_term(int signal);       /*handler per quando il padre termina bruscamente*/
void handle_master_call(int signal); /*questo handler si occupa del movimento del taxi*/
int rand_i(int ind, int length);
void move(int i, int j);
void restart(void);
char corsa_abortita=0, usr_interrupt=0;
int *stat_cond,size_h, size_w, sem_id, nr_taxi, tempo_vita, taxi_id, semStat;
Pos *pos;
cella **mappa;
int main(int argv, char**args){
    char path[50];
    struct sembuf sem;
    struct sigaction sa, sb, sc, sd;
    sigset_t mask, old_mask;
    int *keys, key, key_stat, key_pos, prov_pos;
    int i, fd;

    prctl(PR_SET_PDEATHSIG, SIGTERM);    /*se il parent muove, muore anche lui, e rimuove i semafori*/

        /*settaggio dei vari hendler*/

    signal(SIGALRM,handle_alarm);

    bzero(&sa, sizeof(sa));
    sa.sa_handler = handle_term;
    sigaction(SIGTERM, &sa, NULL);

    bzero(&sb, sizeof(sb));
    sigfillset(&sb.sa_mask);            /*quando c'è un segnale di interrupt blocco tutti gli altri segnali,
                                        tanto è finito il programma*/
    sigdelset(&sb.sa_mask,SIGTERM); /*non voglio bloccare sigterm, se il master è stato ucciso
                                        mentre sono in sigint*/
    sb.sa_handler=handle_interrupt;
    sigaction(SIGINT, &sb, NULL);

    sigemptyset (&mask);                        /*maschero le signal gia' mascherate + SIGUSR1 (signal per muovermi)*/
    sigaddset (&mask, SIGUSR1);                 /* perché non sono pronto a muovermi*/
    sigprocmask (SIG_BLOCK, &mask, &old_mask);   /*tale maschera verrà tolta soltanto nei momenti opportuni*/
                                                /*old mask e' la vecchia maschera e mi servirà per sbloccare SIGUSR1*/


    bzero(&sc, sizeof(sc));                     /*handler del signal SIGUSR1, che il master manda al taxi per far partire una corsa*/
    sc.sa_handler = handle_master_call;
    sigaction(SIGUSR1, &sc, NULL);

    size_w=atoi(args[0]);
    size_h=atoi(args[1]);
    sem_id=atoi(args[2]);
    key_stat=atoi(args[3]);
    key_pos=atoi(args[4]);
    tempo_vita=atoi(args[5]);
    nr_taxi=atoi(args[6]);
    semStat=atoi(args[7]);
    taxi_id=atoi(args[8+size_h]);
    /*scrittura sul file per debugging*/
    sprintf(path,"./taxi_logs/taxi%d.txt",taxi_id);
    fd=open(path, O_RDWR | O_CREAT | O_TRUNC,0600);
#ifdef DEV
     PRINT_ERROR
#endif
    close(0);               /*i taxi non hanno bisongno dell'input da tastiera*/
    dup2(fd,1);
    dup2(fd,2);
    close(fd);
    /*fine parte per il debugging*/

    dprintf(1,"taxi boi\n");
    dprintf(1,"(child):WIDTH=%d HEIGHT=%d Taxi_id:%d Sem_id:%d key_stat:%d key_pos:%d\n",size_w,size_h,taxi_id,sem_id,key_stat,key_pos);
    /*linking statistiche condivise e dell'array dei taxi*/
    stat_cond=shmat(key_stat , NULL ,0);
#ifdef DEV
	PRINT_ERROR
#endif
    shmctl(key_stat, IPC_RMID, NULL);
    pos=shmat(key_pos , NULL ,0);
#ifdef DEV
	PRINT_ERROR
#endif
    shmctl(key_pos, IPC_RMID, NULL);

 /*linking mappa*/
    keys=malloc(size_h*sizeof(int));
    for(i=0;i<size_h;i++)
            keys[i]=atoi(args[8+i]);
    mappa=linking(keys,size_h);
    mark(keys,size_h);

    alarm(tempo_vita);/*inizio il countdown del tempo di vita*/

    /*faccio subito la wait sul semaforo della mia casella inizale*/
    bzero(&sem, sizeof(sem));
    sem.sem_num=(unsigned short) pos[taxi_id].posizione;
    sem.sem_op=-1;
    if(semop (sem_id,&sem,1)==-1){
        dprintf(2,"ERRORE POSIZIONAMENTO INIZIALE TAXI!!\n");/*vuol dire che è partito il handler, perché era stata assegnata una cella piena*/
        exit(0); }
    pos[taxi_id].destinazione=-1;
    /*sono pronto a ricevere destinazioni effettive, ma il signal non verrà recepito
                                    fino a quando non arrivo a sigsuspend, grazie alla maschera che ho impostato*/
#ifdef DEV
	PRINT_ERROR
#endif
    /*uso una funzione per entrare in stato di waiting e rimanerci fino a quando uno dei segnali della maschera non mi sveglia*/
    while(1){/*loop che mi fa rientrare in stato di waiting ogni volta che vengo risvegliato*/

        while (usr_interrupt==0) {/*se non vengo sevegliato da SIGUSR1 ritorno a dormire*/
            sigsuspend(&old_mask); /*sigsuspend mi permette di ricevere SIGUSR1 di nuovo, e nel mentre e' in pausa*/
            if(usr_interrupt==0)
                dprintf(1,"stavo dormendo!\n");
                             }       /*interrotto sigsuspend, il segnale SIGUSR1 torna ad essere bloccato*/

        usr_interrupt=0; /*variabile globale che mi permette di sapere se sigsuspend e' stato svegliato
                            dalla signal del user. Ogni volta che eseguo il loop la imposto a 0,
                            così il handler la puo' mettere a 1*/

        if(corsa_abortita)/*questo viene impostato ad uno quando, appunto, sono finito qui dopo una corsa andata a male,
                            corsa che viene eseguita dal handler della usersignal1*/
        {
            corsa_abortita=0;
            pos[taxi_id].source=-1;
            alarm(tempo_vita);
            pos[taxi_id].destinazione=-1; /*segnalo al master che sono pronto a ricevere una nuova destinazione e ritorno*/
        }
        else{
          alarm(tempo_vita);
          pos[taxi_id].destinazione=-1; /*segnalo al master che sono pronto a ricevere una nuova destinazione e ritorno*/
        }
    }
    exit(-1);
}



void handle_alarm(int signal){
    dprintf(1,"E' partito alarm\n__________________________________________________________________________\n");
    alarm(tempo_vita);
/*dopo le modifiche, tutto ciò che questo handler deve fare, alla fine, e' reimpostare alarm*/
/*se il processo era bloccato sul semaforo, errno verra' settato a EINTR e dopo ogni chiamata bloccante c'è un test di tale errore o c'è
un loop per farlo ripartire, nel caso si trattasse di un semaforo non a rischio di deadlock*/
/*se il processo non era bloccato, allora posso semplicemente lasciarlo proseguire, dato che il taxi doveva esplodere
solo per evitare il deadlock*/
        return;

}


void handle_interrupt(int signal){
    struct sembuf sem;
    bzero(&sem, sizeof(sem));
    dprintf(1,"Fine programma\n");
    /*corse non portate a buon fine per terminazione del programma*/
    if(pos[taxi_id].destinazione!=-1){
        /*stat condivise*/
        sem.sem_num=(unsigned short)size_h*size_w+1;
        sem.sem_op=-1;
    /*avendo settato la maschera dell'interrupt in modo che blocchi tutti i signal, semop è indisturbato*/
        semop(sem_id,&sem,1);
        stat_cond[1]++; /*corse fallite*/
        sem.sem_op=1;
        semop (sem_id,&sem,1);
        /*fine sezione critica*/
        }
    close(1);
    exit(0);
}

void handle_term(int signal){
    if(semctl(sem_id, 0, IPC_RMID)!=-1)           /*il primo taxi che prova a deallocare il semaforo*/
        dprintf(2,"Terminazione innaspettata del Master!\n");   /* segnala l errore. Gli altri falliscono*/

    semctl(semStat, 0 , IPC_RMID);
    exit(-1);
}


void handle_master_call(int signal){
    int meta,i;
    int d_x,d_y,p_x,p_y;

    dprintf(1,"Inizio corsa!\n______________________________________________________________________________________________\n");
    usr_interrupt=1;/*variabile globale che fa sapere al main che questo handler e' stato eseguito*/
    if(pos[taxi_id].source<0 || pos[taxi_id].destinazione<0){   /*non dovrei essere qui se il master non mi ha impostato la destinazione e la source*/
        dprintf(2,"Dati di destinazione errati per il taxi nr. %d",taxi_id);
        exit(-1);
        }
    for(i=0;i<2;i++){   /*bisogna andare fino alla source e poi fino alla destinazione*/
      /*scelta di dove andare*/
      /*quando il taxi arriva alla source imposta la source a -1*/
        if(pos[taxi_id].source!=-1)
            meta=pos[taxi_id].source;
        else
            meta=pos[taxi_id].destinazione;

        /*inizio loop di pathfinding, basato su automa a stati finiti*/
            ind_sep(&d_y,&d_x,meta,size_w);
            while(pos[taxi_id].posizione!=meta){
                /*l'unico posto in cui posso essermi bloccato, e necessitato di riavvio è una move,
                 e in particolare sulla richiesta di semaforo della move. Se ciò è avvenuto la move ha fatto return,
                 e quindi mi trovo qui, oppure c'è stata una seconda move dopo, ma controllo anche quella nel codice*/
                if(corsa_abortita) /*vuol dire che ho ricevuto una signal quando ero bloccato su un semaforo*/
                    break; /*esco dal ciclo di movimento verso una meta*/
                ind_sep(&p_y,&p_x,pos[taxi_id].posizione,size_w);
                dprintf(1,"Attuale x:%d y:%d\nDestinazione x:%d y:%d\n___________________________________________________\n",p_x,p_y,d_x,d_y);

                if(d_x<p_x){
                        if(mappa[p_y][p_x-1].occupata==1){
                            if(d_y<p_y){
                                move(p_y-1,p_x);
                                        }
                            else if(d_y==p_y){
                                move(rand_i(p_y,size_h),p_x);
                                            }
                            else{
                                move(p_y+1,p_x);
                                }
                            }
                        else{
                            move(p_y,p_x-1);
                    }
                }

                else if(d_x==p_x){
                        if(d_y>p_y){
                            if(mappa[p_y+1][p_x].occupata==1){
                                move(p_y,rand_i(p_x,size_w));
                                if(corsa_abortita) /*vuol dire che ho ricevuto una signal quando ero bloccato su un semaforo*/
                                    break;
                                dprintf(1,"Attuale x:%d y:%d\nDestinazione x:%d y:%d\n___________________________________________________\n"
                                ,pos[taxi_id].posizione%size_w,p_y,d_x,d_y);
                                move(p_y+1,pos[taxi_id].posizione%size_w);
                            }
                            else{
                                move(p_y+1,p_x);
                            }
                        }
                        else if(d_y<p_y){
                            if(mappa[p_y-1][p_x].occupata==1){
                                move(p_y,rand_i(p_x,size_w));
                                if(corsa_abortita) /*vuol dire che ho ricevuto una signal quando ero bloccato su un semaforo*/
                                    break;
                                dprintf(1,"Attuale x:%d y:%d\nDestinazione x:%d y:%d\n___________________________________________________\n"
                                ,pos[taxi_id].posizione%size_w,p_y,d_x,d_y);
                                move(p_y-1,pos[taxi_id].posizione%size_w);
                            }
                            else{
                                move(p_y-1,p_x);
                            }
                        }
                        else{
                            dprintf(2,"C'è stato un errore nel pathfinding del taxi!\n");
                            exit(-1);
                        }
                }
                else{
                        if(mappa[p_y][p_x+1].occupata==1){
                            if(d_y<p_y){
                                move(p_y-1,p_x);
                                        }
                            else if(d_y==p_y){
                                move(rand_i(p_y,size_h),p_x);
                                            }
                            else{
                                move(p_y+1,p_x);
                                }
                            }
                        else{
                            move(p_y,p_x+1);
                    }

                }



        }

        if(pos[taxi_id].source==-1 && corsa_abortita==0){ /*vuol dire che sono alla fine del secondo giro */
            /*di impostare la destinazione a -1 ci pensa il loop del main*/
            return;
        }
        else if(pos[taxi_id].source!=-1 && corsa_abortita==0){/*sono alla fine del primo giro*/
            pos[taxi_id].source=-1;
        }
        if(corsa_abortita==1){  /*concludo la funzione handler, dato che mi sono riposizionato
                                    del resto si occupa il loop principale del programma, quello dove dorme*/
                                /*le stat sui viaggi abortiti sono aggiornate da riavvia()*/
            return;
        }
	}




}


void move(int i, int j){
    int index, last_pos, prev_i, prev_j;
    struct timespec time;
    struct sembuf sem,spec_sem[2];
    /*settaggio del tempo di sleep a quello della cella nella quale mi voglio spostare*/
    bzero(&sem, sizeof(sem));
    bzero(spec_sem,sizeof(spec_sem[0]));
    bzero(spec_sem+1,sizeof(spec_sem[0]));
    time.tv_sec=0;
    time.tv_nsec=mappa[i][j].tempo;
    /*preparazione versione linearizzata della destinazione*/
    index=trasf_indice(i,j, size_w);
    /*se sto tentando di spostarmi su una cella buco, c'è stato un errore nel path-finder*/
    if(mappa[i][j].occupata==1)
        {
            dprintf(2,"Errore, tentativo di accesso del taxi %d alla casella %d,%d la quale e' un buco!\n",taxi_id,j,i);
            exit(-1);
        }

    /*parte inerente alla coordinazione master-taxi*/
    spec_sem[0].sem_num=size_w*size_h;
    spec_sem[0].sem_op=0;                  /*semaforo del permesso di fare operazioni, inizializzato a 0*/
                                            /*aspetto 0; l unico che può averlo tolto da 0 è il master, per fare la stampa*/
    spec_sem[1].sem_num=size_w*size_h+1;    /*semaforo di quanti taxi stanno svolgendo un operazione al momento*/
    spec_sem[1].sem_flg=SEM_UNDO;           /*se mai andasse in errore un taxi, deve rilasciare questa risorsa, altrimenti
                                            bloccherebbe tutta la simulazione*/
    spec_sem[1].sem_op=+1;



    /*preparo la richiesta al semaforo della destinazione*/
    bzero(&sem, sizeof(sem));
    sem.sem_num=(unsigned short)index;
    sem.sem_op=-1;

    /*richiesta al semaforo*/
        /*punto critico in cui se aviene la chiamata del handler bisogna riavviare il processo*/
    if(semop(sem_id,&sem,1)==-1){/*vuol dire che è arrivata una signal (alarm; se sono sigint o sigterm questo if non importa)*/
       while(semop(semStat, spec_sem, 2)==-1){PRINT_ERROR}	/*in un operazione atomica chiedo il permesso sul semaforo di coordinazione
                                        e aggiorno quello che conta il num dei taxi al momento impegnati in un'operazione */

        last_pos=pos[taxi_id].posizione;    /*mi salvo la posizione attuale, perché verrà impostata a -1, così il taxi*/
        ind_sep(&i,&j,last_pos,size_w);     /*non verrà stampato su quella cella dal master*/

    /*devo accedere alle stat condivise della mappa per togliere il taxi dalla posizione nella quale si trovava*/
        sem.sem_num=pos[taxi_id].posizione;
        sem.sem_op=-1;
        while(semop (semStat,&sem,1)==-1){PRINT_ERROR}  /*stat condivise cella mappa*/
                                                        /*come al solito, non ci dovrebbe essere rischio di deadlock*/
        mappa[i][j].cur_num_taxi--;   /*num taxi presenti nella cella nella quale sono esploso*/

        sem.sem_op=1;
        semop (semStat,&sem,1);

         /*stat condivisa sulle corse fallite*/
        sem.sem_num=(unsigned short)size_h*size_w+1;
        sem.sem_op=-1;

        while(semop (sem_id,&sem,1)==-1){PRINT_ERROR}   /*stat condivise array*/

        stat_cond[1]++; /*corse fallite*/
        sem.sem_op=1;
        semop (sem_id,&sem,1);

        pos[taxi_id].posizione=-1;

    /*restituisco la risorsa della posizione in cui sono esploso*/
        sem.sem_num=(unsigned short)last_pos;
        sem.sem_op=+1;
        semop(sem_id,&sem,1);
    /*fine sezione critica*/
    /*avverto il master che ho finito le operazioni che alterano posizione o stat importanti*/
        spec_sem[1].sem_op=-1;
        while(semop(semStat, &spec_sem[1],1)==-1){PRINT_ERROR}	/*non voglio che un handler rovini questa operazione*/

        restart();
        return;
    }


    alarm(tempo_vita);/*reimposto il timer, visto che ci siamo ufficialmente mossi*/
    last_pos=pos[taxi_id].posizione;    /*mi serve ricordarmi la pos precedente per rilasciare il semaforo dove ero*/
    ind_sep(&prev_i,&prev_j,last_pos,size_w);   /*mi serve per diminuire il numero di taxi nella cella in cui ero (stat della struct cella)*/

    /*update della pos del taxi e della stat personale sulle celle attraversate e stat globali*/
    while(semop(semStat, spec_sem, 2)==-1){PRINT_ERROR}	/*in un operazione atomica chiedo il permesso sul semaforo di coordinazione
                                        e aggiorno quello che conta il num dei taxi al momento impegnati in un'operazione */

    sem.sem_num=index; /*semaforo fatto per accedere alle statistiche condivise di una particolare cella*/
    sem.sem_op=-1;                                  /*metto nella stessa sezione critica anche le stat individuali, così non*/
    while(semop (semStat,&sem,1)==-1){PRINT_ERROR} /*rischio dati inconsistenti (posizione in pos vs cur_num_taxi in cella)*/
                                        /*come al solito, non ci dovrebbe essere rischio di deadlock*/
    mappa[i][j].tot_taxi++;             /*num taxi che hanno attraversato questa cella*/
    mappa[i][j].cur_num_taxi++;         /*num taxi presenti nella cella in cui sono entrato*/
    pos[taxi_id].posizione=index;
    pos[taxi_id].Max_celle++;             /*celle attraversate da questo taxi*/

    sem.sem_op=1;
    semop (semStat,&sem,1);             /*rilascio il semaforo delle stat della cella in cui mi sono posizionato*/

    sem.sem_num=last_pos; /*semaforo fatto per accedere alle statistiche condivise della cella che ho lasciato*/
    sem.sem_op=-1;          /*metto nella stessa sezione critica anche le stat individuali, così non*/
    while(semop (semStat,&sem,1)==-1){PRINT_ERROR}

    mappa[prev_i][prev_j].cur_num_taxi--;   /*num taxi presenti nella cella dalla quale sono uscito*/

    sem.sem_op=1;
    semop (semStat,&sem,1);
    /*aggiorno anche le stat sui clienti serviti qui(nel caso sia arrivato ad una source)
    o i dati sulle corse completate, dato che il master mi sta ancora aspettando, anche nel caso sia il fine programama*/

    if(pos[taxi_id].source!=-1 && pos[taxi_id].posizione==pos[taxi_id].source){
        pos[taxi_id].clienti_serviti++;
        dprintf(1,"Source Raggiunta!\n___________________________________________________\n");
        }
    else if(pos[taxi_id].source==-1 && pos[taxi_id].posizione==pos[taxi_id].destinazione)
        {
            dprintf(1,"Successo!!!\n___________________________________________________\n");
            /*stat condivisa sulle corse completate*/
            sem.sem_num=(unsigned short)size_h*size_w;
            sem.sem_op=-1;

            while(semop(sem_id,&sem,1)==-1){PRINT_ERROR}
            stat_cond[0]++;
            sem.sem_op=1;
            semop (sem_id,&sem,1);
            /*fine sezione critica*/


        }

    /*parte di coordinazione master-taxi (ho finito le operazioni)*/
    spec_sem[1].sem_op=-1;
    while(semop(semStat, &spec_sem[1],1)==-1){PRINT_ERROR}	/*non voglio che un handler rovini questa operazione*/


    /*rilascio del semaforo sulla cella in cui ero in precedenza*/
    sem.sem_num=last_pos;
    sem.sem_op=1;
    semop (sem_id,&sem,1);

    nanosleep(&time, NULL); /*il tempo di attraversamento della cella*/

    spec_sem[0].sem_op=0;					/*altra stat da coordinare con il master (tempo totale attraversamento)*/
    spec_sem[1].sem_op=+1;
    spec_sem[1].sem_flg=SEM_UNDO;
    while(semop(semStat, spec_sem, 2)==-1){PRINT_ERROR}
    pos[taxi_id].Max_tempo+=mappa[i][j].tempo; /*aggiungo il tempo che ci ho messo ad attraversare a quello totale del taxi*/

    spec_sem[1].sem_op=-1;
    while(semop(semStat, spec_sem+1,1)==-1){PRINT_ERROR}
    /*fine coordinazione master taxi*/
    return;
}


int rand_i(int ind, int length){
    if(ind==0)
        return ind+1;
    else if(ind==length-1)
        return ind-1;
    else
        return ind+1;
}



void restart(){
        int new_pos,i,j;
        struct sembuf sem;
        struct sembuf spec_sem[2];
          /*imposto corsa aborita ad uno, così il loop di path-finding si ferma ed anche quello principale
        della funzione hnadler del master*/
        corsa_abortita=1;

        bzero(spec_sem,sizeof(spec_sem[0]));
        bzero(spec_sem+1,sizeof(spec_sem[0]));

        spec_sem[0].sem_num=size_w*size_h;		  /*semaforo del num di operazioni; e' grosso quanto il num taxi, per cui*/
        spec_sem[0].sem_op=0;					/*l'unico modo in cui puo' fare waiting e' se il master lo ha azzerato*/

        spec_sem[1].sem_num=size_w*size_h+1;
        spec_sem[1].sem_op=+1;
        spec_sem[1].sem_flg=SEM_UNDO;
        dprintf(1,"Tempo massimo raggiunto per un taxi\n___________________________________________________\n");


        sem.sem_num=(unsigned short)size_h*size_w+2; /*questo è un semaforo che ho creato (in master_lib)*/
        sem.sem_op=-1;                                 /*perché ho notato che, se ci sono troppi taxi esplosi*/
        while(semop (sem_id,&sem,1)==-1);              /*che provano contemporaneamente a cercare una nuova posizione,*/
                                                        /*essi tendono a rallentarsi a vicenda*/

          /*faccio la richiesta del semaforo sulla mia nuova posizione*/
        do{
        new_pos=rand_pos(mappa,pos, nr_taxi, size_w, size_h); /*nuova posizione con funzione teoricamente ottimizzata*/

        sem.sem_num=(unsigned short)new_pos;
        sem.sem_flg=IPC_NOWAIT; /*non voglio rimanere bloccato sul semaforo; piuttosto cerco un altra posizione*/
                                /*e' comunque molto probabile che la posizione scelta sia "libera", visto come funziona rand_pos*/
        sem.sem_op=-1;
        PRINT_ERROR
            /* VA MODIFICATA, PERCHE' COSI COME E' NON E' GARANTITA CHE FUNZIONI*/
        }while(semop (sem_id,&sem,1)==-1);
        /*restituisco il semaforo dei taxi esplosi*/
        sem.sem_flg=0;
        sem.sem_num=(unsigned short)size_h*size_w+2;
        sem.sem_op=1;
        semop (sem_id,&sem,1);      /*semaforo per taxi esplosi che cercano di riposizionarsi*/

        ind_sep(&i,&j,new_pos,size_w);
        alarm(tempo_vita);

        sem.sem_num=new_pos;
        sem.sem_op=-1;

        while(semop(semStat, spec_sem, 2)==-1){PRINT_ERROR} /*coordinamento taxi-master*/
        while(semop (semStat,&sem,1)==-1){PRINT_ERROR}  /*cella della mappa le cui stat si vuole aggiornare*/
                                            /*come al solito, non ci dovrebbe essere rischio di deadlock*/
        mappa[i][j].cur_num_taxi++;             /*num taxi che sono presenti nella cella in cui mi sono riposizionato*/
                                            /*non ha senso aggiornare il num dei taxi che hanno attraversato questa cella*/
        pos[taxi_id].posizione=new_pos;

        /*fine sezione critica*/
        sem.sem_op=1;           /*restituisco risorsa condivisa cella*/
        semop (semStat,&sem,1);
        /*coordinazione master-taxi (fine)*/
        spec_sem[1].sem_op=-1;

        while(semop(semStat, spec_sem+1,1)==-1){PRINT_ERROR}	/*non voglio che un handler rovini questa operazione*/



        /*stampa della nuova pos su file per nostro utilizzo*/
        ind_sep(&i,&j,pos[taxi_id].posizione,size_w);
        dprintf(1,"posizione reimpostata a: %d,%d\n___________________________________________________\n",j,i);

        return;

}
