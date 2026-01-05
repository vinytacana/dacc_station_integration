
// edge trigged program -> epoll will give us an event AND something to read only once, even if read() is not called   
#include <stdio.h>
#include <stdbool.h>
// some flag definitions
#include <fcntl.h>
#include <unistd.h>

#include <sys/epoll.h>

#define MAX_EPOLL_EVENTS 16
#define MAX_INPUT_PIPES 4

int main(){
    
    int epoll_File_Descriptor = -1;
    // 
    int events_Ready = 0; 
    
    int info[MAX_INPUT_PIPES][2];

    char message [] = "Hello there";
    char receive_Buffer[4046];
    
    // settings we will give to epoll
    struct epoll_event e_info[MAX_INPUT_PIPES];
    
    // will contain all the events it gets
    struct epoll_event event_Queue[MAX_EPOLL_EVENTS];
    
    epoll_File_Descriptor = epoll_create1(0);
    if (epoll_File_Descriptor == -1) 
        printf("Couldn't create epoll instance!\n");

    printf("Watching for type %d events.\n", EPOLLIN);
    
    for(int i = 0; i < MAX_INPUT_PIPES; i++){
        pipe(info[i]);

        printf("fd for %d = %d, %d\n", i, info[i][0], info[i][1]);
        
        // for every fd in this array, block until one or multiple are avaliable for reading
        e_info[i].events = EPOLLIN | EPOLLET;

        // telling which fd to monitor
        // read-end of the pipe
        e_info[i].data.fd = info[i][0];

        // adding fd to epoll
        if(epoll_ctl(epoll_File_Descriptor, EPOLL_CTL_ADD, info[i][0], &e_info[i]) == -1)
            printf("Couldn't add to epoll!\n");
        
        
        if( i % 2 == 0){
            write(info[i][1], message, sizeof(message));
        }
    }
    
    while (true)
    {
        // epoll_ctl(epoll_File_Descriptor, EPOLL_CTL_DEL, info[2][0], &e_info[2]);

        events_Ready = epoll_wait(epoll_File_Descriptor, event_Queue, MAX_EPOLL_EVENTS, -1);

        for(int i = 0; i < events_Ready; i++){
            printf("Got an event %d for fd %d!\n", event_Queue[i].events, event_Queue[i].data.fd);

            // read(event_Queue[i].data.fd, receive_Buffer, sizeof(receive_Buffer));

            printf("Received the message: %s\n", receive_Buffer);
        }
    }
    
    return 0;
} 