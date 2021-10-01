#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define MAS
#define PRINT_ERROR fprintf(stderr,					\
			    "%s:%d: Errore %d \"%s\"\n",		\
			    __FILE__, __LINE__, errno, strerror(errno));  /*macro per stampa di possibili errori*/
#ifndef PAC
typedef struct {
	long tempo;     /* tempo di attraversamento cella generato casualmente */
	int occupata;   /* cella non utilizzabile (buchi)*/
    int Max_taxi;       /* numero massimo di taxi generato casualmente*/  /*questo numero finisce nel semaforo*/
    int SOURCES;       /*numero di sorgenti sulla cella */
    int tot_taxi;   /*per la statistica sulla cella più attarversata*/
    int cur_num_taxi;
}cella;

typedef struct {
	int posizione;
    int source;
	int destinazione;
    int Max_celle;
    int clienti_serviti;
    long Max_tempo;
}Pos;
#endif



int valore (FILE * st); /*questa funzione legge da uno stream che gli viene passato
                                una "coppia" nomeVariabile / valore e restituisce il valore.
                                Le coppie vanno separate tra di loro con un invio.
                                Il motivo per cui l'ho fatto così è per poter leggere con facilità
                                anche il file "salvataggio", così non impazzisci quando vuoi cambiare un valore*/

void deserializzatore(FILE *st,int* SO_TAXI, int* SO_SOURCES, int* SO_HOLES, int* SO_TOP_CELLS, int* SO_CAP_MIN,
                    int* SO_CAP_MAX, int* SO_TIMENSEC_MIN,int* SO_TIMENSEC_MAX, int* SO_TIMEOUT, int* SO_DURATION);
                    /*deserializzatore vero e proprio. Basta che passi le variabili che hai inizializzato in maniera
                    "hard coded" per indirizzo, con anche un puntatore allo stream aperto del file,
                    e poi le puoi usare esattamente come hai fatto fino ad ora.
                    Sarebbe stato più facile usare un array, ma avrebbe danneggiato la leggibilità troppo
                    P.S. Questa funzione chiude anche lo stream. Se la cosa non ti aggrada, puoi cambiarla!
                    P.P.S Controlla dopo che non ci siano -1 tra i valori, altrimenti vuol dire che c'è stato un errore*/
void print_mappa(cella **map, int SO_HEIGHT , int SO_WIDTH); /*In quetso momento fa un semplice debug dello mappa con buchi e sorgenti*/

void print_mappa_finale(cella **map, int SO_HEIGHT , int SO_WIDTH, int SO_TAXI, Pos *posTaxi,int SO_TOP_CELLS);

int check(cella **map,int flag_i,int flag_j, int SO_HEIGHT,int SO_WIDTH); /*controlla se c'è una casella bloccata attorno alla casella [flag_i][flag_j]*/

int creazione_mappa( int SO_HEIGHT,int SO_WIDTH, cella **mappa,int SO_TIMENSEC_MIN,int SO_TIMENSEC_MAX, int SO_CAP_MIN,
                    int SO_CAP_MAX,int SO_HOLES, int SO_SOURCES,int sem_id, int*PosSO, int semStat, int SO_TAXI);
                    /*assegno i valori iniziali alle celle*/

char** init_arg(int nr_arg, int max_length);       /*serve per fare le malloc per gli argomenti*/
                                            /*mette in automatico l ultima stringa a null*/













