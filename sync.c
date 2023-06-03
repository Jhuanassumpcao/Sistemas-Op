#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define MAX_SETORES 5
#define MAX_SOLICITACOES 5

typedef struct {
    int id;
    int quantidade_impressoes;
    sem_t sem_setor; 	
} Setor;

typedef struct {
    int id;
    sem_t sem_impressora;
} Impressora;

Setor setores[MAX_SETORES];
Impressora *impressoras_t1;
Impressora *impressoras_t2;
int quantidade_impressoras_t1;
int quantidade_impressoras_t2;
int total_sucesso_t1 = 0;
int total_sucesso_t2 = 0;
int total_falhas_t1 = 0;
int total_falhas_t2 = 0;
int total_solicitacoes = 0;

void aguardar(int tempo_min, int tempo_max) {
    int tempo = (rand() % (tempo_max - tempo_min + 1)) + tempo_min;
    usleep(tempo * 1000);
}

void *thread_setor(void *arg) {
    int setor_id = *((int *) arg);

    for (int i = 0; i < MAX_SOLICITACOES; i++) {
        sem_wait(&setores[setor_id].sem_setor);

        printf("Setor %d está realizando solicitação #%d de impressão\n", setor_id, i + 1);

        // Verificar disponibilidade de impressora
        int impressora_id = -1;
        int tipo_impressora;
        int solicitacao_concluida = 0;

        while (!solicitacao_concluida) {
            if (rand() % 2 == 0 && quantidade_impressoras_t1 > 0) {
                impressora_id = rand() % quantidade_impressoras_t1;
                tipo_impressora = 1;
            } else if (quantidade_impressoras_t2 > 0) {
                impressora_id = rand() % quantidade_impressoras_t2;
                tipo_impressora = 2;
            }

            if (impressora_id != -1) {
                if (tipo_impressora == 1) {
                    sem_wait(&impressoras_t1[impressora_id].sem_impressora);
                    aguardar(100, 500);

                    if (rand() % 100 < 15) {
                        printf("Impressão falhou: Setor %d, solicitação #%d, impressora T1-%d\n", setor_id, i + 1, impressora_id);
                        total_falhas_t1++;
                        sem_post(&impressoras_t1[impressora_id].sem_impressora);
                        aguardar(100, 500); // Aguardar um tempo antes de tentar novamente
						printf("Setor %d está realizando solicitação #%d de impressão\n", setor_id, i + 1);
                    } else {
                        printf("Impressora T1-%d está imprimindo a solicitação #%d do Setor %d\n", impressora_id, i + 1, setor_id);
                        aguardar(100, 500);
                        printf("Impressão concluída: Setor %d, solicitação #%d, impressora T1-%d\n", setor_id, i + 1, impressora_id);
                        total_sucesso_t1++;
                        sem_post(&impressoras_t1[impressora_id].sem_impressora);
                        setores[setor_id].quantidade_impressoes++;
                        solicitacao_concluida = 1;
                    }
                } else {
                    sem_wait(&impressoras_t2[impressora_id].sem_impressora);
                    aguardar(100, 500);

                    printf("Impressora T2-%d está imprimindo a solicitação #%d do Setor %d\n", impressora_id, i + 1, setor_id);
                    aguardar(100, 500);
                    printf("Impressão concluída: Setor %d, solicitação #%d, impressora T2-%d\n", setor_id, i + 1, impressora_id);
                    total_sucesso_t2++;
                    sem_post(&impressoras_t2[impressora_id].sem_impressora);
                    setores[setor_id].quantidade_impressoes++;
                    solicitacao_concluida = 1;
                }
            } else {
                printf("Setor %d não possui impressora disponível para a solicitação #%d\n", setor_id, i + 1);
                aguardar(100, 500); // Aguardar um tempo antes de tentar novamente
            }
        }

        if (setores[setor_id].quantidade_impressoes == MAX_SOLICITACOES) {
            printf("Setor %d já teve todas as suas solicitações atendidas.\n", setor_id);
        }

        sem_post(&setores[setor_id].sem_setor);
    }

    pthread_exit(NULL);
}

void *thread_impressora_t1(void *arg) {
    int impressora_id = *((int *) arg);

    while (1) {
        sem_wait(&impressoras_t1[impressora_id].sem_impressora);

        // Verificar se há solicitações de impressão pendentes
        int solicitacao_pendente = 0;
        for (int i = 0; i < MAX_SETORES; i++) {
            if (setores[i].quantidade_impressoes < MAX_SOLICITACOES) {
                solicitacao_pendente = 1;
                break;
            }
        }

        if (solicitacao_pendente) {
            aguardar(100, 500);

            int setor_id = -1;
            int solicitacao_id = -1;

            // Encontrar uma solicitação para imprimir
            for (int i = 0; i < MAX_SETORES; i++) {
                if (setores[i].quantidade_impressoes < MAX_SOLICITACOES) {
                    setor_id = i;
                    solicitacao_id = setores[i].quantidade_impressoes;
                    break;
                }
            }

            printf("Impressora T1-%d está imprimindo a solicitação #%d do Setor %d\n", impressora_id, solicitacao_id + 1, setor_id);
            aguardar(100, 500);
            printf("Impressão concluída: Setor %d, solicitação #%d, impressora T1-%d\n", setor_id, solicitacao_id + 1, impressora_id);
            total_sucesso_t1++;
            sem_post(&impressoras_t1[impressora_id].sem_impressora);
            setores[setor_id].quantidade_impressoes++;
        } else {
            sem_post(&impressoras_t1[impressora_id].sem_impressora);
            break;
        }
    }

    pthread_exit(NULL);
}

