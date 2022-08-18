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
int dispatcher_timer;   /*Aux variable to count Dispatcher's processor time*/
int IDcounter;
int userTasks;
int globalLock;
struct sigaction action ;
struct itimerval timer;

task_t contextmain;
task_t *currentContext;
task_t DispatcherContext;
task_t *Ready_queue; /*Pointer to keep track of queue's head*/
task_t *Sleep_queue;
 
void enter_cs (int *lock)
{
    // atomic OR (Intel macro for GCC)
    while (__sync_fetch_and_or (lock, 1)){} // busy waiting
}
 
void leave_cs (int *lock)
{
    (*lock) = 0 ;
}

/*Enter the critical zone of the semaphore to do a down operation while protected by an atomic lock*/
int sem_down (semaphore_t *s){
    enter_cs(&s->lock);
    globalLock=1;
    if(s){
        s->counter--;
        if(s->counter<0){
            globalLock=0;
            leave_cs(&s->lock);
            task_suspend(&s->queue);
        }
        else{
            globalLock=0;
            leave_cs(&s->lock);
        }
        currentContext->exit_code=0;
        return currentContext->exit_code;
    }
    globalLock=0;
    leave_cs(&s->lock);
    return -1;
}

/*Enter the critical zone of the semaphore to do an up operation while protected by an atomic lock*/
int sem_up (semaphore_t *s){
    enter_cs(&s->lock);
    globalLock=1;
    if(s){
        s->counter++;
        if(s->counter<=0){
            task_resume(s->queue,&s->queue);
        }
        globalLock=0;
        leave_cs(&s->lock);
        return 0;
    }
    globalLock=0;
    leave_cs(&s->lock);
    return-1;
}

/*Creates a semaphore*/
int sem_create (semaphore_t *s, int value){
    globalLock=1;
    s->counter=value;
    s->created=1;
    s->destroyed=0;
    s->lock=0;
    globalLock=0;
    if(s->queue)
        return -1;
    else
        return 0;
}

/*Destroys a semaphore*/
int sem_destroy (semaphore_t *s){
    globalLock=1;
    if(s->created && !s->destroyed){
        task_t *aux=s->queue;
        task_t *resumed;
        for (int i = 0; i < queue_size((queue_t *) s->queue); i++){
            resumed=aux;
            resumed->exit_code=-1;
            aux=aux->next;
            task_resume(resumed,&s->queue);
        }
        s->destroyed=1;
        globalLock=0;
        return 0;
    }
    globalLock=0;
    return -1;
}


unsigned int systime(){
    return ticks;
}

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


void task_resume (task_t * task, task_t **queue){
    globalLock=1;
    queue_remove((queue_t **) queue, (queue_t*)task);
    queue_append((queue_t **) &Ready_queue, (queue_t*)task);
    task->quantum=20;
    task->status=READY;
    globalLock=0;
}

void task_suspend (task_t **queue){
    globalLock=1;
    queue_remove((queue_t **) &Ready_queue, (queue_t*)currentContext);
    queue_append((queue_t **) queue, (queue_t*)currentContext);
    currentContext->status=SUSPENDED;
    globalLock=0;
    task_yield();
}

/*Wake up all the tasks that are due*/
void wake_up(){
    globalLock=1;
    task_t *aux=Sleep_queue;
    task_t *awaken;
    if(queue_size((queue_t *) Sleep_queue)>0){
        for (int i = 0; i < queue_size((queue_t *) Sleep_queue); i++){
            if(aux->wake_up_time<=systime()){
                awaken=aux;
                aux=aux->next;
                task_resume(awaken,&Sleep_queue);
            }else
                aux=aux->next;
        }
    }
    globalLock=0;
}

/*Puts the task to sleep and sets an alarm*/
void task_sleep(int t){
    currentContext->wake_up_time=(systime()+t);
    task_suspend(&Sleep_queue);
}

/*Suspend the current task until the joined task exits*/
int task_join(task_t *task){
    globalLock=1;
    if(!task || task->status==FINISHED)
        return -1;
    task_suspend(&task->joins_queue);
    globalLock=0;
    return(currentContext->exit_code);
}

/*Resume all the tasks that have joined exiting task*/
void resume_joined_tasks(int id,int exit_code){
    globalLock=1;
    while(currentContext->joins_queue){
        currentContext->joins_queue->exit_code=exit_code;
        task_resume(currentContext->joins_queue,&currentContext->joins_queue);
    }
    globalLock=0;
}

task_t *scheduler(){
    task_t* first=Ready_queue;
    task_t* aux=first;
    
    if(Ready_queue){
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
    }
    return (Ready_queue);
}

void Dispatcher(){
    int time=0;
    task_t *next_t;
    while(userTasks>0){
        if(queue_size((queue_t *) Sleep_queue)>0){
            wake_up();
        }
        next_t=scheduler();
        if(next_t!=NULL){
            // printf("next_t = %d\n",next_t->id);
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
    if(globalLock){
        return;
    }
    else{
        printf("preemptei a %d\n",currentContext->id);
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
    contextmain.wake_up_time=0;
    queue_append((queue_t **) &Ready_queue, (queue_t*)&contextmain) ;
    userTasks++;
}

void ppos_init (){
    setvbuf (stdout, 0, _IONBF, 0) ;
    globalLock=0;
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
    
    task_create(&DispatcherContext,Dispatcher,NULL);
    DispatcherContext.preemptable=0;
    set_timer();
}

int task_create (task_t *task,void (*start_func)(void *),void *arg){
    globalLock=1;
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
    task->wake_up_time=0;
    if(task->id>=2){
        queue_append((queue_t **) &Ready_queue, (queue_t*) task) ;
        userTasks++;
    }
    makecontext(&task->context, (void*)(*start_func), 1,arg) ;
    globalLock=0;
    return task->id;
}

void task_exit (int exit_code){
    globalLock=1;
    if(task_id()==1){ /*Check if it's the dispatcher*/
        free(DispatcherContext.context.uc_stack.ss_sp);
        currentContext->Exe_time=(systime()-currentContext->Exe_time);
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",currentContext->id,currentContext->Exe_time,currentContext->Pro_time,currentContext->activations);
        globalLock=0;
        task_switch(&contextmain);
    }
    else{
        currentContext->status=FINISHED;
        resume_joined_tasks(task_id(),exit_code);
        currentContext->Exe_time=(systime()-currentContext->Exe_time);
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",currentContext->id,currentContext->Exe_time,currentContext->Pro_time,currentContext->activations);
        globalLock=0;
        task_switch(&DispatcherContext);
    }
}

int task_switch (task_t *task) {
    globalLock=1;
    // printf("Task atual: %d, nova task: %d \n",currentContext->id,task->id);
    task_t *aux;
    aux=currentContext;
    task->activations++;
    currentContext=task;
    if(task->id==1)
        dispatcher_timer=systime(); /*Used to calculate the dispatcher processor timer*/
    globalLock=0;
    swapcontext (&aux->context, &task->context) ;
    return(0);
}

int task_id () {
    return currentContext->id;
}
