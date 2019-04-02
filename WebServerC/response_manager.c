//
//  response_manager.c
//  server
//
//  Created by Alain Cruz Casanova on 6/2/17.
//  Copyright © 2017 AlainCruz. All rights reserved.
//

#include "response_manager.h"

// We check that the client's request complies with our server's requirements.
int check_response(char* message){
    
    regex_t regex;
    int result;
    char errorbuf[150];
    
    //Posible warning: "unknown scape sequence".
    //We must add double backslashes.
    //Pattern we shall use in our regex.
    char const *PATTERN = "GET.*HTTP\\/1\\.[0-1].*";
    result = regcomp(&regex, PATTERN, 0); //Compiles the regex.
    if (result) {
        printf("Could not compile regex.\n");
        return 0;
    }
    
    //Execute regex
    result = regexec(&regex, message, 0, NULL, 0);
    if (!result) {
        //If result complies with the reqs, we return 1.
        return 1;
    } else if (result == REG_NOMATCH) {
        //Result doesn't complies with the reqs.
        puts("Request doesn't not complies with the requirements.");
        return 0;
    } else {
        //Error executing the regex.
        regerror(result, &regex, errorbuf, sizeof(errorbuf));
        printf("Regex failed. Error -> %s\n",errorbuf);
        return 0;
    }
    return 0;
}

// Method that returns our client's request.
char* get_request(char* message){
    
    // Patterns that we are looking for.
    char const *PATTERN1 = "GET ";
    char *PATTERN2 = " HTTP/1";
    char *start_message;
    char *end_message;
    char *result = NULL;
    
    //Find the first ocurrence of the pattern in the desired message.
    start_message = strstr(message, PATTERN1);
    if (start_message != NULL) {
        
        //If the pattern was found.
        //Move the char pointer after the pattern occurence.
        start_message += strlen(PATTERN1);
        
        //Find the first occurrence of the second pattern, discarding the first pattern occurence.
        end_message = strstr(start_message, PATTERN2);
        if (end_message != NULL) {
            
            // We calculate the request size, in order to allocate memory.
            unsigned long size = (strlen(start_message)-strlen(end_message));
            result = (char *)malloc(end_message-start_message);
            
            // Copy the value, from one varaible to another.
            memcpy(result, start_message, end_message-start_message);
            
            // IMPORTANT: Always finish strings with '\0'.
            result[size] = '\0';
            return result;
        }
    }
    
    // Safe return.
    return "";
}

// Same method as before, but this time, we are looking for the host.
char* get_host(char* message){
    
    // Patterns that we are looking for.
    char const *PATTERN1 = "Host: ";
    char const *PATTERN2 = "\r";
    char *start_message;
    char *end_message;
    char *host = NULL;
    
    // Find the first ocurrence of the pattern in the desired message.
    start_message = strstr(message,PATTERN1);
    if (start_message!=NULL) {
        
        //If the pattern was found.
        //Move the char pointer after the pattern occurence.
        start_message += strlen(PATTERN1);
        
        //Find the first occurrence of the second pattern, discarding the first pattern occurence.
        end_message = strstr(start_message, PATTERN2);
        if (end_message != NULL) {
            
            //We calculate the request size, in order to allocate memory.
            unsigned long size = (strlen(start_message)-strlen(end_message));
            host = (char *)malloc(end_message-start_message+1);
            
            //Copy the value, from one varaible to another.
            memcpy(host, start_message, end_message-start_message);
            
            //IMPORTANT: Always finish strings with '\0'.
            host[size] = '\0';
            return host;
        }
    }
    
    // Safe return.
    return "";
}

// Similar method to the last two, but given a known request, we returns it's type.
char* get_type(char* request) {
    
    // All requests come with an extension. We need to find the occurence of the dot.
    char const *PATTERN1 = ".";
    char *start_message;
    char *result;
    
    // Find the first occurence of the dot.
    start_message = strstr(request, PATTERN1);
    if (start_message!=NULL) {
        
        // We move our pointer one byte to the right, so that the dot is discarded.
        result = start_message + 1;
        result[strlen(result)] = '\0';
        return result;
    }
    
    //Safe return.
    return "";
}