void *thread_impressora_t2(void *arg) {
    int impressora_id = *((int *) arg);

    while (1) {
        sem_wait(&impressoras_t2[impressora_id].sem_impressora);

        // Verificar se há solicitações de impressão pendentes
        int solicitacao_pendente = 0;
        for (int i = 0; i < MAX_SETORES; i++) {
            if (setores[i].quantidade_impressoes < MAX_SOLICITACOES) {
                solicitacao_pendente = 1;
                break;
            }
        }

        if (solicitacao_pendente) {
            aguardar(100, 500);

            int setor_id = -1;
            int solicitacao_id = -1;

            // Encontrar uma solicitação para imprimir
            for (int i = 0; i < MAX_SETORES; i++) {
                if (setores[i].quantidade_impressoes < MAX_SOLICITACOES) {
                    setor_id = i;
                    solicitacao_id = setores[i].quantidade_impressoes;
                    break;
                }
            }

            printf("Impressora T2-%d está imprimindo a solicitação #%d do Setor %d\n", impressora_id, solicitacao_id + 1, setor_id);
            aguardar(100, 500);
            printf("Impressão concluída: Setor %d, solicitação #%d, impressora T2-%d\n", setor_id, solicitacao_id + 1, impressora_id);
            total_sucesso_t2++;
            sem_post(&impressoras_t2[impressora_id].sem_impressora);
            setores[setor_id].quantidade_impressoes++;
        } else {
            sem_post(&impressoras_t2[impressora_id].sem_impressora);
            break;
        }
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <numero_impressoras>\n", argv[0]);
        return 1;
    }

    int quantidade_impressoras = atoi(argv[1]);

    // Inicializar semáforos dos setores
    for (int i = 0; i < MAX_SETORES; i++) {
        sem_init(&setores[i].sem_setor, 0, 1);
        setores[i].id = i;
        setores[i].quantidade_impressoes = 0;
    }

    // Inicializar semáforos das impressoras
    quantidade_impressoras_t1 = quantidade_impressoras / 2;
    quantidade_impressoras_t2 = quantidade_impressoras - quantidade_impressoras_t1;

    impressoras_t1 = (Impressora *) malloc(quantidade_impressoras_t1 * sizeof(Impressora));
    impressoras_t2 = (Impressora *) malloc(quantidade_impressoras_t2 * sizeof(Impressora));

    for (int i = 0; i < quantidade_impressoras_t1; i++) {
        sem_init(&impressoras_t1[i].sem_impressora, 0, 1);
        impressoras_t1[i].id = i;
    }

    for (int i = 0; i < quantidade_impressoras_t2; i++) {
        sem_init(&impressoras_t2[i].sem_impressora, 0, 1);
        impressoras_t2[i].id = i;
    }

    // Inicializar gerador de números aleatórios
    srand(time(NULL));

    // Criar threads dos setores
    pthread_t threads_setores[MAX_SETORES];
    int setor_ids[MAX_SETORES];

    for (int i = 0; i < MAX_SETORES; i++) {
        setor_ids[i] = i;
        pthread_create(&threads_setores[i], NULL, thread_setor, &setor_ids[i]);
    }

    // Criar threads das impressoras T1
    pthread_t threads_impressoras_t1[quantidade_impressoras_t1];
    int impressora_ids_t1[quantidade_impressoras_t1];

    for (int i = 0; i < quantidade_impressoras_t1; i++) {
        impressora_ids_t1[i] = i;
        pthread_create(&threads_impressoras_t1[i], NULL, thread_impressora_t1, &impressora_ids_t1[i]);
    }

    // Criar threads das impressoras T2
    pthread_t threads_impressoras_t2[quantidade_impressoras_t2];
    int impressora_ids_t2[quantidade_impressoras_t2];

    for (int i = 0; i < quantidade_impressoras_t2; i++) {
        impressora_ids_t2[i] = i;
        pthread_create(&threads_impressoras_t2[i], NULL, thread_impressora_t2, &impressora_ids_t2[i]);
    }

    // Aguardar finalização das threads dos setores
    for (int i = 0; i < MAX_SETORES; i++) {
        pthread_join(threads_setores[i], NULL);
    }

    // Aguardar finalização das threads das impressoras T1
    for (int i = 0; i < quantidade_impressoras_t1; i++) {
        pthread_join(threads_impressoras_t1[i], NULL);
    }

    // Aguardar finalização das threads das impressoras T2
    for (int i = 0; i < quantidade_impressoras_t2; i++) {
        pthread_join(threads_impressoras_t2[i], NULL);
    }

  // Exibir informações
    printf("\n--- Relatório Final ---\n");
    for (int i = 0; i < MAX_SETORES; i++) {
        printf("Setor %d: %d impressões\n", i, setores[i].quantidade_impressoes);
    }

    total_solicitacoes = MAX_SETORES * MAX_SOLICITACOES;
    printf("\nNúmero total de solicitações: %d\n", total_solicitacoes);

    printf("\nImpressoras do tipo T1:\n");
    printf("Sucesso: %d impressões\n", total_sucesso_t1);
    printf("Falhas: %d impressões\n", total_falhas_t1);

    printf("\nImpressoras do tipo T2:\n");
    printf("Sucesso: %d impressões\n", total_sucesso_t2);
    printf("Falhas: %d impressões\n", total_falhas_t2);

    // Liberar memória alocada
    free(impressoras_t1);
    free(impressoras_t2);

    return 0;
}
