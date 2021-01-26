#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wait.h>
#include "selvaggi.h"


/* gcc selvaggi.c -Wall -Wextra -pedantic -lrt -pthread -o selvaggi */

/* Variabile che indica il numero di selvaggi che l'utente ha passato
da linea di comando. Verrà usata per craare i singoli processi per i
selvaggi. Dichiarata qua, in quanto verrà usata solo nel processo padre. */
size_t selvaggiCount = 0;

/* Dichiarazione dei puntatori che punteranno all'area di memoria condivisa
dove ci saranno i semafori. Saranno poi visibili a tutti i processi dopo
la fork. */
sem_t *sem_pieno;           // Semaforo posix pentola piena
sem_t *sem_vuoto;           // Semaforo posix pentola vuota
sem_t *mutex;               // Mutex per mutua esclusione var. condivise

/* Dichiarazione file descriptor per le aree condivise */
int shm_fd_sem_pieno;
int shm_fd_sem_vuoto;
int shm_fd_mutex;
int shm_fd_info;

/* Dichiarazione puntatore struttura variabili condivise */
shared *sharedInfo;



/**
 * @brief 
 * Per iniziare l'esecuzione del programma, servono alcune informazioni che
 * devono essere inserite da linea di comando.
 *   argv[0] -> <nome_programma>
 *   argv[1] -> <numero_selvaggi_da_creare>
 *   argv[2] -> <numero_massimo_porzioni_per_pentola>
 *   argv[3] -> <numero_mangiate_per_selvaggio>
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char const *argv[]){
    
    /* Controllo numero parametri */
    if(argc < 4) {
        fprintf(stderr, "[!] Numero parametri errato.\n");
        fprintf(stdout, "[!] Esempio: ./selvaggi <numero_selvaggi> <numero_massimo_porzioni> <numero_mangiate>\n");
        return EXIT_FAILURE;
    }

    /* Controllo correttezza parametri */
    if( (atoi(argv[1]) < 1) || (atoi(argv[2]) < 1) || (atoi(argv[3]) < 1)) {
        fprintf(stderr, "[!] Non sono ammessi valori negativi o uguali a 0 come parametri.\n");
        fprintf(stdout, "[!] Hai inserito: /selvaggi %d %d %d\n", atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
        return EXIT_FAILURE;
    }

    /* Creazione area di memoria condivisa: sem_vuoto */
    shm_fd_sem_vuoto = shm_open("/myshmsemvuoto", O_CREAT|O_RDWR, 0600);
    if (shm_fd_sem_vuoto == -1) perror("Creazione memoria condivisa semaforo vuoto");
    ftruncate(shm_fd_sem_vuoto, sizeof(sem_t));
    sem_vuoto = mmap(0, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_sem_vuoto, 0);
    if (sem_vuoto == MAP_FAILED) perror("Creazione memoria condivisa semaforo vuoto");

    /* Creazione area di memoria condivisa: sem_pieno */
    shm_fd_sem_pieno = shm_open("/myshmsempieno", O_CREAT|O_RDWR, 0600);
    if (shm_fd_sem_pieno == -1) perror("Creazione memoria condivisa semaforo pieno");
    ftruncate(shm_fd_sem_pieno, sizeof(sem_t));
    sem_pieno = mmap(0, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_sem_pieno, 0);
    if (sem_pieno == MAP_FAILED) perror("Creazione memoria condivisa semaforo pieno");

    /* Creazione area di memoria condivisa: mutex */
    shm_fd_mutex = shm_open("/myshmmutex", O_CREAT|O_RDWR, 0600);
    if (shm_fd_mutex == -1) perror("Creazione memoria condivisa mutex");
    ftruncate(shm_fd_mutex, sizeof(sem_t));
    mutex = mmap(0, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_mutex, 0);
    if (mutex == MAP_FAILED) perror("Creazione memoria condivisa mutex");

    /* Creazione area di memoria condivisa: mutex */
    shm_fd_info = shm_open("/myshminfo", O_CREAT|O_RDWR, 0600);
    if (shm_fd_info == -1) perror("Creazione memoria condivisa variabili condivise");
    ftruncate(shm_fd_info, sizeof(shared));
    sharedInfo = mmap(0, sizeof(shared), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_info, 0);
    if (sharedInfo == MAP_FAILED) perror("Creazione memoria condivisa variabili condivise");

    /* Stampa solo per scopi di debug */
    if(DBG) printf("Indirizzo condiviso per sem_vuoto: %p\n", (void*)sem_vuoto);
    if(DBG) printf("Indirizzo condiviso per sem_pieno: %p\n", (void*)sem_pieno);
    if(DBG) printf("Indirizzo condiviso per mutex: %p\n", (void*)mutex);
    if(DBG) printf("Indirizzo condiviso per variabili: %p\n", (void*)sharedInfo);

    /* Inizializzazioni numero porzioni, numero selvaggi e salvataggi dei vari
    parametri nelle variabili condivise. Essendo inizialmente la pentola piena,
    inizializzo il numero di porzioni in pentola attuali con il numero 
    massimo di porzioni. */
    selvaggiCount = (size_t) atoi(argv[1]);
    sharedInfo->max_porzioni = atoi(argv[2]);
    sharedInfo->porzioni = sharedInfo->max_porzioni;
    sharedInfo->giri_mangiate = atoi(argv[3]);
    sharedInfo->riempimenti = 0;
    
    /* Stampa solo per scopi di debug */
    if(DBG) printf("Numero di selvaggi settato a %ld.\n", selvaggiCount);
    if(DBG) printf("Numero massimo di porzioni in pentola settato a %d.\n", sharedInfo->max_porzioni);
    if(DBG) printf("Numero di porzioni in pentola attuali settato a %d.\n", sharedInfo->porzioni);
    if(DBG) printf("Numero di mangiate che i selvaggi dovranno fare e' %d.\n", sharedInfo->giri_mangiate);

    /* Inizializzazione semafori condivisi */
    sem_init(sem_pieno, 1, 0);
    sem_init(sem_vuoto, 1, 0);
    sem_init(mutex, 1, 1);

    /* Creazione processo cuoco */
    pid_t pidcuoco = fork();
    if(pidcuoco == (pid_t) -1) {
        perror("Fork cuoco fallita");
        return EXIT_FAILURE;
    } else if (pidcuoco == (pid_t) 0) {
        cuoco();
    }else {
        if(DBG) printf("Creato processo cuoco con pid %d.\n", pidcuoco);
    }

    /* Creazione di tutti i selvaggi */
    for (size_t i = 0; i < selvaggiCount; i++){
        pid_t pidselv = fork();
        if(pidselv == (pid_t) -1) {
            perror("Fork selvaggio fallita");
            return EXIT_FAILURE;
        } else if (pidselv == (pid_t) 0) {
            selvaggio( ((int)i) + 1);
            exit(0);
        } else {
            if(DBG) printf("Creato processo selvaggio n°%d con pid %d.\n", (int)i+1, pidselv);
        }
    }

    /* Io che sono il padre, devo aspettare che tutti i selvaggi termino */
    for (size_t i = 0; i < selvaggiCount; i++) {
        int ret = 0;
        pid_t rr = wait(&ret);
        if(DBG) printf("Ho aspettato pid %d con ret = %d\n", (int)rr, ret);
    } 

    /* Ora che i selvaggi sono terminati, il cuoco non mi serve più e lo uccido */
    kill(pidcuoco, SIGTERM);

    /* Stampo alcune informazioni */
    printf("\n");
    printf("La pentola è stata riempita %d volte dal cuoco.\n", sharedInfo->riempimenti);
    printf("I selvaggi e il cuoco sono terminati. Sono rimeste %d porzioni di stufato nella pentola.\n", sharedInfo->porzioni);
    
    /* Deallocazione memoria condivisa */
    cleanSharedMem();

    return 0;
}




