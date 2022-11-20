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
        MPI_Barrier(MPI_COMM_WORLD);
        printf("process: %d - of node: %s.. sending Alfas...\n", pid, hostname);
        MPI_Bcast(a, GRAU + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        /* Preenche vetores */
        printf("process: %d - of node: %s.. SendedAlfas...\n", pid, hostname);
    }
    else
    {
        double process_alfa[GRAU + 1];
        printf("Starting process: %d - of node: %s.. waiting on barirer.\n", pid, hostname);
        MPI_Barrier(MPI_COMM_WORLD);
        printf("process: %d - of node: %s.. exiting barirer.\n", pid, hostname);
        MPI_Bcast(process_alfa, GRAU + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        printf("process: %d - of node: %s.. receiving alfas...\n", pid, hostname);        
    }
    MPI_Finalize();
    return 0;
}
