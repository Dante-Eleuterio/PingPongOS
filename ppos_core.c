#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
/*Keep the current context and main's contexts*/
task_t contextmain;
task_t *currentContext;

/*Counter for the ids*/
int counter;

void ppos_init (){
    counter=0;
    contextmain.id=0;
    contextmain.status=1;
    contextmain.next=NULL;
    contextmain.prev=NULL;
    contextmain.preemptable=0;
    currentContext=&contextmain;
    setvbuf (stdout, 0, _IONBF, 0) ;
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
    task->id=counter;
    task->status=1;
    task->next=NULL;
    task->prev=NULL;
    task->preemptable=0;
    makecontext (&task->context, (void*)(*start_func), 1,arg) ;
    
    return task->id;
}

void task_exit (int exit_code){
    task_switch(&contextmain);
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
