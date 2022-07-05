#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
/*Keep the current context and main's contexts*/
task_t contextmain;
task_t *currentContext;
task_t Dispatcher;
/*Counter for the ids*/
int IDcounter;
int userTasks;
task_t task_yield (){

}

void scheduler(){

}

void ppos_init (){
    IDcounter=0;
    contextmain.id=0;
    contextmain.status=1;
    contextmain.next=NULL;
    contextmain.prev=NULL;
    contextmain.preemptable=0;
    currentContext=&Dispatcher;
    setvbuf (stdout, 0, _IONBF, 0) ;
    task_create(&Dispatcher,scheduler,NULL);
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
    counter++;
    task->id=IDcounter;
    task->status=1;
    task->next=NULL;
    task->prev=NULL;
    task->preemptable=0;
    
    makecontext (&task->context, (void*)(*start_func), 1,arg) ;
    
    return task->id;
}

void task_exit (int exit_code){
    task_switch(&Dispatcher);
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
