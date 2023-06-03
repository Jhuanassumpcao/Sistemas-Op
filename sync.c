#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_REQUESTS 10 // Número máximo de solicitações de impressão por setor

// Definição de semáforos
sem_t t1_printer_sem;
sem_t t2_printer_sem;

// Variáveis globais para controle de impressões
int t1_print_success = 0;
int t2_print_success = 0;
int t1_print_failures = 0;
int t2_print_failures = 0;
int total_requests = 0;

// Função para simular o envio de uma solicitação de impressão
void send_print_request(int department_id, int request_id) {
    printf("Setor %d está realizando solicitação #%d de impressão\n", department_id, request_id);
    sleep(rand() % 3 + 1); // Simula tempo de envio da solicitação
}

// Função para simular a impressão de uma solicitação de impressão
void print_request(int department_id, int request_id, int printer_id, int type) {
    printf("Impressora %d está imprimindo a solicitação #%d do setor %d\n", printer_id, request_id, department_id);
    sleep(rand() % 5 + 1); // Simula tempo de impressão

    if (type == 1) {
        if (rand() % 100 < 15) {
            printf("Impressão #%d falhou\n", request_id);
            if (department_id == 1) {
                t1_print_failures++;
                sem_post(&t1_printer_sem);
            } else {
                t2_print_failures++;
                sem_post(&t2_printer_sem);
            }
            return;
        } else {
            t1_print_success++;
        }
    } else {
        t2_print_success++;
    }

    printf("Impressão #%d concluída com sucesso\n", request_id);
}

// Função para o thread de impressoras T1
void *t1_printer_thread(void *arg) {
    int printer_id = *((int *) arg);

    while (1) {
        sem_wait(&t1_printer_sem);

        int department_id = 1;
        int request_id = total_requests + 1;

        if (request_id > MAX_REQUESTS) {
            sem_post(&t1_printer_sem);
            break;
        }

        print_request(department_id, request_id, printer_id, 1);

        total_requests++;
        sem_post(&t1_printer_sem);
    }

    pthread_exit(NULL);
}

// Função para o thread de impressoras T2
void *t2_printer_thread(void *arg) {
    int printer_id = *((int *) arg);

    while (1) {
        sem_wait(&t2_printer_sem);

        int department_id = 2;
        int request_id = total_requests + 1;

        if (request_id > MAX_REQUESTS) {
            sem_post(&t2_printer_sem);
            break;
        }

        print_request(department_id, request_id, printer_id, 2);

        total_requests++;
        sem_post(&t2_printer_sem);
    }

    pthread_exit(NULL);
}

// Função para o thread de cada departamento
void *department_thread(void *arg) {
    int department_id = *((int *) arg);

    while (1) {
        send_print_request(department_id, total_requests + 1);

        if (department_id == 1) {
            sem_post(&t1_printer_sem);
        } else {
            sem_post(&t2_printer_sem);
        }

        usleep(500); // Tempo de espera antes de enviar a próxima solicitação
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <MAX>\n", argv[0]);
        return 1;
    }

    int MAX = atoi(argv[1]);

    // Inicialização dos semáforos
    sem_init(&t1_printer_sem, 0, 0);
    sem_init(&t2_printer_sem, 0, 0);

    // Criação dos threads das impressoras T1
    pthread_t t1_printers[MAX];
    for (int i = 0; i < MAX; i++) {
        int *printer_id = malloc(sizeof(int));
        *printer_id = i + 1;
        pthread_create(&t1_printers[i], NULL, t1_printer_thread, printer_id);
    }

    // Criação dos threads das impressoras T2
    pthread_t t2_printers[MAX];
    for (int i = 0; i < MAX; i++) {
        int *printer_id = malloc(sizeof(int));
        *printer_id = i + 1;
        pthread_create(&t2_printers[i], NULL, t2_printer_thread, printer_id);
    }

    // Criação dos threads de cada departamento
    pthread_t department_threads[2];
    int department_ids[2] = {1, 2};
    for (int i = 0; i < 2; i++) {
        pthread_create(&department_threads[i], NULL, department_thread, &department_ids[i]);
    }

    // Espera pela conclusão dos threads das impressoras T1
    for (int i = 0; i < MAX; i++) {
        pthread_join(t1_printers[i], NULL);
    }

    // Espera pela conclusão dos threads das impressoras T2
    for (int i = 0; i < MAX; i++) {
        pthread_join(t2_printers[i], NULL);
    }

    // Finalização dos semáforos
    sem_destroy(&t1_printer_sem);
    sem_destroy(&t2_printer_sem);

    // Exibição dos resultados
    printf("\n");
    printf("Número de impressões do setor 1 (T1): %d\n", t1_print_success);
    printf("Número de impressões do setor 2 (T2): %d\n", t2_print_success);
    printf("Número de falhas do setor 1 (T1): %d\n", t1_print_failures);
    printf("Número de falhas do setor 2 (T2): %d\n", t2_print_failures);
    printf("Número total de solicitações de impressão: %d\n", total_requests);

    return 0;
}
