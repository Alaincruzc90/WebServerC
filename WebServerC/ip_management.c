//
//  ip_management.c
//  server
//
//  Created by Alain Cruz Casanova on 6/1/17.
//  Copyright © 2017 AlainCruz. All rights reserved.
//

#include "ip_management.h"

//We init our ip_management.
int init_ip_management(int timeout){
    //We need to create our named semaphore;
    sem_ip_name = "ip";
    sem_ip = sem_open(sem_ip_name, O_CREAT, 0777, 1);
    sem_post(sem_ip);
    //Set timeout duration.
    banned_time = timeout;
    if (sem_ip == SEM_FAILED)
    {
        //Should the initialization fails, we return 0.
        return 0;
    }
    //We will use max_number, to avoid looping through all the array.
    //We will reach our maximun number and then stop.
    max_number = 0;
    int i;
    for (i = 0; i<array_amount; i++) {
        //Init our excluded ips' array.
        excludedIP[i] = 0;
    }
    return 1;
}

//Destructor.
void destroy_ip_management(){
    //We close our semaphore.
    sem_unlink(sem_ip_name);
}

//Check if a given ip was excluded.
int check_ip(int ip){
    int i = 0;
    for (i=0;i<=max_number;i++){
        if (excludedIP[i]==ip) {
            return 1;
        }
    }
    return 0;
}

//Add an ip to our excluded list.
void exclude_ip(int ip){
    int i = 0;
    //Look for the next empty value.
    while (excludedIP[i] != 0) {
        i++;
    }
    //We need to avoid possible race conditions' issues.
    sem_wait(sem_ip);
    excludedIP[i] = ip;
    //Set the new max_number.
    if(i>=max_number) {
        max_number = i+1;
    }
    sem_post(sem_ip);
    //Creates a new thred and set the ip in timeout.
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, set_timeout, (void*)excludedIP+i);
}

//Method that sets an ip in timeout.
void *set_timeout(void *param){
    //We get the location of the ip in our array.
    int n = (*(int*) param);
    int i = get_position(n);
    printf("Ip excluded -> %d\n",n);
    //We set this thread in a NON RUNNABLE state for a given duration.
    sleep(banned_time);
    //We need to avoid race conditions issues.
    sem_wait(sem_ip);
    printf("Ip released -> %d\n",excludedIP[i]);
    excludedIP[i] = 0;
    //If the ip's locations was bigger than our max_number, we need to find the smallets empty space.
    if(i>=max_number) {
        int n = i;
        while(excludedIP[n] == 0 || n<=0){
            n--;
        }
        max_number = n;
    }
    //We let other threads work with our array and max_number.
    sem_post(sem_ip);
    pthread_exit(NULL);
}

//Find an ip's position in the array.
int get_position(int ip){
    int n = 0;
    for (n = 0; n<=max_number; n++) {
        if (excludedIP[n] == ip) {
            return n;
        }
    }
    return 0;
}
