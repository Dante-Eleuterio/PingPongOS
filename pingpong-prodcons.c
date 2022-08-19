#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

#define N 5

typedef struct buffer_t
{
    struct buffer_t *prev ;  // aponta para o elemento anterior na fila
    struct buffer_t *next ;  // aponta para o elemento seguinte na fila
    int item;
} buffer_t;

buffer_t *queue;
task_t      p1, p2, p3, c1, c2;
semaphore_t s_buffer,s_item,s_vaga;

void produtor(void * arg){
    int item=0;
    buffer_t *buffer;
    while (1)
    {
        buffer=malloc(sizeof(buffer_t));
        task_sleep (1000);
        item = rand() %100;
        buffer->item=item;

        sem_down (&s_vaga);
        
        sem_down (&s_buffer);
        if(queue_size((queue_t *)queue)<=N){
            queue_append((queue_t **)&queue, (queue_t*)buffer);
            printf("p%s produziu %d\n",(char *) arg,item);
        }
        sem_up (&s_buffer);

        sem_up (&s_item);
    }
}

void consumidor(void * arg){
    int item=0;
    buffer_t *buffer;
    while (1)
    {
        sem_down (&s_item);

        sem_down (&s_buffer);
        buffer=queue;
        queue_remove((queue_t **)&queue, (queue_t*)queue);
        sem_up (&s_buffer);

        sem_up (&s_vaga);
        printf("\tc%s consumiu %d\n",(char *) arg,buffer->item);
        free(buffer);
        task_sleep (1000);
    }
}

int main (int argc, char *argv[])
{
    printf ("main: inicio\n") ;

    srand(time(NULL));
    
    ppos_init () ;

    // cria semaforos
    sem_create (&s_buffer, 1) ;
    sem_create (&s_item, 0) ;
    sem_create (&s_vaga, N) ;

    // cria tarefas
    task_create (&p1, produtor,"1");
    task_create (&p2, produtor,"2");
    task_create (&p3, produtor,"3");
    task_create (&c1, consumidor,"1");
    task_create (&c2, consumidor,"2");

    // aguarda a1 encerrar
    task_join (&p1);
    task_join (&p2);
    task_join (&p3);
    task_join (&c1);
    task_join (&c2);

    // destroi semaforos
    sem_destroy(&s_buffer);
    sem_destroy(&s_item);
    sem_destroy(&s_vaga);

    printf ("main: fim\n") ;
    task_exit (0) ;

exit (0) ;
}