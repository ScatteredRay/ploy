// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "types.h"

struct compiler_type
{
    dynamic_types ID;
    compiler_type* combined_type;
};

struct compiler_variable
{
    compiler_type* type;
    symbol name;
};

struct var_collection
{
    compiler_variable var;
    var_collection* next;
};

struct function
{
    compiler_type return_type;
    var_collection* params;
};

#endif _FUNCTION_H_
