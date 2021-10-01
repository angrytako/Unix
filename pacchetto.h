#define _GNU_SOURCE   /*mi raccomando inserisci sempre quando usi cose di unix*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#define PAC
#define PRINT_ERROR fprintf(stderr,					\
			    "%s:%d: Errore %d \"%s\"\n",		\
			    __FILE__, __LINE__, errno, strerror(errno));  /*macro per stampa di possibili errori*/
#ifndef MAS
typedef struct {
	long tempo;     /* tempo di attraversamento cella generato casualmente */
	int occupata;   /* cella non utilizzabile (buchi)*/
    int Max_taxi;       /* numero massimo di taxi generato casualmente*/  /*questo numero finisce nel semaforo*/
    int SOURCES;       /*numero di sorgenti sulla cella */
    int tot_taxi;   /*per la statistica sulla cella più attarversata*/
    int cur_num_taxi;  /*numero di taxi attualmente nella cella*/
}cella;
typedef struct {
	int posizione;
    int source;
	int destinazione;
    int Max_celle;
    int clienti_serviti;
    long Max_tempo;
}Pos;                      /* ho cambiato il nome della struct perchè mi dava troppo fastidio!!*/
#endif
cella** linking(int* keys, int size_a); /*gli dai la chiave di allocazione dell array di array, un array contente
                                                le chiavi di allocazione di ogni singolo array puntato dall'array di array, e la
                                                dimensione di tale array; la funzione ti fa il linking per ogni processo,
                                                così non impazzisci; restituisce il puntatore all'array di array di struct*/


void mark(int* keys, int size_hight);   /*funzione per eseguire per dire al SO che se viene
                                            terminato il processo, la dealoccazione debba avvenire in automatico,
                                            così in realtà non c'è neanche bisogno di un
                                            handler apposta, se la esegui in ogni processo dopo aver fatto il linking*/

void detach(cella **sup,int  size_hight,int sem_id);  /*detach memoria condivisa dal main + deallocazione semafori*/
void print_pos(Pos* pos, int id_taxi);                 /*stampa la struct per un determinato taxi*/
int trasf_indice(int i,int j,int SO_WIDTH); /* traforma l'inidce da [i][j] a [return] */
void ind_sep(int *y, int *x,int index, int SO_WIDTH); /*prende un indice linearizzato e lo separa in x e y*/

int rand_pos(cella** mappa,Pos *pos, int nr_taxi,  int size_w, int size_h);
/*funzione ottimizzata per la ricerca di una casella libera, quando si tenta di riposizionare il taxi*/
/*la funzione e' leggermente approssimativa, nel senso che senza un semaforo che blocchi ogni operazione sull'aray condiviso,
e' possibile che i dati vengano letti in maniera inconsistente, ma al massimo, se ciò è accaduto la funzione verrà richiamata, che
da test pratici è risultato molto più veloce che bloccare ogni operazione sulle stat condivise per ogni taxi che si riposiziona
(oltre al fatto che si dovrebbe permettere ad un unico taxi per volta di riposizionarsi)*/



