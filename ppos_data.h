// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#define FINISHED 0
#define READY 1
#define SUSPENDED 2
#define BUSY 3
#define SLEEPING 4;
#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  struct task_t *joins_queue;           //Tarefas joined
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  short preemptable ;			// pode ser preemptada?
  int Sprio;        //Prioridade estatica
  int Dprio;        //Prioridade Dinamica
  int quantum;      //Tempo de quantum de cada tarefa
  int activations;  //Numero de ativacoes
  int Exe_time;     //Tempo total de execucao
  int Pro_time;     //Tempo total de processamento
  int exit_code;    //Codigo de saida do join
  int wake_up_time;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

