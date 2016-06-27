/*
 * =====================================================================================
 *
 *       Filename:  threadpool.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/27/16 14:38:51
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
//#include <iostrem.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
using namespace std;
class CThread_pool;
class CThread_worker{
    private:
        void * arg;
        void *(*process)(void *arg);
    public:
        friend class CThread_pool;
        CThread_worker():process(NULL),arg(NULL),next(NULL){};
        CThread_worker(void*(*process_init)(void *arg_init),void *arg_init):process(process_init),arg(arg_init),next(NULL){};
        CThread_worker * next;
};
class CThread_pool{
    private:
        pthread_mutex_t queue_lock;
        pthread_cond_t queue_ready;
        
        CThread_worker *queue_head;

        int shutdown;
        pthread_t *threadid;
        int max_thread_num;
        int cur_queue_size;
    public:
        friend class CThread_worker;
        CThread_pool():queue_head(NULL),shutdown(0),threadid(NULL),max_thread_num(0),cur_queue_size(0){
            pthread_mutex_init(&queue_lock,NULL);
            pthread_cond_init(&queue_ready,NULL);
        };
        CThread_pool(int max_thread_num_init):queue_head(NULL),shutdown(0),threadid(NULL),max_thread_num(max_thread_num_init),cur_queue_size(0)
        {
            pthread_mutex_init(&queue_lock,NULL);
            pthread_cond_init(&queue_ready,NULL);
        };
        ~CThread_pool();
        void pool_create();
        int pool_add_worker(void*(*process_init)(void *arg_init),void *arg_init);
        void *thread_routine();
        static void *thread_routine_helper(void *context)
        {
            return ((CThread_pool *)context)->thread_routine();
        }
};

void * CThread_pool::thread_routine()
{
    printf("starting thread %lu\n",pthread_self());
    while(true)
    {
        pthread_mutex_lock(&queue_lock);
        while(cur_queue_size==0&&!shutdown)
        {
            printf("thread %lu is waiting\n",pthread_self());
            pthread_cond_wait(&queue_ready,&queue_lock);
        }

        if(shutdown)
        {
            pthread_mutex_unlock(&queue_lock);
            printf("thread %lu will exit\n",pthread_self());
            pthread_exit(NULL);
        }

        printf("thread %lu is starting to work\n",pthread_self());

        assert(cur_queue_size!=0);
        assert(queue_head!=NULL);
        --cur_queue_size;
        CThread_worker *worker = queue_head;
        queue_head = queue_head->next;
        pthread_mutex_unlock(&queue_lock);

        (*(worker->process))(worker->arg);
        delete worker;
    }
    pthread_exit(NULL);
}
void CThread_pool::pool_create()
{
         for(int i=0;i<max_thread_num;i++)
             pthread_create(&(threadid[i]),NULL,&thread_routine_helper,NULL);
}
int CThread_pool::pool_add_worker(void *(*process_init)(void *arg_init),void *arg_init)
{
    CThread_worker *newworker = new CThread_worker(process_init,arg_init);
    pthread_mutex_lock(&queue_lock);
    CThread_worker *member = queue_head;
    if(member!=NULL)
    {
        while(member->next!=NULL)
            member = member->next;
        member->next=newworker;
    }
    else{
        queue_head = newworker;
    }
    assert(queue_head!=NULL);

    ++cur_queue_size;
    pthread_mutex_unlock(&queue_lock);
    pthread_cond_signal(&queue_ready);
    return 0;
}
CThread_pool::~CThread_pool()
{
    if(shutdown);
    else{
        shutdown=1;
        pthread_cond_broadcast(&queue_ready);
        for(int i=0;i<max_thread_num;i++)
            pthread_join(threadid[i],NULL);
        free(threadid);
        
        CThread_worker *head=NULL;
        while(queue_head!=NULL)
        {
            head=queue_head;
            queue_head=queue_head->next;
            delete head;
        }
        head =NULL;
        pthread_mutex_destroy(&queue_lock);
        pthread_cond_destroy(&queue_ready);
    }
}


void * myprocess(void *arg)
{
    printf("threadid is %lu, working on task %d\n",pthread_self(),*(int*)arg);
    sleep(1);
    return NULL;
}

int main(int argc,char ** argv)
{
    CThread_pool *pool = new CThread_pool(3);
    int * workingnum = new int(10);
    for(int i=0;i<10;i++)
    {
        workingnum[i]=i;
        pool->pool_add_worker(myprocess,&workingnum[i]);
    }
    sleep(5);
    delete pool;
    delete workingnum;
    return 0;
}
