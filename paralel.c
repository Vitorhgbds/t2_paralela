/* Sequencial.c (Roland Teodorowitsch) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

/* CONTANTES */
#define GRAU 10
#define TAG 1
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
    int CURRENT_PID;           /* Identificador do processo */
    int process_count; /* Numero de processos */
    int i, size;
    double *vet, valor, *vresp, resposta, tempo, a[GRAU + 1];
    int hostsize; /* Tamanho do nome do nodo */
    char hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status; /* Status de retorno */

    MPI_Init(&argc, &argv);
    MPI_Get_processor_name(hostname, &hostsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &CURRENT_PID);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    if (CURRENT_PID == 0)
    {
        /* Gera os coeficientes do polinomio */
#pragma omp parallel for
        for (i = 0; i <= GRAU; ++i)
            a[i] = (i % 3 == 0) ? -1.0 : 1.0;

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
        printf("process: root - Sending Alfas...\n");
        MPI_Bcast(&a, GRAU, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        //printf("process: root - sleep 10 sec FIRST\n");
        //sleep(10);
        //printf("process: root - awake\n");

        /* Gera tabela com tamanhos e tempos */
        int init = 0;
        int receivedInit = 0;
        for (size = TAM_INI; size <= TAM_MAX; size += TAM_INC)
        {
            /* Calcula */
            tempo = -MPI_Wtime();

            for (int pid = 1; pid < process_count; pid++)
            {
                int fim = pid * size / (process_count - 1);// + (size - TAM_INI);
                //printf("process: root - calculo: %d * %d / (%d) == %d", pid, size, (process_count - 1), fim);
                int tam = fim - init;
               // printf("process: root - calculo: %d - %d == %d", fim, init, tam);
                
                //double arrToSend[tam];
                //printf("process: root - MAKING CHUNK OF SIZE %d, startIndex: %d, endIndex: < %d for pid: %d...\n", tam, init, fim, pid);
                //for (int i = 0; i < tam; i++)
                //{
                //   arrToSend[i] = x[i + init];
                //    printf("process: root - index: %d, arrToSend[%d] = %f - x[%d + %d ==%d] - %f to worker: %d...\n", i,i, arrToSend[i], i, init, i + init, x[i + init], pid);
                //}

                printf("process: root - sending size of: %d to worker: %d...\n", tam, pid);
                MPI_Send(&tam, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD);
                MPI_Send(&init, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD);
                printf("process: root.. Sending array to worker: %d ...\n", pid);
                MPI_Send(&x[init], tam, MPI_DOUBLE, pid, TAG, MPI_COMM_WORLD);
                init = fim;
            }

            for (int pid = 1; pid < process_count; pid++)
            {

                /*int fim = pid * size / (process_count - 1)// + (size - TAM_INI);
                printf("process: root - calculo: %d * %d / (%d) == %d", pid, size, (process_count - 1), fim);
                int tam = fim - receivedInit;
                printf("process: root - calculo: %d - %d == %d", fim, receivedInit, tam);

                double recv[tam];*/
                int recv_init;
                int recv_tam;
                printf("process: root.. Waiting response of worker %d...\n", pid);
                MPI_Recv(&recv_tam, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(&recv_init, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(&y[recv_init], recv_tam, MPI_DOUBLE, pid, TAG, MPI_COMM_WORLD, &status);
                //for (int i = 0; i < tam; i++)
               // {
               //     y[i + receivedInit] = recv[i];
                //    printf("process: root - index: %d, y[%d + %d == %d] - %f | - recv[%d] - %f from worker: %d...\n", i , i, receivedInit, i + receivedInit, y[i + receivedInit], i, recv[i], pid);
                //}
                //receivedInit = fim;
            }

            tempo += MPI_Wtime();
            /* Verificacao */

           // printf("process: root - sleep 10 sec\n");
            //sleep(10);
            printf("Process: Root validation: {\n");
            for (i = 0; i < size; ++i)
            {
            //    printf("(%f - %f), ", y[i], gabarito[i]);
                if (y[i] != gabarito[i])
                {
                    printf("verificacao falhou!\n");
                }
            }
           // printf("\n}\n\n");
            
            /* Mostra tempo */
            printf("%d %lf\n", size, tempo);
        }
        printf("FINISH EXECUTING EVERTIHING on main, will stop workers");
        for (int pid = 1; pid < process_count; pid++)
        {
            int stopSign = -1;
            MPI_Send(&stopSign, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD);
        }
    }
    else
    {
        printf("Starting worker: %d - of node: %s.. waiting on barrirer.\n", CURRENT_PID, hostname);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&a, GRAU, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        printf("worker: %d - of node: %s.. receiving alfas...\n", CURRENT_PID, hostname);
        int tam;
        int init;
        while (1)
        {
            printf("worker: %d - of node: %s.. WAITING FOR MAIN NODE TO SEND ARRAY SIZE ...\n", CURRENT_PID, hostname);
            MPI_Recv(&tam, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            printf("worker: %d - of node: %s.. Received size of: %d ...\n", CURRENT_PID, hostname, tam);
            if (tam == -1)
            {
                printf("worker: %d - of node: %s.. received -1. Will Stop ...\n", CURRENT_PID, hostname);
                break;
            }
            MPI_Recv(&init, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            printf("worker: %d - of node: %s.. waiting for receiving array of size: %d ...\n", CURRENT_PID, hostname, tam);
            MPI_Recv(&x[0], tam, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD, &status);
            printf("worker: %d - of node: %s.. receiver array ...\n", CURRENT_PID, hostname);

            for (int i = 0; i < tam; i++)
            {
                printf("worker: %d - of node: %s.. WIll Calculate: %f...\n", CURRENT_PID, hostname, x[i]);
                y[i] = polinomio(a, GRAU, x[i]);
                printf("worker: %d - of node: %s.. Result: %f...\n", CURRENT_PID, hostname, y[i]);
            }
            printf("worker: %d - of node: %s.. sending array with answers...\n", CURRENT_PID, hostname);
            MPI_Send(&tam, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD);
            MPI_Send(&init, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD);
            MPI_Send(&y[0], tam, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD);
        }
    }
    printf("process: %d - of node: %s..  stoping ...\n", CURRENT_PID, hostname);
    MPI_Finalize();
    return 0;
}
