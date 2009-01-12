#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#include <stdio.h>

struct environment
{
	FILE* out;
};

environment* create_environment();
void destroy_environment(environment* env);

#endif //_ENVIRONMENT_H_
