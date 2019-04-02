//
//  response_manager.h
//  server
//
//  Created by Alain Cruz Casanova on 6/2/17.
//  Copyright © 2017 AlainCruz. All rights reserved.
//

#ifndef response_manager_h
#define response_manager_h

#include <stdio.h>
#include <regex.h> //regex
#include <string.h> //strstr
#include <stdlib.h>

//Methods
int check_response(char* request);
char* get_request(char *message);
char* get_host(char* message);
char* get_type(char* request);

#endif /* response_manager_h */
