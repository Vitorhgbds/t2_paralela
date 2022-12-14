/* Sequencial.c (Roland Teodorowitsch) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

/* CONTANTES */
#define GRAU 400
#define TAG 1
#define TAM_INI 1000000
#define TAM_INC 1000000
#define TAM_MAX 10000000

struct SizeTime
{
    int size;
    double time;
};

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
    int CURRENT_PID;   /* Identificador do processo */
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
        struct SizeTime sizes_with_time[10];
        int loop;
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

        printf("process: root Stoping at barrier\n");
        MPI_Barrier(MPI_COMM_WORLD);
        printf("process: root - Sending Alfas...\n");
        MPI_Bcast(&a, GRAU, MPI_DOUBLE, 0, MPI_COMM_WORLD);



        /* Gera tabela com tamanhos e tempos */
        for (size = TAM_INI, loop = 0; size <= TAM_MAX; size += TAM_INC, loop++)
        {
            /* Calcula */
            tempo = -MPI_Wtime();

            int init = 0;
            for (int pid = 1; pid < process_count; ++pid)
            {
                int fim = pid * size / (process_count - 1); 
                printf("process: root - calculo: %d * %d / (%d) == %d\n", pid, size, (process_count - 1), fim);
                int tam = fim - init;
                printf("process: root - calculo: %d - %d == %d\n", fim, init, tam);

                printf("process: root - sending size of: %d to worker: %d...\n", tam, pid);
                MPI_Send(&tam, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD);
                MPI_Send(&init, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD);
                printf("process: root.. Sending array to worker: %d ...\n", pid);
                MPI_Send(&x[init], tam, MPI_DOUBLE, pid, TAG, MPI_COMM_WORLD);
                init = fim;
            }

            for (int pid = 1; pid < process_count; ++pid)
            {

                int recv_init;
                int recv_tam;
                printf("process: root.. Waiting response of worker %d...\n", pid);
                MPI_Recv(&recv_tam, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(&recv_init, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD, &status);
                printf("process: root.. will receive from pid: %d - from Init: %d to %d ...\n", pid, recv_init, recv_tam);
                MPI_Recv(&y[recv_init], recv_tam, MPI_DOUBLE, pid, TAG, MPI_COMM_WORLD, &status);
            }

            tempo += MPI_Wtime();
            /* Verificacao */
            printf("Process: Root validation: {\n");
            for (i = 0; i < size; ++i)
            {
                if (y[i] != gabarito[i])
                {
                    printf("(%f - %f) - FALHOU , \n", y[i], gabarito[i]);
                    erro("verificacao falhou!\n");
                }
            }

            /* Mostra tempo */
            printf("%d %lf\n", size, tempo);
            sizes_with_time[loop].size = size;
            sizes_with_time[loop].time = tempo;
        }
        printf("FINISH EXECUTING EVERTIHING on main, will stop workers\n");
        for (int pid = 1; pid < process_count; pid++)
        {
            int stopSign = -1;
            MPI_Send(&stopSign, 1, MPI_INT, pid, TAG, MPI_COMM_WORLD);
        }
            printf("\n\n\ntamanho,tempo\n");
        for (int i = 0; i < 10; i++)
        {
            printf("%d,%lf\n", sizes_with_time[i].size, sizes_with_time[i].time);
        }
    }
    else
    {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&a, GRAU, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        int tam;
        int init;
        while (1)
        {
            MPI_Recv(&tam, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            if (tam == -1)
            {
                printf("worker: %d - of node: %s.. received -1. Will Stop ...\n", CURRENT_PID, hostname);
                break;
            }
            MPI_Recv(&init, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(&x[0], tam, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD, &status);

            for (int i = 0; i < tam; i++)
            {   
                y[i] = polinomio(a, GRAU, x[i]);
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
