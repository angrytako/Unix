#include "pacchetto.h" /* file che ti ho creato dove mettere funzioni che servono a più processi*/
/* anche la struct te l ho messa nel .h; ogni struct multi processo mettila lì, e poi includi il file .h*/
#include "master_lib.h" /* libreria del master con serializazione*/
#include <signal.h> /*ho messo molti dei tuoi include ne .h così non vanno dichiarati di nuovo nei processi*/
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
void handle_interrupt(int signal);
void handle_alarm(int signal);
#define SO_WIDTH 60
#define SO_HEIGHT 20
/*variabili grobali*/
cella **mappa; /*array[][] con dentro le struct di una cella*/
int sem_id, SO_SOURCES, SO_TAXI, SO_DURATION, semStat;
Pos *posTaxi;
int sec=0;  /*servono per il print della mappa */
int *pidTaxi, *pidSo, pidKDaemon;
int *stat_cond; /*array statistiche*/
struct sembuf sem;
int pipeSO[2];
int SO_TOP_CELLS;
int main (int argv, char** args) {
    int n, destinazione, in[2]; /*momentaneo per pipe */
    int retval;  /* sempre prova per pipe non bloccante lettura*/
   /*sempre per ciclo pipe*/
    int i_SO, j_SO, i_TAX, j_TAX, distanza, min_distanza;
    struct sigaction sa;
    sigset_t  my_mask;
    FILE* st;
    int fd;

    int SO_HOLES, SO_CAP_MIN, SO_CAP_MAX,
    SO_TIMENSEC_MIN, SO_TIMENSEC_MAX,SO_TIMEOUT;
	char **argt,**argq,**argk;/*la q perchè mi andava */
    int i,j,a,k,count;
    size_t size;
    int key_stat, key_posTaxi; /*chiave dell array di array*/
    int *keys; /* array che conterrà le chiavi dei sottoarray*/
    int *posSO;    /*array con posizione SOurces*/


    /*settaggio dell operzione per semaforo coordinazione master-taxi*/
    bzero(&sem,sizeof(sem));
    /*fine settaggio*/

    st=fopen("./input.txt","r");
#ifdef DEV
    PRINT_ERROR
#endif
 /*lettura delle variabili dal file*/
    deserializzatore(st,&SO_TAXI,&SO_SOURCES, &SO_HOLES, &SO_TOP_CELLS, &SO_CAP_MIN, &SO_CAP_MAX,
    &SO_TIMENSEC_MIN, &SO_TIMENSEC_MAX,&SO_TIMEOUT,&SO_DURATION);
   /* printf("%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",SO_TAXI,SO_SOURCES, SO_HOLES, SO_TOP_CELLS, SO_CAP_MIN, SO_CAP_MAX,
    SO_TIMENSEC_MIN, SO_TIMENSEC_MAX,SO_TIMEOUT,SO_DURATION);*/

            /*handler*/
    sa.sa_handler = handle_interrupt;
	sa.sa_flags = 0;
	sigfillset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);

    signal(SIGALRM, handle_alarm);	/* gestione del segnale SIGALRM  per print mappa ogni secondo*/


/*inizializzare array di descriptor per le pipe, che hanno sia le pipe per taxi che per le sources*/
    pipe(pipeSO);
/*fine inizializzazione pipe*/

     /*dimensiono array con il numero delle SOurces*/
    posSO = malloc(SO_SOURCES*sizeof(int));
    /*allocazione dei due array per i pid dei processi*/
    pidTaxi=malloc(SO_TAXI*sizeof(int));
    pidSo=malloc(SO_SOURCES*sizeof(int));


         /*creazione arraysemafori*/
    sem_id = semget( IPC_PRIVATE , SO_WIDTH*SO_HEIGHT+3, 0600 | IPC_CREAT| IPC_EXCL);
    semStat=  semget( IPC_PRIVATE , SO_WIDTH*SO_HEIGHT+2, 0600 | IPC_CREAT| IPC_EXCL);
     /*ti ho aggiunto 3 semafori in più,2 da usare per stat condivise, ed uno per riposizionamento taxi*/
#ifdef DEV
        PRINT_ERROR
#endif
   /* printf("Sem_id:%d SemStat:%d stat_key:%d pos_key:%d\n",sem_id,semStat,key_stat,key_posTaxi );*/
         /*fine parte sui semafori*/

    /*creazione di memoria condivisa per array di array, ed ogni array interno*/
    keys=malloc(SO_HEIGHT*sizeof(int));
