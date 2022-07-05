#include "ppos.h"
void ppos_init (){
    
    



    setvbuf (stdout, 0, _IONBF, 0) ;
}

int task_create (task_t *task,void (*start_func)(void *),void *arg){

}

void task_exit (int exit_code) ;

int task_switch (task_t *task) ;

int task_id () ;
