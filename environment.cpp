// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#include "environment.h"

environment* create_environment()
{
	environment* env = new environment();
	env->out = stdout;
};

void destroy_environment(environment* env)
{
	delete env;
}
