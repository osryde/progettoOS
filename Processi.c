#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> //Libreria di Linux
#include <sys/time.h>
#include <signal.h>

void handle_sigcont(int sig){}

int main(){
	
    //Inizializzo le variabili
	int num_processi = 0;
    int dim_array = 0;
    int num_array = 0;
    struct timeval t1, t0;

    //Controllo sugli input
    do{
        printf("Inserisci il numero di array --> ");
        scanf("%d", &num_array);
        printf("Inserisci la dimensione dell'array --> ");
        scanf("%d", &dim_array);
        printf("inserisci il numero dei processi (tra 5 e 10) --> ");
        scanf("%d", &num_processi);

    }while(num_processi < 5 || num_processi > 10 || dim_array < 1 || dim_array < num_processi || num_array < 1);

    //Inizializzo le pipe e controllo il valore di ritorno

    int pipes[num_processi][2]; //[0] READ, [1] WRITE

    for(int i = 0; i < num_processi; i++){
        if(pipe(pipes[i]) == -1){
            printf("Errore");
            return 4;
        }
    }

    //Creo e inizializzo i valori dell'array
    int array[num_array][dim_array];
    for(int j = 0; j < num_array; j++){
        for(int i = 0; i < dim_array; i++)
            array[j][i] = rand()%100;
    }

    fflush(stdin);
    pid_t pid[num_processi];
    int progressivo = 0; //Variabile che identifica il numero del processo

    //Creo i processi figlio
    for(int i = 0; i < num_processi; i++){
        pid[i] = fork();
        if(pid[i] == 0){
            progressivo = i; //Assegna un numero progressivo ai processi figlio
            break;
        }
        else if(pid[i] == -1)//Controllo il valore di ritorno della fork()
            return 1;
    }

    
    if(pid[progressivo] == 0){//Codice eseguito dai processi figli
       
        signal(SIGCONT, handle_sigcont);
        pause();//Metto in pausa i processi per il calcolo del tempo in modo da farli partire insieme

        int somma_parziale[num_array];
        int quantita = dim_array/num_processi;
        int resto = dim_array%num_processi;

        //Chiudo le pipe in lettura
        for(int i = 0; i < num_processi; i++)
            close(pipes[i][0]);

        //Chiudo le pipe non utilizzate in scrittura
        for(int i = 0; i < num_processi; i++)
            if(i != progressivo)
                close(pipes[i][1]);
        
        for(int j = 0; j < num_array; j++){
            somma_parziale[j] = 0;
            for(int i = progressivo*quantita; i < quantita*(progressivo+1); i++)
                somma_parziale[j] += array[j][i];
        }
        
        if(progressivo == 0){
            for(int j = 0; j < num_array; j++){
                for(int i = quantita*num_processi; i <  quantita*num_processi+resto; i++)
                    somma_parziale[j] += array[j][i];
            }
        }
        
        //Scrivo sulla pipe e ne controllo il valore di ritorno
        if(write(pipes[progressivo][1], somma_parziale, sizeof(int)*num_array) == -1){//Mando l'array sulla pipe
                printf("ERRORE");
                return 2;
            }

        close(pipes[progressivo][1]);

    }else{//Codice eseguito dal processo main

        //riattivo i processi per il cacolo del tempo inviando un segnale
        gettimeofday(&t0, 0);
        for(int i = 0; i < num_processi; i++){
            if(kill(pid[i] ,SIGCONT) == -1)
                fprintf(stderr, "Errore");
        }
        
        //Attendo i processi
        for(int i = 0; i < num_processi; i++)
            wait(NULL);

        int somma_processi[num_array];
        int x[num_array];
        
        //Chiudo le pipe in scrittura
        for(int i = 0; i < num_processi; i++)
            close(pipes[i][1]);
        
        //Inizializzo l'array
        for(int i = 0; i < num_array; i++)
            somma_processi[i] = 0;
        
        //Leggo il valore in ogni pipe
        for(int i = 0; i < num_processi; i++){
            if(read(pipes[i][0], x, sizeof(int)*num_array) == -1){//ricevo l'array della pipe
                printf("ERRORE");
                return 3;
            }
            for(int j = 0; j < num_array; j++)
                somma_processi[j] += x[j];
            close(pipes[i][0]);
        }

        gettimeofday(&t1, 0);
        long elapsed_processes = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;

        //Somma iterativa con calcolo del tempo impiegato in ms
        gettimeofday(&t0, 0);

        int somma_iterativa[num_array];
        for(int j = 0; j < num_array; j++){
            somma_iterativa[j] = 0;
            for(int i = 0; i < dim_array; i++)
                somma_iterativa[j] += array[j][i];    
        }

        gettimeofday(&t1, 0);
        long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;

        printf("\nSOMME\n");
        for(int i = 0; i < num_array; i++){
            if(somma_iterativa[i] == somma_processi[i])
                printf("| La somma è corretta | %d | Array %d\n", somma_processi[i], i+1);
            else
                printf("Errore nella somma\n");
        }

        printf("\nTEMPI\n");
        printf("Tempo impiegato dalla somma iterativa      | %ld us\n", elapsed);
        printf("Tempo impiegato dalla somma con i processi | %ld us\n\n", elapsed_processes);
        
    }

    return 0;
}
