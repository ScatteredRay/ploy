// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
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
