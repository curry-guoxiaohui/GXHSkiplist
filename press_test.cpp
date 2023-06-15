#include <iostream>
#include <chrono>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include "skiplist.h"
#include <string>

SkipList<std::string,std::string> sklist(18);

void* insertElement(void* arg)
{
    int tmp = *(int*)arg;
    for(int i = 0; i < tmp;++i )
    {
        sklist.insert_element(std::to_string(rand() % tmp),"aa");
    }

    pthread_exit(NULL);
}

void* searchElement(void* arg)
{
    int tmp = *(int*)arg;
    for(int i = 0; i < tmp;++i )
    {
        std::string s;
        sklist.search_element(std::to_string(rand() % tmp),s);
    }

    pthread_exit(NULL);
}

int main(int argc,char* argv[])
{
    if(argc < 2){
        printf("Usage:%s <num>",argv[0]);
        exit(-1);
    }

    int TEST_COUNT = atoi(argv[1]);
    srand(time(NULL));

    int rc;
    int i;

    auto start = std::chrono::high_resolution_clock::now();

    pthread_t writepthread;
    rc = pthread_create(&writepthread,NULL,insertElement,&TEST_COUNT);

    if(rc)
    {
        std::cout << "Error:create writepthread fail," << rc << std::endl;
        exit(-1);
    }

    void* ret;
    if(pthread_join(writepthread,&ret) != 0){
        perror("thread exit error");
        exit(3);
    }

    auto finish = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = finish - start;

     auto start1 = std::chrono::high_resolution_clock::now();

    pthread_t readthread;
    rc = pthread_create(&readthread,NULL,searchElement,&TEST_COUNT);

    if(rc)
    {
        std::cout << "Error:create readthread fail," << rc << std::endl;
        exit(-1);
    }

    
    if(pthread_join(readthread,&ret) != 0){
        perror("thread exit error");
        exit(3);
    }

    auto finish1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = finish1 - start1;

    std::cout << "insert elapsed:" << elapsed.count() << std::endl;
    std::cout << "search elapsed:" << elapsed1.count() << std::endl;
    return 0;
}