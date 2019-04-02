//
//  ip_management.h
//  server
//
//  Created by Alain Cruz Casanova on 6/1/17.
//  Copyright © 2017 AlainCruz. All rights reserved.
//

#ifndef ip_management_h
#define ip_management_h

#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include <unistd.h> //sleep

//Attr
static sem_t *sem_ip;
static char* sem_ip_name;
static const int array_amount = 300;
static int excludedIP[array_amount];
static int max_number;
static int banned_time;

//Methods
void exclude_ip(int ip);
int check_ip(int ip);
void *set_timeout(void* param);
int init_ip_management(int timeout);
void destroy_ip_management(void);
int get_position(int ip);


#endif /* ip_management_h */
