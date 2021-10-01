#include "master_lib.h"
int valore (FILE * st){
    int carattere, i=0;
    char* buf;
    buf=malloc(100);
    while((carattere=getc(st))!= '/'){
        if(carattere==EOF)
            return -1;
    }
    while((carattere=getc(st))!= '\n' && carattere!=EOF){
        buf[i]=(char) carattere;
        i++;
    }
    return atoi(buf);



}
void deserializzatore(FILE *st, int* SO_TAXI, int* SO_SOURCES, int* SO_HOLES, int* SO_TOP_CELLS, int* SO_CAP_MIN,
                    int* SO_CAP_MAX, int* SO_TIMENSEC_MIN,int* SO_TIMENSEC_MAX, int* SO_TIMEOUT, int* SO_DURATION){

    *SO_TAXI= valore(st);
    *SO_SOURCES= valore(st);
    *SO_HOLES= valore(st);
    *SO_TOP_CELLS= valore(st);
    *SO_CAP_MIN= valore(st);
    *SO_CAP_MAX= valore(st);
    *SO_TIMENSEC_MIN= valore(st);
    *SO_TIMENSEC_MAX= valore(st);
    *SO_TIMEOUT= valore(st);
    *SO_DURATION= valore(st);
    fclose(st);
    return;
    }


void print_mappa(cella **map, int SO_HEIGHT , int SO_WIDTH){
    int i,j,k,count,count_D;

    printf("|");
        for (i=0;i<SO_HEIGHT;i++){
            for (j=0;j<SO_WIDTH;j++){
                if (map[i][j].occupata==1){
                    printf("X|");
                }else if (map[i][j].cur_num_taxi==0){
                    printf("_|");
                }else {
                    printf("%d|",map[i][j].cur_num_taxi);
                }
            }
            printf("  linea %d \n|",i);
        }
}


void print_mappa_finale(cella **map, int SO_HEIGHT , int SO_WIDTH, int SO_TAXI, Pos *posTaxi,int SO_TOP_CELLS){
    int i,j,l,appoggio,max,count;
    long maxx;
    char ops;
    int *best;
    best= malloc(sizeof(int)*SO_TOP_CELLS);

    printf("stampafinale\n");

    count=0;
    appoggio =SO_TOP_CELLS;
    while(appoggio>=0){
        /*cerco il massimo non ancora inserito nel arry*/
        max=-1;
        for (i=0;i<SO_HEIGHT;i++){
            for (j=0;j<SO_WIDTH;j++){

                if (map[i][j].tot_taxi>=max) {
                    /*se non c'è nell'array di besy;*/
                    ops=1;
                    for (l=0;l<=count;l++){
                        if (best[l]== trasf_indice(i,j,SO_WIDTH) ) {
                            ops=0;
                        }
                    }
                    if (ops==1){
                        max=map[i][j].tot_taxi;
                    }
                }

            }
        }
        /*massimo trovato*/
        /*lo inserico in modo ordinato*/
        for (i=0;i<SO_HEIGHT;i++){
            for (j=0;j<SO_WIDTH;j++){
                if (map[i][j].tot_taxi==max){
                    /*se non c'è nell'array*/
                    ops=1;
                    for (l=0;l<=count;l++){
                        if (best[l]== trasf_indice(i,j,SO_WIDTH) ) {
                            ops=0;
                        }
                    }
                    if (ops==1){ /*non è già stata inserita questa cella*/
                        if (count<SO_TOP_CELLS){
                            best[count]=trasf_indice(i,j,SO_WIDTH);
                        }
                        count++;
                        appoggio--;
                    }
                }
            }
        }

    }  /*fine preparazione posizioni con best attraversate*/


    printf("S= Sources -- += SO_TOP_CELL -- Æ= Entrambi\n");
    printf("|");
    for (i=0;i<SO_HEIGHT;i++){
            for (j=0;j<SO_WIDTH;j++){

            ops=1;
            for (l=0;l<SO_TOP_CELLS;l++)   {
                if (best[l]==trasf_indice(i,j,SO_WIDTH)) {
                    ops=0;
                    if (map[i][j].SOURCES==1) printf("Æ");
                    else  printf("+");

                }
            }
            if (ops==1){
                if (map[i][j].SOURCES==1) printf("S");
                else  printf("_");
            }




            printf("|");


            }
            printf("  linea %d \n|",i);
    }

    max=-1;
    for (i=0;i<SO_TAXI;i++){
        if (posTaxi[i].Max_celle>max)
        {
            max= posTaxi[i].Max_celle;
            j=i;
        }
    }
    printf("Il taxi che ha attraversato più caselle è il:%d con:%d celle attraversate\n",j,max);

     maxx=-1;
    for (i=0;i<SO_TAXI;i++){
        if (posTaxi[i].Max_tempo >maxx)
        {
            maxx= posTaxi[i].Max_tempo;
            j=i;
        }
    }
    printf("Il taxi che ha fatto il viaggio più lungo è il:%d con:%ld nano secondi\n",j,maxx);

     max=-1;
    for (i=0;i<SO_TAXI;i++){
        if (posTaxi[i].clienti_serviti >max)
        {
            max= posTaxi[i].clienti_serviti;
            j=i;
        }
    }
    printf("Il taxi che ha servito più clienti è il:%d con:%d clienti\n",j,max);

}





int check(cella **map,int flag_i,int flag_j,int SO_HEIGHT, int SO_WIDTH){  /*false 0 -->occupata | true 1-->libera */  /*questo commento serve a me xoxo*/
    int i,j,out=1;
  
    for (i=(flag_i-1);i<(flag_i+2);i++){
        for (j=(flag_j-1);j<(flag_j+2);j++){
            if ( i>=0 && i<SO_HEIGHT && j>=0 && j<SO_WIDTH ){
                if (map[i][j].occupata == 1) out =0;  /*se è occupata la cella [flag_i][flag_j] è inutilizzabile*/
            }
        }
    }
    
    return out;
}




