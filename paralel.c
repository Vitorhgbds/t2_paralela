/* Sequencial.c (Roland Teodorowitsch) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

/* CONTANTES */
#define GRAU 400
#define TAG_SEND_SIZE 1
#define TAG_SEND_ARR 2
#define TAM_INI 1000000
#define TAM_INC 1000000
#define TAM_MAX 10000000

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
    int pid;           /* Identificador do processo */
    int process_count; /* Numero de processos */
    int i, size;
    double *vet, valor, *vresp, resposta, tempo, a[GRAU + 1];
    int hostsize; /* Tamanho do nome do nodo */
    char hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status; /* Status de retorno */

    MPI_Init(&argc, &argv);
    MPI_Get_processor_name(hostname, &hostsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    if (pid == 0)
    {
        /* Gera os coeficientes do polinomio */
#pragma omp parallel for
        for (i = 0; i <= GRAU; ++i)
            a[i] = (i % 3 == 0) ? -1.0 : 1.0;

        printf("process: %d - of node: %s Stoping at barrier\n", pid, hostname);
        MPI_Barrier(MPI_COMM_WORLD);
        printf("process: %d - of node: %s.. Sending Alfas...\n", pid, hostname);
        MPI_Bcast(a, GRAU + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        /* Preenche vetores */
        printf("process: %d - of node: %s.. GENERATING VALIDATION...\n", pid, hostname);
#pragma omp parallel for
        for (i = 0; i < TAM_MAX; ++i)
        {
            x[i] = 0.1 + 0.1 * (double)i / TAM_MAX;
            gabarito[i] = polinomio(a, GRAU, x[i]);
        }

        printf("process: %d - of node: %s Stoping at barrier\n", pid, hostname);
        MPI_Barrier(MPI_COMM_WORLD);

        printf("process: %d - of node: %s sleep 10 sec\n", pid, hostname);
        sleep(10);
        printf("process: %d - of node: %s awake\n", pid, hostname);

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
                for (int i = 0; i < tam; i++)
                {
                    arrToSend[i] = x[i + init];
                }

                printf("process: %d - of node: %s.. sending size of: %d to worker: %d...\n", pid, hostname, tam, pid);
                MPI_Send(&tam, 1, MPI_INT, pid, TAG_SEND_SIZE, MPI_COMM_WORLD);
                printf("process: %d - of node: %s.. Senfing array to worker: %d ...\n", pid, hostname, pid);
                MPI_Send(arrToSend, tam, MPI_DOUBLE, pid, TAG_SEND_ARR, MPI_COMM_WORLD);
                init = fim;
            }

            for (int pid = 1; pid < process_count; pid++)
            {
                int fim = pid * size / (process_count - 1);
                int tam = fim - receivedInit;
                int recv[tam];
                MPI_Recv(recv, tam, MPI_DOUBLE, pid, TAG_SEND_ARR, MPI_COMM_WORLD, &status);
                printf("process: %d - of node: %s.. Receving partial array from worker: %d ...\n", pid, hostname, pid);
                for (int i = 0; i < tam; i++)
                {
                    y[i + receivedInit] = recv[i];
                }
                receivedInit = fim;
            }

            tempo += MPI_Wtime();
            /* Verificacao */
            for (i = 0; i < size; ++i)
            {
                if (y[i] != gabarito[i])
                {
                    erro("verificacao falhou!");
                }
            }
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
        printf("Starting process: %d - of node: %s.. waiting on barrirer.\n", pid, hostname);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(process_alfa, GRAU + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        printf("process: %d - of node: %s.. receiving alfas...\n", pid, hostname);

        MPI_Barrier(MPI_COMM_WORLD);
        int size = 1;
        int elements_size;
        while (1)
        {
            printf("process: %d - of node: %s.. WAITING FOR MAIN NODE TO SEND ARRAY WITH DATA %d ...\n", pid, hostname, elements_size);
            MPI_Recv(&elements_size, 1, MPI_INT, 0, TAG_SEND_SIZE, MPI_COMM_WORLD, &status);
            if (elements_size == -1)
            {
                printf("process: %d - of node: %s.. received -1. Will Stop ...\n", pid, hostname);
                break;
            }

            printf("process: %d - of node: %s.. receiver array size of: %d ...\n", pid, hostname, elements_size);
            double processArray[elements_size];
            MPI_Recv(processArray, elements_size, MPI_DOUBLE, 0, TAG_SEND_ARR, MPI_COMM_WORLD, &status);
            printf("process: %d - of node: %s.. receiver array ...\n", pid, hostname);
            double answer[elements_size];
            for (int i = 0; i < elements_size; i++)
            {
                answer[i] = polinomio(process_alfa, GRAU, processArray[i]);
            }
            MPI_Send(answer, elements_size, MPI_DOUBLE, 0, TAG_SEND_ARR, MPI_COMM_WORLD);
        }
    }
    printf("process: %d - of node: %s..  stoping ...\n", pid, hostname);
    MPI_Finalize();
    return 0;
}