#ifdef DEV
    PRINT_ERROR
#endif
    for(i=0;i<SO_HEIGHT;i++){
        keys[i]=shmget (IPC_PRIVATE, SO_WIDTH*sizeof(**mappa) , 0600 | IPC_CREAT | IPC_EXCL);
#ifdef DEV
    PRINT_ERROR
#endif
        }
/* ora uso la funzione che ho creato per linkare le chiavi ad un puntatore vero*/
    mappa=linking(keys,SO_HEIGHT);

/* se tutto è andato a buon fine da qui in poi mappa contiene il puntatore in memoria della matrice
    per questo processo, mentre per i processi figli dovrai semplicemente ripetere il linking*/
    /*eseguo subito il mark, così se qualcosa va male l SO rimuove la memoria*/
    mark(keys,SO_HEIGHT);

                /*statistiche condivise*/
    key_stat = shmget (IPC_PRIVATE, 2*sizeof(int) , 0600 | IPC_CREAT | IPC_EXCL);  /* viaggi eseguiti con successo e inevasi*/
#ifdef DEV
	PRINT_ERROR
#endif
    stat_cond =shmat(key_stat, NULL ,0);
#ifdef DEV
        PRINT_ERROR
#endif
    shmctl(key_stat, IPC_RMID, NULL);
#ifdef DEV
        PRINT_ERROR
#endif
    stat_cond[0]=0;   /*inizializzo a 0*/ /*corse a buon fine*/
    stat_cond[1]=0;  /*corse abortite*/

            /*fine statistiche condivise*/

    /*array per la posizione e statistiche individuali dei taxi*/
    key_posTaxi= shmget (IPC_PRIVATE, SO_TAXI*sizeof(Pos) ,0600| IPC_CREAT | IPC_EXCL); /*array per posizione taxi e statistiche*/
#ifdef DEV
        PRINT_ERROR
#endif
    posTaxi = shmat(key_posTaxi, NULL, 0);
#ifdef DEV
        PRINT_ERROR
#endif
    shmctl(key_posTaxi, IPC_RMID, NULL);
#ifdef DEV
        PRINT_ERROR
#endif

     /*fine array per la posizione e statistiche individuali dei taxi*/

                    /*creazione mappa*/

    /*retutn 0 in caso di errore nella crezione della mappa */
    /*assegnazione valori caselle mappa*/
    if(creazione_mappa(SO_HEIGHT,SO_WIDTH, mappa, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX,  SO_CAP_MIN,
                     SO_CAP_MAX, SO_HOLES, SO_SOURCES,sem_id,posSO,semStat, SO_TAXI)  == 0) {
                        print_mappa(mappa,SO_HEIGHT,SO_WIDTH);
                        semctl(sem_id, 0 /* ignored */, IPC_RMID);
                        semctl(semStat, 0 /* ignored */, IPC_RMID);
                        exit(-1);
                     }

    /*inizializzo la tabella taxi */
    /*controllo che non mi chiedano di inserire troppi taxi*/
    for (i=0;i<SO_HEIGHT;i++){
        for (j=0;j<SO_WIDTH;j++){
            k += mappa[i][j].Max_taxi;  /*conto il massimo numero di taxi*/
        }
    }

    if ((k*0.8)<=SO_TAXI) {
        printf("error, la simulazione non può funzionare con troppi taxi\n");
        semctl(sem_id, 0 /* ignored */, IPC_RMID);
        semctl(semStat, 0 /* ignored */, IPC_RMID);
        exit(-1);

    }

    /*controllare la che il taxi non superi la dimensione massima*/
    srand (getpid());
    for(i=0;i<SO_TAXI;i++){
        a=rand()% (SO_WIDTH*SO_HEIGHT);
        ind_sep(&j,&k,a,SO_WIDTH);
        while (mappa[j][k].occupata==1 || mappa[j][k].cur_num_taxi>=mappa[j][k].Max_taxi) {
            a=rand()%(SO_WIDTH*SO_HEIGHT);
            ind_sep(&j,&k,a,SO_WIDTH);
        }
        posTaxi[i].posizione = a;
        mappa[j][k].cur_num_taxi++;
        posTaxi[i].destinazione = -2;
        posTaxi[i].source = -1;
        posTaxi[i].Max_celle=0;
        posTaxi[i].clienti_serviti=0;
        posTaxi[i].Max_tempo=0;

    }
            /*prima print della mappa*/
    printf("mappa iniziale\n");
    /*mappa allo stato iniziale*/
    print_mappa(mappa,SO_HEIGHT,SO_WIDTH);





	/*preparazione degli args per i taxi*/
	/*il numero di argomenti deve essere quello effettivo+1, in quanto l ultimo arg DEVE essere NULL*/
    /*init_arg si seeta in automatico l ultimo a NULL*/
    argt=init_arg(9+SO_HEIGHT+1,100);
    sprintf(argt[0],"%d",SO_WIDTH);
    sprintf(argt[1],"%d",SO_HEIGHT);
    sprintf(argt[2],"%d",sem_id);
    sprintf(argt[3],"%d",key_stat);
    sprintf(argt[4],"%d",key_posTaxi);
    sprintf(argt[5],"%d",SO_TIMEOUT);
    sprintf(argt[6],"%d",SO_TAXI);
    sprintf(argt[7],"%d",semStat);
    for(i=0;i<SO_HEIGHT;i++)
            sprintf(argt[8+i],"%d",keys[i]);

    /*fine prep args per taxi*/

    for(i=0;i<SO_TAXI;i++){
        if(pidTaxi[i]=fork()){
            continue;
        }
        else{
            sprintf(argt[8+SO_HEIGHT],"%d",i);
            execvp("./taxi",argt);
            PRINT_ERROR
            dprintf(2,"(execvp TAXI did not work)\n");
            exit(-1);
        }}
