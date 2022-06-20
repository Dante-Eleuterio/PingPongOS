#include <stdio.h>
#include "queue.h"

int queue_size(queue_t *queue){
    if(queue==NULL)
        return 0;
    int size=1;
    queue_t *first=queue;
    queue_t *aux=first->next;
    while(aux!=first){
        aux=aux->next;
        size++;
    }
    return size;
}

int queue_append (queue_t **queue, queue_t *elem){
    if(!queue){
        fprintf(stderr,"Queue does not exist \n");
        return -1;
    }
    if(!elem){
        fprintf(stderr,"Elem does not exist \n");
        return -1;
    }
    if(elem->next!=NULL){
        fprintf(stderr,"Elem already exists in a queue\n");
        return -1;
    }
    
    if(!(*queue))
    {
        (*queue)=elem;
        elem->next=elem;
        elem->prev=elem;
        return 0;
    }

    elem->prev=(*queue)->prev;
    elem->next=(*queue);
    (*queue)->prev->next=elem;
    (*queue)->prev=elem;
    return 0;
}
void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    queue_t *aux=queue;
    if(!aux)
        fprintf(stdout,"%s: []\n",name);
    else\
    {
        fprintf(stdout,"%s: [",name);
        do{
            print_elem(aux);
            if(aux->next!=queue)
                fprintf(stdout," ");
            aux=aux->next;
        } while (aux!=queue);
        fprintf(stdout,"]\n");

    }
}

int find_elem(queue_t **queue,queue_t *elem){
    queue_t *aux=(*queue);
    for (int i = 0; i < queue_size(*queue); i++){
        if(aux==elem)
            return 1;
        aux=aux->next;
    }
    return 0;    
}

int queue_remove (queue_t **queue, queue_t *elem){
    if(!queue){
        fprintf(stderr,"Queue does not exist \n");
        return -1;
    }
    
    if(queue_size(*queue)==0){
        fprintf(stderr,"Empty queue\n");
        return -1;
    }
    if(!elem){
        fprintf(stderr,"Elem does not exist \n");
        return -1;
    }
    if(!find_elem(queue,elem)){
        fprintf(stderr,"Elem does not belong to this queue\n");
        return -1;
    }

    if(queue_size(*queue)==1)
    {
        (*queue)->next=NULL;
        (*queue)->prev=NULL;
        (*queue)=NULL;
        return 0;
    }
    queue_t *first=(*queue);
    queue_t *aux=first;
    int found=0;
    do
    {
        if(aux==elem){
            if((*queue)==aux)
                (*queue)=first->next;
            aux->prev->next=aux->next;
            aux->next->prev=aux->prev;
            aux->next=NULL;
            aux->prev=NULL;
            found=1;
        }
        aux=aux->next; 
    }
    while ((aux!=first)&&(!found));

    return 0;
}