int creazione_mappa(int SO_HEIGHT, int SO_WIDTH, cella **mappa,int SO_TIMENSEC_MIN,int SO_TIMENSEC_MAX, int SO_CAP_MIN,
                    int SO_CAP_MAX,int SO_HOLES, int SO_SOURCES,int sem_id,int*PosSO,int semStat, int SO_TAXI){
    int a,i,j,count, sup_i, sup_j;
    srand (getpid());
    /* genero casualmente: tempo attraversamento, numero taxi e inizializzo il resto a 0*/
    for (i=0;i<SO_HEIGHT;i++){
        for (j=0;j<SO_WIDTH;j++){
            mappa[i][j].tempo= (long) SO_TIMENSEC_MIN+rand()%(SO_TIMENSEC_MAX-SO_TIMENSEC_MIN +1); /*ti ho aggiunto +1, perché immagino che SO_TIMENSEC_MAX
                                                                                            e SO_CAP_MAX sono inclusi nel range*/
            mappa[i][j].Max_taxi= SO_CAP_MIN+rand()%(SO_CAP_MAX-SO_CAP_MIN +1);
            mappa[i][j].occupata=0;
            mappa[i][j].SOURCES=0;
            mappa[i][j].tot_taxi=0;
            mappa[i][j].cur_num_taxi=0;
            semctl(sem_id, trasf_indice(i,j,SO_WIDTH), 16, mappa[i][j].Max_taxi);   /* 16=SETVAL macro non andava...*/
            semctl(semStat, trasf_indice(i,j,SO_WIDTH), 16, 1);
        #ifdef DEV
            PRINT_ERROR
        #endif
           }
    }
    /*semafori per le statistiche condivise*/
    semctl(sem_id, SO_WIDTH*SO_HEIGHT, 16, 1);
    semctl(sem_id, SO_WIDTH*SO_HEIGHT+1, 16, 1);
    /*semaforo per riposizionamento taxi*/
    semctl(sem_id, SO_WIDTH*SO_HEIGHT+2, 16, SO_CAP_MAX*2);
    /*generazione dei buchi */
    /*semafori coordinazione master-taxi*/
    semctl(semStat, SO_WIDTH*SO_HEIGHT, 16, 0);
    semctl(semStat, SO_WIDTH*SO_HEIGHT+1, 16, 0);
    a=SO_HOLES; /* variabile di appoggio*/
    count=0; /*se non riesce ad trovare casualmente una cella compatibile si arrende */
    while (a>0){
        i=rand()%SO_HEIGHT;
        j=rand()%SO_WIDTH;
        count++;
        if (check(mappa,i,j,SO_HEIGHT, SO_WIDTH)) {
            mappa[i][j].occupata=1;
            count=0;
            a--;
        } else if (count > SO_HEIGHT*SO_WIDTH/2)
            {
                for(sup_i=i+1;(sup_i%SO_HEIGHT) != i;sup_i++)
                    {
                        if (sup_i==SO_HEIGHT) sup_i= 0;

                        if(count==0)  break;
                        for(sup_j=j+1;(sup_j%SO_WIDTH) !=j;sup_j++)
                            {
                                if (sup_j==SO_WIDTH) sup_j= 0;
                                if (check(mappa,sup_i,sup_j,SO_HEIGHT, SO_WIDTH))
                                    {
                                        mappa[sup_i][sup_j].occupata=1;
                                        a--;
                                        count=0;
                                        break;
                                    }
                            }
                    }
                if(count!=0){
                    /*impossibile trovare un buco disponibile*/
                    fprintf(stderr,"Impossibile creare %d buchi\n impossibile posizionare ultimi %d buchi\n",SO_HOLES,a);
                    return 0;
                }

            }
    }
    a= SO_SOURCES;
    count=0;
    while (a>0){
        i=rand()%SO_HEIGHT;
        j=rand()%SO_WIDTH;
    count++;
        if (mappa[i][j].occupata==0 && mappa[i][j].SOURCES==0) {
            mappa[i][j].SOURCES = 1;
            PosSO[SO_SOURCES-a]=(i*SO_WIDTH)+j;
            a--;
            count=0;
        } else if (count > SO_HEIGHT*SO_WIDTH/2){
                for(sup_i=i+1;sup_i%SO_HEIGHT !=i;sup_i++){
                    if (sup_i==SO_HEIGHT) sup_i= 0;
                    if(count==0)
                        break;
                    }for(sup_j=j+1;sup_j%SO_WIDTH !=j;sup_j++){
                            if (sup_j==SO_WIDTH) sup_j= 0;
                          if (mappa[sup_i][sup_j].occupata==0 && mappa[sup_i][sup_j].SOURCES==0) {
                                mappa[sup_i][sup_j].SOURCES = 1;
                                PosSO[SO_SOURCES-a]=i*SO_WIDTH+j;
                                a--;

                                count=0;
                                break;
        }}
                if(count!=0){
                    /*impossibile trovare un buco disponibile*/
                    fprintf(stderr,"Impossibile creare %d sorgenti\n impossibile posizionare le ultime %d sorgenti\n",SO_SOURCES,a);
                    return 0;
                }

        }
    }

    return 1;
}
char** init_arg(int nr_arg, int max_length){
    char **arg;
    int i;
    arg=malloc(sizeof(*arg)*nr_arg);
    for(i=0;i<nr_arg-1;i++)
        arg[i]=malloc(max_length+10);
    arg[i]=NULL;
    return arg;

}



