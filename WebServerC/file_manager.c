//
//  file_manager.c
//  server
//
//  Created by Alain Cruz Casanova on 6/2/17.
//  Copyright © 2017 AlainCruz. All rights reserved.
//

#include "file_manager.h"

// Constant strings, representing the codes' names.
char * const code_200 = "OK";
char * const code_204 = "No Content";
char * const code_404 = "Not Found";
char * const code_500 = "Server Error";
// Semaphore name.
char * const sem_name = "file";
// Our resources directory name.
char * const resources_dir = "/resources";

struct stat st = {0};

// Initialized the semaphore.
int init_file_semaphore(){
    sem_file = sem_open(sem_name, O_CREAT, 0777, 1);
    if (sem_file == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore file");
        return 0;
    }
    return 1;
}

// Close the semaphore.
void close_semaphore() {
    sem_unlink(sem_name);
}

// Method that returns the file status and it's content.
struct file_status get_status(char* path, char* host, char* content_type) {
    
    // Get the current working directory.
    char cwd[1024];
    getcwd(cwd,sizeof(cwd));
    
    // Check that the directory exists, else simply return a server error.
    DIR* srcdir = opendir(cwd);
    if(srcdir == NULL) {
        
        // Return if the directory either doesn't exists or fails to open.
        struct file_status stats;
        stats.code = 501;
        stats.size = 0;
        stats.code_name = code_500;
        stats.file_bytes = "";
        return stats;
    }
    struct dirent *dir;
    
    // Try to read from our directory..
    if((dir = readdir(srcdir)) != NULL) {
        
        // Fetch the current directory name and save it in a variable.
        char *dir_name = dir->d_name;
        char file_name[1024] = "";
        
        // Concatenate the file path to the current directory.
        strcat(file_name, dir_name);
        strcat(file_name, resources_dir);
        strcat(file_name, path);
        strcat(file_name, "\0");
        
        // Open the file with read permission.
        // Using "rb" since opening non-text files can cause unappropiate translations otherwise.
        FILE *file = fopen(file_name, "rb");
        if (file == NULL) {
            
            // Should the file fails to open or doesn't exists.
            perror(file_name);
            struct file_status stats;
            stats.code = 404;
            stats.size = 0;
            stats.code_name = code_404;
            stats.file_bytes = "";
            
            // We always need to make sure we are closing all files and directories we open.
            closedir(srcdir);
            return stats;
        } else {
            
            // We check that the file isn't empty.
            fseek(file, 0, SEEK_END); //Go to the end of the file.
            size_t size = ftell(file); //Get the file's size, by calculating the current position in the file.
            rewind(file); //We go back to the beginning of the file.
            
            if (size > 0) {
                
                //Should it has content, we save the file information in a string.
                char * result_bytes = (char *)malloc((size + 1) * sizeof(char));
                
                struct file_status stats;
                
                fread(result_bytes, size, 1, file);
                // It is always important to add '\0' and all malloc assingend strings.
                // '\0' represents the end of the string. Ommiting this will cause issues.
                result_bytes[size] = '\0';
                
                stats.code = 200;
                stats.size = (int)size;
                stats.code_name = code_200;
                stats.file_bytes = result_bytes;
                stat(file_name, &stats.attrib);
                
                // Make sure we are closing all files and directories we opened.
                fclose(file);
                closedir(srcdir);
                return stats;
            } else {
                
                
                struct file_status stats;
                stats.code = 204;
                stats.size = 0;
                stats.code_name = code_204;
                stats.file_bytes = "";
                stat(file_name, &stats.attrib);
                
                //We always need to make sure we are closing all files and directories we open.
                fclose(file);
                closedir(srcdir);
                return stats;
            }
        }
    }
    //In case we can't read from the directory.
    closedir(srcdir);
    struct file_status stats;
    stats.code = 500;
    stats.size = 0;
    stats.code_name = code_500;
    stats.file_bytes = "";
    return stats;
}

// Writes log
void write_log(char* log_info, int type) {
    char cwd[1024];
    //We get the current working directory.
    getcwd(cwd,sizeof(cwd));
    //We open and check that the directory exists.
    DIR* srcdir = opendir(cwd);
    if(srcdir == NULL) {
        //Return if the directory either doesn't exists or fails to open.
        printf("Can't open current working directory.");
    }
    struct dirent *dir;
    
    // We read from our directory.
    if((dir = readdir(srcdir)) != NULL) {
        char *dir_name = dir->d_name;
        char log_dir_name[1024] = "";
        char file_name[1024] = "";
        
        strcat(log_dir_name, dir_name);
        strcat(log_dir_name, "/logs/");
        strcat(log_dir_name, "\0");
        
        // Make directory if it doesn't exist where we will save our log.
        if (stat(log_dir_name, &st) == -1) {
            mkdir(log_dir_name, 0700);
        }
        
        //Concatenate the file path to the current directory.
        strcat(file_name, dir_name);
        strcat(file_name, "/logs/server.log");
        strcat(file_name, "\0");
        //Avoid race conditions.
        sem_wait(sem_file);
        FILE *file = fopen(file_name, "ab+");
        if (file == NULL) {
            printf("Can't open server log.\n");
        } else {
            fprintf(file, "%s\n", log_info);
            fclose(file);
        }
    }
    closedir(srcdir);
    //Release lock,
    sem_post(sem_file);
}

//Method to print the file status.
//Only use for testing.
void print_file_status(struct file_status stats){
    printf("%d %s | Size: %d\nCode: %s\n",stats.code,stats.code_name,stats.size,stats.file_bytes);
}

