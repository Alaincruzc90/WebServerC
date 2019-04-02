//
//  file_manager.h
//  server
//
//  Created by Alain Cruz Casanova on 6/2/17.
//  Copyright © 2017 AlainCruz. All rights reserved.
//

#ifndef file_manager_h
#define file_manager_h


#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <dirent.h> //DIR
#include <unistd.h> //malloc
#include <string.h> //strcat
#include <stdlib.h>
#include <sys/types.h>
#include <semaphore.h> //semaphore
#include <sys/stat.h> //get file last modification date

//Struct where we will save our file's status
struct file_status {
    int code;
    int size;
    char *code_name;
    char *file_bytes;
    struct stat attrib;
};

//Semaphore to prevent overwritting server.log
static sem_t *sem_file;

//Methods
struct file_status get_status(char *path, char *host, char* content_type);
void write_log(char* log_info, int type);
void print_file_status(struct file_status stats);
int init_file_semaphore(void);
void close_semaphore(void);

#endif /* file_manager_h */