#ifdef DEV
    PRINT_ERROR
#endif

/*free degli args dei taxi, visto che non servono più*/
    for(i=0;i<8+SO_HEIGHT;i++)
        free(argt[i]);
    free(argt);


/*inizio preparazione args per sources*/
    argq = init_arg(5+SO_HEIGHT+1,100);
    sprintf(argq[0],"%d",SO_WIDTH);
    sprintf(argq[1],"%d",SO_HEIGHT);
    for(i=0;i<SO_HEIGHT;i++)
            sprintf(argq[4+i],"%d",keys[i]);

/*fine argq per SOURCE*/
/*crezione processi sources + pipeSources*/
 for(i=0;i<SO_SOURCES;i++)
    {
        sprintf(argq[2],"%d",pipeSO[1]);
        sprintf(argq[3],"%d",posSO[i]);
        sprintf(argq[4+SO_HEIGHT],"%d",i);
        if(pidSo[i]=fork())
            {    /*padre */
            continue;
            }
        else{   /*figlo */
            close(pipeSO[0]);
            execvp("./sources",argq);
            PRINT_ERROR
            dprintf(2,"(execvp SOURCES did not work)\n ");
            exit(-1);
        }
    }
    close(pipeSO[1]);

    /*KDaemon*/
    argk = init_arg(1+SO_SOURCES+1,100);
    sprintf(argk[0],"%d",SO_SOURCES);

    for(i=0;i<SO_SOURCES;i++)
        sprintf(argk[i+1],"%d",pidSo[i]);

    if(pidKDaemon = fork()){

    } else {
    execvp("./KDaemon",argk);
    PRINT_ERROR
    dprintf(2,"(execvp KDaemon did not work)\n ");
    exit(-1);
    }


    /*free degli args delle sources, visto che non servono più*/
    for(i=0;i<5+SO_HEIGHT;i++)
        free(argq[i]);
    free(argq);

    alarm(1);

/*main loop*/
/*qui dentro andranno gestite le sources, ovviamente, al posto di rand_dest*/
    while(1)
    {

        read(pipeSO[0], &in,2* sizeof(int));

        /* tenere per debug*/
        /*  destinazione = atoi (line) ;   /*destinazione finale da mettre nella pipe taxi*/
        ind_sep( &i_SO,  &j_SO, posSO[in[1]],  SO_WIDTH); /*indici mappa So richiedente*/

        do  /*cicla finchè non trova un taxi libero   k=taxi da associare alla SO*/
        {
            min_distanza=SO_WIDTH+SO_HEIGHT+1;
            k=-1;
            count=0;
            for(j=0;j<SO_TAXI;j++)
            {
                if ( posTaxi[j].destinazione==-1 && posTaxi[j].posizione!=-1)
                {
                    ind_sep( &i_TAX,  &j_TAX, posTaxi[j].posizione,  SO_WIDTH);
                    distanza= abs( i_SO - i_TAX ) + abs(j_SO - j_TAX) ;
                    if (min_distanza > distanza)
                    {
                        min_distanza = distanza;
                        k=j;
                        count++;
                        if (count>10) j=SO_TAXI;
                    }
                }
            }
        }while(k==-1);
        /*la source viene impostata con la posizione della pipe*/
        /*la destinazione viene impostata con la destinazione effettiva*/
        /*viene mandato uno dei due segnali riservati agli utenti al taxi per dirgli di muoversi*/
        posTaxi[k].destinazione=in[0];
        posTaxi[k].source=posSO[in[1]];
        kill(pidTaxi[k],SIGUSR1);


    }


	exit(0);
}

