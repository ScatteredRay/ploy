// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _TYPES_H_
#define _TYPES_H_
#include <cstring>
#include "symbol.h"

#define CCALL extern "C"

const size_t invalid_size = -1;

typedef void* pointer;
const pointer NIL = NULL;

//TODO: Dynamic build this

enum dynamic_types
{
    DT_Pair = 0,
    DT_Symbol = 1,
    DT_Int = 2,
    DT_Real = 3,
    DT_String = 4,
    DT_Char = 5,
    DT_TypeInfo = 6,
    DT_Invalid = 7,
    DT_Any = 8,
    DT_Static_Flag = 0x80000000
};

struct dynamic_type
{
    dynamic_types Id;
    size_t AllocSize;
    void (*finish)(pointer P);
};

struct pair
{
    pointer _car;
    pointer _cdr;
};

CCALL const dynamic_type* get_type(dynamic_types typeId);

struct symbol_table;

CCALL pointer create_pair(pointer car, pointer cdr);
pointer create_symbol(symbol_table* tbl, const char* sym);
CCALL pointer create_symbol(symbol_table* tbl, const char* sym, size_t len);
CCALL pointer create_int(int i);
CCALL pointer create_real(float f);
CCALL pointer create_char(char c);
pointer create_string(const char* str);
CCALL pointer create_string(const char* str, size_t len);

CCALL pointer alloc_string(size_t len);

CCALL bool is_type(pointer P, dynamic_types type);
CCALL dynamic_types get_type_id(pointer P);

CCALL void destroy_list(pointer P);

CCALL pointer pair_car(pointer P);
CCALL pointer pair_cdr(pointer P);

CCALL symbol* get_symbol(pointer P);
CCALL int get_int(pointer P);
CCALL float get_real(pointer P);
CCALL char get_char(pointer P);
CCALL char* get_string_ref(pointer P);
CCALL const char* get_string(pointer P);

CCALL void print_object(pointer P, symbol_table* table);

#define car(p) pair_car(p)
#define cdr(p) pair_cdr(p)

#define caar(p)          car(car(p))
#define cadr(p)          car(cdr(p))
#define cdar(p)          cdr(car(p))
#define cddr(p)          cdr(cdr(p))
#define cadar(p)         car(cdr(car(p)))
#define caddr(p)         car(cdr(cdr(p)))
#define cdddr(p)         cdr(cdr(cdr(p)))
#define cadaar(p)        car(cdr(car(car(p))))
#define cadddr(p)        car(cdr(cdr(cdr(p))))
#define cddddr(p)        cdr(cdr(cdr(cdr(p))))


#endif //_TYPES_H_
