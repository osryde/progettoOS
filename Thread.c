#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

//Semafori e progressivo del thread per determinare le posizioni su cui effettuare il calcolo
sem_t mutex;
sem_t progr;
int progressivo;
int execution = 1;

//Struct da passare alla funzione eseguita dai thread
struct args{
	int* sum;
    int num_array;
	int thread;
	int array;
	int** p_array;
};

void *sumF(void* numero){
	
	//Valori per il calcolo
	int quantita = ((struct args*)numero)->array/ ((struct args*)numero)->thread; 
	int resto = ((struct args*)numero)->array%((struct args*)numero)->thread; 
    int num_array = ((struct args*)numero)->num_array;
	int somma[num_array];

    //busy waiting per farli partire insieme nel calcolo. Necessario solo per un calcolo corretto del tempo di esecuzione dei thread
    while(execution == 1){
        continue;
    }

	sem_wait(&progr);
	progressivo++;
	int turno = progressivo; 
	sem_post(&progr);
	
    for(int j = 0; j < num_array; j++){
        somma[j] = 0;
        for(int i = quantita*turno; i < (turno+1)*quantita; i++)
            somma[j] += ((struct args*)numero)->p_array[j][i];
    }

	//Attribuisco al primo thread numeri in più da sommare dati dal resto (se presente)
	if(turno == 0 && resto != 0 ){
        for(int j = 0; j < num_array; j++)
            for(int i = quantita*((struct args*)numero)->thread; i < quantita*((struct args*)numero)->thread+resto; i++)
                somma[j] += ((struct args*)numero)->p_array[j][i];
	}
		
	sem_wait(&mutex);
    for(int i = 0; i < num_array; i++)
	    (((struct args*)numero)->sum)[i] += somma[i];
	sem_post(&mutex);
}

int main(){
	
	//Inizializzo le variabili
	int dim_array = 0;
	int num_thread = 0;
    int num_array = 0;
	sem_init(&mutex, 0, 1);
	sem_init(&progr, 0, 1);
	progressivo = -1; 
    struct args *struct_thread;
	
	//Controllo dell'input utente
	do{
		
        printf("Inserisci il numero di array --> ");
        scanf("%d", &num_array);
		printf("Inserisci la dimensione degli array --> ");
		scanf("%d", &dim_array);
		printf("Inserisci il numero di thread (tra 5 e 10) --> ");
		scanf("%d", &num_thread);
		
	}while(dim_array < 1 || num_thread < 5 || num_thread > 10 || dim_array < num_thread || num_array < 1);

    //Genero e controllo la memoria dinamica con casting
	struct_thread = (struct args*) malloc(sizeof(struct args));
	struct_thread->sum = (int*) malloc(num_array * sizeof(int)); //array di somme
	
    if(struct_thread == NULL || struct_thread->sum == NULL){
        fprintf(stderr, "Errore, memoria dinamica non alloata");
        exit(0);
    }

    for(int i = 0; i < num_array; i++)
        (struct_thread->sum)[i] = 0; 
	
	//Definisco array, thread e valori dello struct
	struct_thread->thread =  num_thread;
    struct_thread->num_array = num_array;
	struct_thread->array = dim_array;
	int array[num_array][dim_array];//Array bidimensionale
	pthread_t thread[num_thread];
	
    int* array_pointer[num_array];
    for(int i = 0; i < num_array; i++)
        array_pointer[i] = array[i];

    struct_thread->p_array = array_pointer;
	
	//Genero i numeri all'interno dell'array
    for(int j = 0; j < num_array; j++){
		for(int i = 0; i < dim_array; i++)
		    array[j][i] = rand()%100;
	}
	
    struct timeval t1, t0; //Struct per il calcolo del tempo fornito dalla libreria

	//Creo e attendo la fine dell'esecuzione dei thread
	for(int i = 0; i < num_thread; i++){
		if(pthread_create(&thread[i], NULL, sumF, (void*)struct_thread) < 0){
			fprintf(stderr, "Errore nella creazione dei thread");
			exit(0);
		}
	}
	
    gettimeofday(&t0, 0);//Calcolo il tempo
	execution = 0; //Faccio partire tutti i thread che sono in busy waiting

	for(int i = 0; i < num_thread; i++)
		pthread_join(thread[i], NULL);
    
    gettimeofday(&t1, 0);

    long elapsed_thread = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;//Tempo per il calcolo dei thread
	
	//Somma con iterazione
	int somma_iterazione[num_array];

    gettimeofday(&t0, 0);//Calcola il tempo

    for(int j = 0; j < num_array; j++){
        somma_iterazione[j] = 0;
        for(int i = 0; i < dim_array; i++)
		    somma_iterazione[j] += array[j][i];
    }

    gettimeofday(&t1, 0);

    long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;//Tempo per il calcolo iterativo

	printf("\nSOMME\n");
	//Confronto il risultato dei thread con quello ricavato dall'iterazione del ciclo for
    for(int i = 0; i < num_array; i++){
        if(somma_iterazione[i] == (struct_thread->sum)[i])
            printf("| La somma è corretta | %d | Array %d \n",somma_iterazione[i], i+1);
        else
            fprintf(stderr, "Errore. Risultato non corretto");
    }
    
    printf("\nTEMPI\n| Tempo impiegato dalla somma iterativa    | %ld us\n", elapsed);
    printf("| Tempo impiegato dalla somma con i thread | %ld us\n\n", elapsed_thread);
	//Ripulisco la memoria allocata dinamicamente ed elimino semafori.
	sem_destroy(&mutex);
	sem_destroy(&progr);
	free(struct_thread->sum);
	free(struct_thread);
	
	return 0;
}
