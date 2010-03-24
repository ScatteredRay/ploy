// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "symbol.h"

struct compiler;
struct compile_block;

typedef void* pointer;

compiler* init_compiler(symbol_table* table);
void destroy_compiler(compiler* compile);
void compiler_compile_expression(compiler* compile, pointer P);
void compiler_print_module(compiler* compile);
void compiler_write_asm_file(compiler* compile, const char* output_filename);

#endif //_COMPILER_H_
