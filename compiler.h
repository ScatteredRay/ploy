#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "symbol.h"

struct compiler;
struct compiled;

typedef void* pointer;

compiler* init_compiler(symbol_table* table);
void destroy_compiler(compiler* compile);
compiled* compiler_compile_expression(compiler* compile, pointer P);

#endif //_COMPILER_H_
