/*
*   PingPong OS 
*   GRR20206686 Dante Eleutério dos Santos  
*/

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
int ticks;
int lock;
int dispatcher_timer;   /*Aux variable to count Dispatcher's processor time*/
int IDcounter;
int userTasks;

struct sigaction action ;
struct itimerval timer;

task_t contextmain;
task_t *currentContext;
task_t DispatcherContext;
task_t *Ready_queue; /*Pointer to keep track of queue's head*/
task_t *Suspend_queue;
unsigned int systime(){
    return ticks;
}

void task_setprio (task_t *task, int prio){
    lock=1;
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
    lock=0;
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

void print_queue1(){
    lock=0;
    task_t *aux=Ready_queue;
    printf("\n");
    for (int i = 0; i < queue_size((queue_t*) Ready_queue); i++)
    {
        printf("%d ",aux->id);
        aux=aux->next;
    }
    printf("\n");
    lock=1;
}
void print_queue2(){
    lock=0;
    task_t *aux=Suspend_queue;
    printf("\n");
    for (int i = 0; i < queue_size((queue_t*)Suspend_queue); i++)
    {
        printf("%d ",aux->id);
        aux=aux->next;
    }
    printf("\n");
    lock=1;
}


void task_resume (task_t * task, task_t **queue){
    queue_remove((queue_t **) queue, (queue_t*)task);
    queue_append((queue_t **) &Ready_queue, (queue_t*)task);
    task->status=READY;
}

void task_suspend (task_t **queue){
    queue_remove((queue_t **) &Ready_queue, (queue_t*)currentContext);
    queue_append((queue_t **) queue, (queue_t*)currentContext);
    currentContext->status=SUSPENDED;
}


int task_join(task_t *task){
    lock=1;
    if(!task || task->status==FINISHED)
        return -1;
    task_suspend(&task->joins_queue);
    lock=0;
    task_yield();
    return(currentContext->exit_code);
}

void resume_joined_tasks(int id,int exit_code){
    lock =1;

    while(currentContext->joins_queue){
        currentContext->joins_queue->exit_code=exit_code;
        task_resume(currentContext->joins_queue,&currentContext->joins_queue);
    }
    lock=0;
}

task_t *scheduler(){
    lock=1;
    task_t* first=Ready_queue;
    task_t* aux=first;
    do{
        if(Ready_queue->Dprio>aux->Dprio) /*Finds the task with the smaller priority*/
            Ready_queue=aux;
        aux=aux->next;
    }while(aux!=first);
    Ready_queue->Dprio=Ready_queue->Sprio; /*Reset next task's priority*/
    aux=Ready_queue->next;
    while (aux!=Ready_queue){
        aux->Dprio--;
        aux=aux->next; /*Goes through Ready_queue aging the other tasks*/
    }
    lock=0;
    return (Ready_queue);
}

void Dispatcher(){
    int time=0;
    task_t *next_t;
    while(userTasks>0){
        next_t=scheduler();
        if(next_t!=NULL){
            next_t->status=BUSY;
            time=systime(); 
            DispatcherContext.Pro_time+=(time-dispatcher_timer); /*Calculate the Dispatcher processor time*/
            task_switch(next_t);
            next_t->Pro_time+=(systime()-time); /*Calculate the task processor time*/
            switch (next_t->status)
            {
            case READY:
                break;
            case FINISHED: /*Removes the task from the Ready_queue if it's done*/
                if(queue_size((queue_t*) Ready_queue)!=0){
                    queue_remove((queue_t **) &Ready_queue,(queue_t*) next_t);
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

void quantum_handler(){
    ticks++;
    
    if(!currentContext->preemptable){ /*Check if the current task is preemptable*/
        return;
    }
    if(currentContext->quantum>0){ /* Check if it's time to preempt it*/
        --currentContext->quantum;
        return;
    }
    if(lock){ /*Lock preemptions if the task is inside a kernel function*/
        return;
    }
    else{
        currentContext->quantum=20;
        currentContext->status=READY;
        task_switch(&DispatcherContext);
    }
}

void set_timer(){ /*Start the timer*/
    action.sa_handler = quantum_handler ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0){
        perror ("Sigaction Error: ") ;
        exit (1) ;
    }
    
    timer.it_value.tv_usec = 1000 ;      
    timer.it_interval.tv_usec = 1000 ;
    if (setitimer (ITIMER_REAL, &timer, 0) < 0){
        perror ("Setitimer Error: ") ;
        exit (1) ;
    }
}

void create_main(){
    contextmain.id=0;
    contextmain.status=READY;
    contextmain.next=NULL;
    contextmain.prev=NULL;
    contextmain.preemptable=1;
    contextmain.Exe_time=systime();
    contextmain.Pro_time=0;
    contextmain.Sprio=0;
    contextmain.Dprio=0;
    contextmain.quantum=20;
    contextmain.activations=0;
    contextmain.exit_code=-1;
    queue_append((queue_t **) &Ready_queue, (queue_t*)&contextmain) ;
    userTasks++;
}

void ppos_init (){
    set_timer();
    lock=1;
    dispatcher_timer=0;
    userTasks=0;
    IDcounter=0;
    ticks=0;
    char *stack;
    getcontext(&contextmain.context);
    stack = malloc (STACKSIZE) ;
    if (stack)
    {
        contextmain.context.uc_stack.ss_sp = stack ;
        contextmain.context.uc_stack.ss_size = STACKSIZE ;
        contextmain.context.uc_stack.ss_flags = 0 ;
        contextmain.context.uc_link = 0 ;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        exit (1) ;
    }
    
    create_main();
    currentContext=&contextmain;
    
    setvbuf (stdout, 0, _IONBF, 0) ;
    task_create(&DispatcherContext,Dispatcher,NULL);
    DispatcherContext.preemptable=0;
    lock=0;
}

int task_create (task_t *task,void (*start_func)(void *),void *arg){
    lock=1;
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
    task->Exe_time=systime();
    task->Pro_time=0;
    task->id=IDcounter;
    task->status=READY;
    task->next=NULL;
    task->prev=NULL;
    task->preemptable=1;
    task->Sprio=0;
    task->Dprio=0;
    task->quantum=20;
    task->activations=0;
    if(task->id>=2){
        queue_append((queue_t **) &Ready_queue, (queue_t*) task) ;
        userTasks++;
    }
    makecontext(&task->context, (void*)(*start_func), 1,arg) ;
    lock=0;
    return task->id;
}

void task_exit (int exit_code){
    lock=1;
    if(task_id()==1){ /*Check if it's the dispatcher*/
        free(DispatcherContext.context.uc_stack.ss_sp);
        currentContext->Exe_time=(systime()-currentContext->Exe_time);
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",currentContext->id,currentContext->Exe_time,currentContext->Pro_time,currentContext->activations);
        task_switch(&contextmain);
    }
    else{
        currentContext->status=FINISHED;
        resume_joined_tasks(task_id(),exit_code);
        currentContext->Exe_time=(systime()-currentContext->Exe_time);
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",currentContext->id,currentContext->Exe_time,currentContext->Pro_time,currentContext->activations);
        task_switch(&DispatcherContext);
    }
}

int task_switch (task_t *task) {
    lock=1;
    task_t *aux;
    aux=currentContext;
    task->activations++;
    currentContext=task;
    if(task->id==1)
        dispatcher_timer=systime(); /*Used to calculate the dispatcher processor timer*/
    lock=0;
    swapcontext (&aux->context, &task->context) ;
    return(0);
}

int task_id () {
    return currentContext->id;
}
