/*
基于链表的多生产者多消费者问题
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

struct Node{
    int number;
    struct Node* next;
};//链表

pthread_mutex_t mutex;//互斥锁
pthread_cond_t cond;//条件变量，用于控制消费者
struct Node *head = NULL;//链表头结点指针

void *producer(void *arg);//生产者
void *consumer(void *arg);//消费者

void Init(pthread_t *ptid, pthread_t *ctid);
void Dest(pthread_t *ptid, pthread_t *ctid);

int main(int argc, char **argv)
{
    pthread_t ptid[6];//定义生产者线程
    pthread_t ctid[6];//定义消费者线程
    Init(&ptid[0], &ctid[0]);//初始化
    Dest(&ptid[0], &ctid[0]);//反初始化
    
    return 0;
}

void *producer(void *arg)
{//生产者
    while(1){//一直生产
        pthread_mutex_lock(&mutex);
        struct Node *pnew = (struct Node*)malloc(sizeof(struct Node));//创建新节点
        pnew -> number = rand() % 100;//初始化节点
        pnew -> next = head;//链表头插
        head = pnew;//头结点前移
        printf("Producer: number = %d, tid = %ld\n", pnew -> number, pthread_self());
        pthread_mutex_unlock(&mutex);

        pthread_cond_broadcast(&cond);//生产完毕后通知所有阻塞的消费者来消费

        sleep(rand() % 3);//小睡一下，避免生产过快
    }
    return NULL;
}

void *consumer(void *arg)
{
    while(1){//一直消费
        pthread_mutex_lock(&mutex);
        while(head == NULL){//使用条件变量
            pthread_cond_wait(&cond, &mutex);
        }
        struct Node *pnode = head;//取出头结点
        printf("Consumer: number = %d, tid = %ld\n", pnode -> number, pthread_self());
        head = pnode -> next;//使头结点偏移，创建新头结点
        free(pnode);//删除原先头结点
        pthread_mutex_unlock(&mutex);

        sleep(rand() % 3);//同理
    }
    return NULL;
}

void Init(pthread_t *ptid, pthread_t *ctid)
{
    //初始化互斥锁与条件变量
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    //初始化线程
    for(int i = 0; i < 6; i++){
        pthread_create(&ptid[i], NULL, producer, NULL);
    }
    for(int i = 0; i < 6; i++){
        pthread_create(&ctid[i], NULL, consumer, NULL);
    }
}

void Dest(pthread_t *ptid, pthread_t *ctid)
{
    //反初始化互斥锁与条件变量
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    //反初始化线程
    for (int i = 0; i < 6; i++){
        pthread_join(ptid[i], NULL);
    }
    for(int i = 0; i < 6; i++){
        pthread_join(ctid[i], NULL);
    }
}
