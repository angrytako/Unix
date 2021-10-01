#include "pacchetto.h"
#define PAC
cella** linking(int* keys, int size_a){
    cella **sup;
    int i;
    /*sup=shmat(key, NULL ,0);*/
    sup=malloc(size_a*sizeof(cella*));
    for(i=0;i<size_a;i++){
        sup[i]=shmat(keys[i] , NULL ,0);
#ifdef DEV
        PRINT_ERROR
#endif
        }
    return sup;


}





void mark(int* keys, int size_hight){
    int i;
    for(i=0;i<size_hight;i++){
        shmctl(keys[i], IPC_RMID, NULL);
#ifdef DEV
        PRINT_ERROR
#endif
}
    return;
}


void detach(cella **mappa ,int size_hight,int sem_id){
    int i,j;


    printf("cancellazione semaforo:%d\n",semctl(sem_id , 0, IPC_RMID));


    for(i=0;i<size_hight;i++){
        shmdt(mappa[i]);

    }
    shmdt(mappa);
    return;
}
void print_pos(Pos* pos, int id_taxi){
    dprintf(1,"Posizione attuale:%d\nDestinazione:%d\nSource:%d\nCelle percorse:%d\nTempo di attivita':%ld\n",pos[id_taxi].posizione,
    pos[id_taxi].destinazione, pos[id_taxi].source,pos[id_taxi].Max_celle,pos[id_taxi].Max_tempo);
    return;

}

int trasf_indice(int i,int j,int SO_WIDTH){
    return (SO_WIDTH*i)+j;
}
void ind_sep(int *y, int *x,int index, int SO_WIDTH){
    *y=index/SO_WIDTH;
    *x=index%SO_WIDTH;
    return;
}
int rand_pos(cella** mappa,Pos *pos, int nr_taxi, int size_w, int size_h){
    int i,j,sup_i,sup_j,index;
    static int sup=0;/*serve una variabile statica, perché sia time(NULL) che getpid ti danno lo stesso valore ogni volta come seed*/
                        /*se questa funzione viene chiamata più volte in un arco di tempo breve*/

/*sup verrà usato come seed per srand, per cui viene inizializzato con il pid e poi incrementa ogni chiamata di questa funzione*/
   if(sup==0)
       sup=getpid();
    else
        sup++;

    srand(sup);
    i=rand()%size_h;
    j=rand()%size_w;

        for(sup_i=i+1;sup_i%size_h !=i;sup_i++){
            for(sup_j=j+1;sup_j%size_w !=j;sup_j++){
                index=trasf_indice(sup_i%size_h, sup_j%size_w, size_w);
                /*se sup_i e sup_j, presi in modulo, non sono un buco e ci sono meno taxi della capienza massima, tale cella
                viene restituita*/
                if (mappa[sup_i%size_h][sup_j%size_w].occupata==0 && mappa[sup_i%size_h][sup_j%size_w].cur_num_taxi<mappa[sup_i%size_h][sup_j%size_w].Max_taxi)
                    return index;
                    }}
}