void cuoco() {
    /* Ciclo infinito. Il processo cuoco verrà terminato con un segnale dal padre 
    oppure risvegliato dai selvaggi quando non ci saranno porzioni in pentola. */
    while(1) {
        
        /* Faccio la down sul sem_vuoto. Appena creato il processo si sospende 
        qua perchè la pentola è piena e non ce bisogno del cuoco. Alla
        successiva iterazione di while verrà sospeso di nuovo. */
        sem_wait(sem_vuoto);
        printf("[CUOCO]: Mi sono svegliato.\n");

        /* Quando questo processo viene svegliato perchè si è fatta una up
        sul sem_vuoto, controllo se ci sono porzioni in pentola. Se è
        vuota, questo processo cuoco cucina le porzioni e le inserisce.
        Se non è vuoto, verrà sospeso di nuovo */
        if(sharedInfo->porzioni == 0) {
            printf("[CUOCO]: Sto cucinando perchè la pentola è vuota.\n");

            /* Cucina le porzioni */
            for (int i = 0; i < sharedInfo->max_porzioni; i++){
                sharedInfo->porzioni++;
            }

            /* Incrementa il contatore dei rimepimenti */
            sharedInfo->riempimenti++;
            printf("[CUOCO]: Ho appena riempito la pentola con %d porzioni.\n", sharedInfo->max_porzioni);

            /* Ora che ho riempito la pentola, i selvaggi possono continuare
            a mangiare. Risveglio il selvaggio che ha trovato pentola vuota */
            sem_post(sem_pieno);
            
        }

        /* A questo punto devo sospendermi di nuovo */
        printf("[CUOCO]: Ora vado a dormire.\n");

    }
}




