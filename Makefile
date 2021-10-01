# dal carattere '#' fino a fine riga, il testo dentro il Makefile e`
# un commento
#
# flags per la compilazione
CFLAGS =  -std=c89 -Wpedantic
# target ovvero nome dell'eseguibile che si intende produrre
TARGET = master
TAXI= taxi
SO= sources
KD = KDaemon
# object files necessari per produrre l'eseguibile
OBJ    = pacchetto.o master.o master_lib.o
TAXI_OBJ = pacchetto.o taxi.o
SO_OBJ= pacchetto.o sources.o
KD_OBJ = KDaemon.o
# Si sfrutta la regola implicita per la produzione dei file oggetto in
# $(OBJ)
#
# Le "variabili" del Makefile sono espanse con $(NOME_VAR). Quindi
# scrivere
#
# $(TARGET): $(OBJ)
#
# equivale a scrivere
#
# application: matrix.o application.o
#
# ovvero a specificare che per produrre l'eseguibile "application"
# servono i due object files "matrix.o" e "application.o"

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)
$(TAXI): $(TAXI_OBJ)
	$(CC) $(TAXI_OBJ) $(LDFLAGS) -o $(TAXI)
$(SO): $(SO_OBJ)
	$(CC) $(SO_OBJ) $(LDFLAGS) -o $(SO)
$(KD): $(KD_OBJ)
	$(CC) $(KD_OBJ) $(LDFLAGS) -o $(KD)
# solitamente il target "all" e` presente
all: $(TARGET) $(TAXI) $(SO)  $(KD) 

# con "make clean" solitamente si vogliono eliminare i prodotti della
# compilazione e ripristinare il contenuto originale
clean:
	rm -f *.o $(TARGET) *~
	rm -f *.o $(TAXI) *~
	rm -f *.o $(SO) *~
	rm -f *.o $(KD) *~
	rm -f ./taxi_logs/*.txt

# il target run si usa talvolta per eseguire l'applicazione
run: $(TARGET)
	./$(TARGET)
