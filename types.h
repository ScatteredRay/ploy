#ifndef _TYPES_H_
#define _TYPES_H_
#include <cstring>
#include "symbol.h"

#define invalid_size -1

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
	DT_Invalid = 6,
	DT_Any = 7
};

struct dynamic_type
{
	dynamic_types Id;
	size_t AllocSize;
};

struct pair
{
	pointer _car;
	pointer _cdr;
};


const dynamic_type* get_type(dynamic_types typeId);

struct symbol_table;

pointer create_pair(pointer car, pointer cdr);
pointer create_symbol(symbol_table* tbl, const char* sym);
pointer create_symbol(symbol_table* tbl, const char* sym, size_t len);
pointer create_int(int i);
pointer create_real(float f);
pointer create_char(char c);
pointer create_string(const char* str);
pointer create_string(const char* str, size_t len);

bool is_type(pointer P, dynamic_types type);
dynamic_types get_type_id(pointer P);

void destroy_list(pointer P);

pointer pair_car(pointer P);
pointer pair_cdr(pointer P);

symbol* get_symbol(pointer P);
int get_int(pointer P);
float get_real(pointer P);
char get_char(pointer P);
char* get_string_ref(pointer P);
const char* get_string(pointer P);
pointer alloc_string(size_t len);

void print_object(pointer P, symbol_table* table);

#define car(p) pair_car(p)
#define cdr(p) pair_cdr(p)

#define caar(p)          car(car(p))
#define cadr(p)          car(cdr(p))
#define cdar(p)          cdr(car(p))
#define cddr(p)          cdr(cdr(p))
#define cadar(p)         car(cdr(car(p)))
#define caddr(p)         car(cdr(cdr(p)))
#define cadaar(p)        car(cdr(car(car(p))))
#define cadddr(p)        car(cdr(cdr(cdr(p))))
#define cddddr(p)        cdr(cdr(cdr(cdr(p))))
	

#endif //_TYPES_H_