void handle_interrupt(int signal)
{
    int i,in[2],count;
    /*usa gli array di pid per mandare sigint a tutti i processi figli*/
    /*prima però lascio che i taxi impegnati in un operazione finiscano, così non rischio dati inconsistenti*/
    /*intanto nessun altro taxi può eseguire più operazioni*/
   if(sec==SO_DURATION){
        sem.sem_num=SO_WIDTH*SO_HEIGHT;	/*aggiungo 1 al semaforo sul quali i taxi si bloccano se esso non è 0*/
        sem.sem_op=1;

        semop(semStat, &sem,1);		/*aggiungo 1 al semaforo sul quali i taxi si bloccano se esso non è 0*/

        sem.sem_num=SO_WIDTH*SO_HEIGHT+1;	/*aspetto che il numero dei taxi impegnati in un operazione raggiunga lo 0*/
        sem.sem_op=0;
        while(semop(semStat, &sem,1)==-1) 	/*aspetto che i taxi impegnati in un'operazione finiscano*/
            {

                printf("The number of taxi ops currently is:%d\n",semctl(semStat, SO_WIDTH*SO_HEIGHT+1, GETVAL));

            }/*aspetto che i taxi impegnati in un operazione finiscano(l'op e' "aspetta 0")*/
        /*non ha senso che permetta ad altri taxi di eseguire operazioni, visto che sto per terminarli*/
   }
    for(i=0;i<SO_TAXI;i++)
        kill(pidTaxi[i],SIGINT);
    for(i=0;i<SO_SOURCES;i++)
        kill(pidSo[i],SIGINT);
    kill(pidKDaemon,SIGINT);
    while(wait(&i)!=-1);
    i=4;
    count=0;
    while(i>0){
        i=read(pipeSO[0], &in,2* sizeof(int));
        /*printf("inevese%d, destiazione%d, Sorgente:%d\n",i,in[0],in[1]);*/
        count++;
    }

    print_mappa_finale(mappa,SO_HEIGHT,SO_WIDTH, SO_TAXI, posTaxi,SO_TOP_CELLS);

    semctl(sem_id, 0 /* ignored */, IPC_RMID);
    semctl(semStat, 0 /* ignored */, IPC_RMID);
    dprintf(1,"\nCorse portate a buon fine:%d\nCorse fallite:%d\nCorse inevase:%d\n",stat_cond[0],stat_cond[1],count);
    exit(0);

}


void handle_alarm(int signal){
    sec++;
    if(sec==SO_DURATION) /*fine programma*/
        raise(SIGINT);

    /*coordinazione master-taxi*/
    sem.sem_num=SO_WIDTH*SO_HEIGHT;	/*aggiungo 1 al semaforo sul quali i taxi si bloccano se esso non è 0*/
    sem.sem_op=1;

    semop(semStat, &sem,1);		/*aggiungo 1 al semaforo sul quali i taxi si bloccano se esso non è 0*/

    sem.sem_num=SO_WIDTH*SO_HEIGHT+1;	/*aspetto che il numero dei taxi impegnati in un operazione raggiunga lo 0*/
    sem.sem_op=0;
    while(semop(semStat, &sem,1)==-1) 	/*aspetto che i taxi impegnati in un'operazione finiscano*/
        {

            printf("The number of taxi ops currently is:%d\n",semctl(semStat, SO_WIDTH*SO_HEIGHT+1, GETVAL));

        }/*aspetto che i taxi impegnati in un operazione finiscano(l'op e' "aspetta 0")*/
    printf("sec:%d\n",sec);
    /*printmappa*/
    print_mappa(mappa,SO_HEIGHT,SO_WIDTH);

    sem.sem_num=SO_WIDTH*SO_HEIGHT;	/*tolgo -1 al semaforo che permette ai taxi di fare operazioni, riportandolo a 0*/
    sem.sem_op=-1;

    semop(semStat, &sem,1);
    alarm(1);

}
