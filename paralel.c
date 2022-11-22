/* Sequencial.c (Roland Teodorowitsch) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

/* CONTANTES */
#define GRAU 10
#define TAG_SEND_SIZE 1
#define TAG_SEND_ARR 2
#define TAM_INI 10
#define TAM_INC 10
#define TAM_MAX 100

/* VARIAVEIS GLOBAIS */
double x[TAM_MAX], y[TAM_MAX], gabarito[TAM_MAX];

/* PROTOTIPOS */
double polinomio(double v[], int grau, double x);
void erro(char *msg_erro);

double polinomio(double a[], int grau, double x)
{
    int i;
    double res = a[0], pot = x;
    for (i = 1; i <= grau; ++i)
    {
        res += a[i] * pot;
        pot = pot * x;
    }
    return res;
}

void erro(char *msg_erro)
{
    fprintf(stderr, "ERRO: %s\n", msg_erro);
    MPI_Finalize();
    exit(1);
}

int main(int argc, char **argv)
{
    int MAIN_PID;           /* Identificador do processo */
    int process_count; /* Numero de processos */
    int i, size;
    double *vet, valor, *vresp, resposta, tempo, a[GRAU + 1];
    int hostsize; /* Tamanho do nome do nodo */
    char hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status; /* Status de retorno */

    MPI_Init(&argc, &argv);
    MPI_Get_processor_name(hostname, &hostsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &MAIN_PID);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    if (MAIN_PID == 0)
    {
        /* Gera os coeficientes do polinomio */
#pragma omp parallel for
        for (i = 0; i <= GRAU; ++i)
            a[i] = (i % 3 == 0) ? -1.0 : 1.0;

        printf("process: root -  Stoping at barrier\n");
        MPI_Barrier(MPI_COMM_WORLD);
        printf("process: root - Sending Alfas...\n");
        MPI_Bcast(a, GRAU + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        /* Preenche vetores */
        printf("process: root - GENERATING VALIDATION...\n");
#pragma omp parallel for
        for (i = 0; i < TAM_MAX; ++i)
        {
            x[i] = 0.1 + 0.1 * (double)i / TAM_MAX;
            gabarito[i] = polinomio(a, GRAU, x[i]);
        }
        printf("GAB: {\n");
        for (int i = 0; i < TAM_MAX; i++)
        {
            printf("%f, ", gabarito[i]);
        }
        printf("\n}\n\n");
        

        printf("process: root Stoping at barrier\n");
        MPI_Barrier(MPI_COMM_WORLD);
        printf("process: root leaving barrier\n");

        printf("process: root - sleep 10 sec FIRST\n");
        sleep(10);
        printf("process: root - awake\n");

        /* Gera tabela com tamanhos e tempos */
        int init = 0;
        int receivedInit = 0;
        for (size = TAM_INI; size <= TAM_MAX; size += TAM_INC)
        {
            /* Calcula */
            tempo = -MPI_Wtime();
            int process_pid = 1;

            for (int pid = 1; pid < process_count; pid++)
            {
                int fim = pid * size / (process_count - 1);
                int tam = fim - init;
                double arrToSend[tam];
                printf("process: root - MAKING CHUNK OF SIZE %d, startIndex: %d, endIndex: < %d for pid: %d...\n", tam, init, fim, pid);
                for (int i = 0; i < tam; i++)
                {
                    arrToSend[i] = x[i + init];
                }

                printf("process: root - sending size of: %d to worker: %d...\n", tam, pid);
                MPI_Send(&tam, 1, MPI_INT, pid, TAG_SEND_SIZE, MPI_COMM_WORLD);
                printf("process: root.. Sending array to worker: %d ...\n", pid);
                MPI_Send(arrToSend, tam, MPI_DOUBLE, pid, TAG_SEND_ARR, MPI_COMM_WORLD);
                init = fim;
            }

            for (int pid = 1; pid < process_count; pid++)
            {
                int fim = pid * size / (process_count - 1);
                int tam = fim - receivedInit;
                int recv[tam];
                printf("process: root.. Waiting response of worker %d...\n", pid);
                MPI_Recv(recv, tam, MPI_DOUBLE, pid, TAG_SEND_ARR, MPI_COMM_WORLD, &status);
                printf("process: root - RECEIVING CHUNK OF SIZE %d, startIndex: %d, endIndex: %d for pid: %d -> sizeof recv %ld...\n", tam, receivedInit, fim, pid, sizeof(recv));
                for (int i = 0; i < tam; i++)
                {
                    y[i + receivedInit] = recv[i];
                }
                receivedInit = fim;
            }

            tempo += MPI_Wtime();
            /* Verificacao */

            printf("process: root - sleep 10 sec\n");
            sleep(10);
            printf("Process: Root validation: {\n");
            for (i = 0; i < size; ++i)
            {
                printf("(%f - %f), ", y[i], gabarito[i]);
                if (y[i] != gabarito[i])
                {
                    printf("verificacao falhou!\n");
                }
            }
            printf("\n}\n\n");
            
            /* Mostra tempo */
            printf("%d %lf\n", size, tempo);
        }
        printf("FINISH EXECUTING EVERTIHING on main, will stop workers");
        for (int pid = 1; pid < process_count; pid++)
        {
            int stopSign = -1;
            MPI_Send(&stopSign, 1, MPI_INT, pid, TAG_SEND_SIZE, MPI_COMM_WORLD);
        }
    }
    else
    {
        double process_alfa[GRAU + 1];
        printf("Starting worker: %d - of node: %s.. waiting on barrirer.\n", MAIN_PID, hostname);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(process_alfa, GRAU + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        printf("worker: %d - of node: %s.. receiving alfas...\n", MAIN_PID, hostname);

        MPI_Barrier(MPI_COMM_WORLD);
        int size = 1;
        int elements_size;
        while (1)
        {
            printf("worker: %d - of node: %s.. WAITING FOR MAIN NODE TO SEND ARRAY SIZE ...\n", MAIN_PID, hostname);
            MPI_Recv(&elements_size, 1, MPI_INT, 0, TAG_SEND_SIZE, MPI_COMM_WORLD, &status);
            printf("worker: %d - of node: %s.. Received size of: %d ...\n", MAIN_PID, hostname, elements_size);
            if (elements_size == -1)
            {
                printf("worker: %d - of node: %s.. received -1. Will Stop ...\n", MAIN_PID, hostname);
                break;
            }

            printf("worker: %d - of node: %s.. waiting for receiving array of size: %d ...\n", MAIN_PID, hostname, elements_size);
            double processArray[elements_size];
            MPI_Recv(processArray, elements_size, MPI_DOUBLE, 0, TAG_SEND_ARR, MPI_COMM_WORLD, &status);
            printf("worker: %d - of node: %s.. receiver array ...\n", MAIN_PID, hostname);

            double answer[elements_size];
            for (int i = 0; i < elements_size; i++)
            {
                answer[i] = polinomio(process_alfa, GRAU, processArray[i]);
            }
            printf("worker: %d - of node: %s.. sending array with answers...\n", MAIN_PID, hostname);
            MPI_Send(answer, elements_size, MPI_DOUBLE, 0, TAG_SEND_ARR, MPI_COMM_WORLD);
        }
    }
    printf("process: %d - of node: %s..  stoping ...\n", MAIN_PID, hostname);
    MPI_Finalize();
    return 0;
}
