/* ■
* Costante usata per abilitare la stampa di tutto il codice usato per debuggare.
* 0 -> nascondi tutte le stampe;
* 1 -> mostra tutte le stampe. */
#define DBG 0

/* ■
* Definizione struttura per le variabili condivise. */
struct info {

    /* Numero di porzioni di stufato ancora in pentola,
    pronte per essere mangiate dai selvaggi */
    int porzioni;   

    /* Numero di volte che il cuoco è stato svegliato da un selvaggio
    per riempire di stufato la pentola perchè era vuota. */
    int riempimenti;

    /* Quantità massima di porzioni che ci stanno in una pentola.
    Sarà anche il numero di stufati che il cuoco farà quando verrà
    svegliato. */
    int max_porzioni;

    /* Numero di volte che un selvaggio deve mangiare prima di 
    terminare. */
    int giri_mangiate;
};
typedef struct info shared;


/* ■
* Definizione prototipi funzioni */

/**
 * @brief Funzione che stabilisce il comportamento del cuoco
 */
void cuoco();

/**
 * @brief Funzione che stabilisce il comportamento del singolo selvaggio.
 * Verranno poi trattati come processi diversi.
 * 
 * @param int Numero del selvaggio
 */
void selvaggio(int n);

/**
 * @brief Metodo per deallocare la memoria condivisa.
 * 
 */
void cleanSharedMem();
