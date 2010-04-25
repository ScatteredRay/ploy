// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#include "function.h"

compiler_type* create_compiler_type(dynamic_types type_ID)
{
    compiler_type* type = new compiler_type();
    type->ID = type_ID;
    type->combined_type = NULL;
    return type;
}

compiler_type* duplicate_compiler_type(compiler_type* src)
{
    if(!src)
        return NULL;

    compiler_type* type = create_compiler_type(src->ID);
    type->combined_type = duplicate_compiler_type(src->combined_type);
}

void destroy_compiler_type(compiler_type* type)
{
    if(type->combined_type)
        destroy_compiler_type(type->combined_type);
    delete type;
}


compiler_variable* create_compiler_variable_own_type(compiler_type* type, symbol name)
{
    compiler_variable* var = new compiler_variable();
    var->type = type;
    var->name = name;
    return var;
}

compiler_variable* create_compiler_variable(compiler_type* type, symbol name)
{
    return create_compiler_variable_own_type(duplicate_compiler_type(type), name);
}

void destroy_compiler_variable(compiler_variable* var)
{
    destroy_compiler_type(var->type);
    delete var;
}
