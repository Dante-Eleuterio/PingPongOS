/*
*   PingPong OS 
*   GRR20206686 Dante Eleutério dos Santos  
*/

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
task_t *queue; /*Pointer to keep track of queue's head*/
/*Counter for the ids*/
int IDcounter;
int userTasks;

void task_setprio (task_t *task, int prio){
    if(prio<=20 && prio >=-20){
        if(task){
            task->Sprio = prio;
            task->Dprio = prio;
        }
        else{
            currentContext->Sprio=prio;
            currentContext->Dprio=prio;
        }
    }
    else
        fprintf(stderr,"Priority greater than 20 or lesser than -20");
}

int task_getprio (task_t *task){
    if(task)
        return task->Sprio;
    else
        return currentContext->Sprio;
}

/*Control goes back to Dispatcher*/
void task_yield (){
    task_switch(&DispatcherContext);
}


task_t *scheduler(){
    task_t* first=queue;
    task_t* aux=first->next;
    first->Dprio=first->Sprio; /*Reset next task's priority*/
    while (aux!=first){
        aux->Dprio--;
        aux=aux->next; /*Goes through queue aging the other tasks*/
    }
    aux=first->next;
    while (aux!=first){
        if(queue->Dprio>aux->Dprio) /*Finds the task with the smaller priority*/
            queue=aux;
        aux=aux->next;
    }
    return (first);
}

void Dispatcher(){

    task_t *next_t;
    while(userTasks>0){
        next_t=scheduler();
        if(next_t!=NULL){
            task_switch(next_t);
            switch (next_t->status)
            {
            case READY:
                break;
            case FINISHED: /*Removes the task from the queue if it's done*/
                if(queue_size((queue_t*) queue)!=0){
                    queue_remove((queue_t **) &queue,(queue_t*) next_t);
                    userTasks--;
                    free(next_t->context.uc_stack.ss_sp);
                }
                break;
            case SUSPENDED:
                break;
            default:
                break;
            }
        }
    }
    task_exit(0);
}


void ppos_init (){
    
    userTasks=0;
    IDcounter=0;
    contextmain.id=0;
    contextmain.status=READY;
    contextmain.next=NULL;
    contextmain.prev=NULL;
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
    task->status=READY;
    task->next=NULL;
    task->prev=NULL;
    task->preemptable=0;
    task->Sprio=0;
    task->Dprio=0;
    if(task->id>=2){
        queue_append((queue_t **) &queue, (queue_t*) task) ;
        userTasks++;
    }
    makecontext(&task->context, (void*)(*start_func), 1,arg) ;
    
    return task->id;
}

void task_exit (int exit_code){
    if(task_id()==1){ /*Check if it's the dispatcher*/
        free(DispatcherContext.context.uc_stack.ss_sp);
        task_switch(&contextmain);
    }
    else{
        currentContext->status=FINISHED;
        task_switch(&DispatcherContext);
    }
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
