#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
/*Keep the current context and main's contexts*/
task_t contextmain;
task_t *currentContext;
task_t DispatcherContext;
task_t *queue;
/*Counter for the ids*/
int IDcounter;
int userTasks;

void task_yield (){
    task_switch(&DispatcherContext);
}

task_t *scheduler(){
    task_t* prox=queue;
    queue=queue->next;
    return (prox);
}

void Dispatcher(){

    task_t *proxima;
    while(userTasks>0){
        proxima=scheduler();
        if(proxima!=NULL){
            task_switch(proxima);
        }
    }
    task_exit(0);
}


void ppos_init (){
    
    userTasks=0;
    IDcounter=0;
    contextmain.id=0;
    contextmain.status=1;
    contextmain.next=NULL;
    contextmain.prev=NULL;
    contextmain.preemptable=0;
    currentContext=&contextmain;


    setvbuf (stdout, 0, _IONBF, 0) ;
    task_create(&DispatcherContext,Dispatcher,NULL);
}

int task_create (task_t *task,void (*start_func)(void *),void *arg){
    char *stack;
    getcontext(&task->context);
    stack = malloc (STACKSIZE) ;
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack ;
        task->context.uc_stack.ss_size = STACKSIZE ;
        task->context.uc_stack.ss_flags = 0 ;
        task->context.uc_link = 0 ;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        exit (1) ;
    }
    IDcounter++;
    task->id=IDcounter;
    task->status=1;
    task->next=NULL;
    task->prev=NULL;
    task->preemptable=0;
    
    if(task->id>=2){
        queue_append((queue_t **) &queue, (queue_t*) task) ;
        userTasks++;
    }
    makecontext(&task->context, (void*)(*start_func), 1,arg) ;
    
    return task->id;
}

void task_exit (int exit_code){
    if(queue_size((queue_t*) queue)!=0){
        queue_remove((queue_t **) &queue,(queue_t*) currentContext);
        userTasks--;
    }
    if(task_id()==1)
        task_switch(&contextmain);
    else
        task_switch(&DispatcherContext);
}

int task_switch (task_t *task) {
    task_t *aux;
    aux=currentContext;
    currentContext=task;
    swapcontext (&aux->context, &task->context) ;
    return(0);
}

int task_id () {
    return currentContext->id;
}