void selvaggio(int n) {

    /* Ciclo delle mangiate di questo selvaggio. */
    for (size_t i = 0; i < (size_t) sharedInfo->giri_mangiate; i++){
        
        /* Accedo il mutua esclusione alle porzioni */
        if(DBG) printf("--------------prima di mutex down selvaggio %d\n", n);
        sem_wait(mutex);
        if(DBG) printf("--------------dopo di mutex down selvaggio %d\n", n);

        /* Stampa l'intezione di mangiare */
        printf("[SELVAGGIO %d]: Sto per prendere una porzione di stufato.\n", n);

        /* Per prima cosa, prima di mangiare, devo controllare se ci sono
        porzioni nella pentola. Se non ci sono sveglio il cuoco che me le
        cucina. */
        if(sharedInfo->porzioni == 0) {

            /* MEssaggio pentola vuota */
            printf("[SELVAGGIO %d]: La pentola è vuota e non posso mangiare. Ora sveglio subito il cuoco!!\n", n);

            /* Sveglio il cuoco sul semaforo vuoto (stava aspettando su questo 
            semaforo ). */
            sem_post(sem_vuoto);

            /* Ora che ho svegliato il cuoco devo aspettare che riempa la pentola. */
            sem_wait(sem_pieno);

        }

        /* A questo punto vi è disponibile almeno una porzione di stufato e posso
        mangiare */
        sharedInfo->porzioni--;
        printf("[SELVAGGIO %d]: Ho appena mangiato una porzione di stufato. Ne restano altre %d.\n", n, sharedInfo->porzioni);
        printf("[SELVAGGIO %d]: Questa era la %d° volta che mangiavo.\n",n, (int)i + 1);

        /* Accedo il mutua esclusione alle porzioni */
        if(DBG) printf("--------------prima di mutex up selvaggio %d \n", n);
        sem_post(mutex);
        if(DBG) printf("--------------dopo di mutex up selvaggio %d\n", n);

        /* Prima di mangiare di nuovo, devo digerire */
        sleep(1);
    }

    /* Quando ho finito di mangiare tutte le mangiate, termino */
    printf("[SELVAGGIO %d]: Non voglio piu' mangiare perchè sono sazio. \n", n);
    
}




void cleanSharedMem() {
    munmap(sem_vuoto, sizeof(sem_t));
    close(shm_fd_sem_vuoto);
    munmap(sem_pieno, sizeof(sem_t));
    close(shm_fd_sem_pieno);
    munmap(mutex, sizeof(sem_t));
    close(shm_fd_mutex);
    munmap(sharedInfo, sizeof(shared));
    close(shm_fd_info);
}

