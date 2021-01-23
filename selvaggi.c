#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wait.h>
#include "selvaggi.h"

/* Variabile che indica il numero di selvaggi che l'utente ha passato
da linea di comando. Verrà usata per craare i singoli processi per i
selvaggi. Non viene usata  */
size_t selvaggiCount = 0;

/* Dichiarazione dei puntatori che punteranno all'area di memoria condivisa
dove ci saranno i semafori. Saranno poi visibili a tutti i processi dopo
la fork. */
sem_t *sem_pieno;           // Semaforo posix pentola piena
sem_t *sem_vuoto;           // Semaforo posix pentola vuota
pthread_mutex_t *mutex;     // Mutex per mutua esclusione var. condivise

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
 *   argv[0] -> <nome_programma>s
 *   argv[1] -> <numero_selvaggi_da_creare>
 *   argv[2] -> <numero_massimo_porzioni_per_pentola>
 *   argv[3] -> <numero_mangiate_per_selvaggio>
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char const *argv[]){
    
    /* Controllo numero parametri */
    if(argc < 4) {
        fprintf(stderr, "[!] Numero parametri errato.\n");
        fprintf(stdout, "[!] Esempio: ./selvaggi <numero_selvaggi> <numero_massimo_porzioni> <numero_mangiate>");
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
    ftruncate(shm_fd_mutex, sizeof(pthread_mutex_t));
    mutex = mmap(0, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_mutex, 0);
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











    /* Deallocazione memoria condivisa */
    cleanSharedMem();

    return 0;
}




void cleanSharedMem() {
    munmap(sem_vuoto, sizeof(sem_t));
    close(shm_fd_sem_vuoto);
    munmap(sem_pieno, sizeof(sem_t));
    close(shm_fd_sem_pieno);
    munmap(mutex, sizeof(pthread_mutex_t));
    close(shm_fd_mutex);
    munmap(sharedInfo, sizeof(shared));
    close(shm_fd_info);
}

