#include "types.h"
#include "symbol.h"
#include <assert.h>


//TODO: allow compile time code transformation by appending source data (strings, and line info to the type data.

//TODO: Dynamic build this too
// It is essential that this is orderd in the same way the enums are numbered.
static const dynamic_type TypeList[DT_Invalid] =
{
	{DT_Pair, sizeof(pair)},
	{DT_Symbol, sizeof(symbol)},
	{DT_Int, sizeof(int)},
	{DT_Real, sizeof(float)},
	{DT_String, invalid_size},
	{DT_Char, sizeof(char)}
};

const dynamic_type* get_type(dynamic_types typeId)
{
	assert(typeId < DT_Invalid);
	return &TypeList[typeId];
}

pointer ploy_alloc(const dynamic_type* type, size_t size)
{
	assert(size != invalid_size || type->AllocSize != invalid_size);

	size_t alloc_size = size;
	if(alloc_size == invalid_size)
		alloc_size = type->AllocSize;

	alloc_size += sizeof(dynamic_types);

	void* alloc = malloc(alloc_size);
	memset(alloc, 0, alloc_size); // Remove if we implement type initializers
	*((dynamic_types*)alloc) = type->Id;

	return (pointer)alloc;
	
}

pointer ploy_alloc(const dynamic_type* type)
{
	return ploy_alloc(type, invalid_size);
}

void ploy_free(pointer P)
{
	free(P);
}

dynamic_types get_type_id(pointer P)
{
	return *(dynamic_types*)P;
}

bool is_type(pointer P, dynamic_types type)
{
	return get_type_id(P) == type;
}

void* get_value(pointer P)
{
	return (char*)P+sizeof(dynamic_types);
}

pair* get_pair(pointer P)
{
	assert(is_type(P, DT_Pair));
	return (pair*)get_value(P);
}

pointer create_pair(pointer car, pointer cdr)
{
	pointer ret = ploy_alloc(get_type(DT_Pair));
	
	pair* value = get_pair(ret);
	
	value->_car = car;
	value->_cdr = cdr;

	return ret;
}

pointer pair_car(pointer P)
{
	assert(is_type(P, DT_Pair));
	return get_pair(P)->_car;
}

pointer pair_cdr(pointer P)
{
	assert(is_type(P, DT_Pair));
	return get_pair(P)->_cdr;
}

symbol* get_symbol(pointer P)
{
	assert(is_type(P, DT_Symbol));
	return (symbol*)get_value(P);
}

pointer create_symbol(symbol_table* tbl, const char* sym)
{
	pointer ret = ploy_alloc(get_type(DT_Symbol));

	*get_symbol(ret) = symbol_from_string(tbl, sym);

	return ret;
}

pointer create_symbol(symbol_table* tbl, const char* sym, size_t len)
{
	pointer ret = ploy_alloc(get_type(DT_Symbol));

	*get_symbol(ret) = symbol_from_string(tbl, sym, len);

	return ret;
}

int* get_int_ref(pointer P)
{
	return (int*)get_value(P);
}

int get_int(pointer P)
{
	
	return *get_int_ref(P);
}

pointer create_int(int i)
{
	pointer ret = ploy_alloc(get_type(DT_Int));
	
	*get_int_ref(ret) = i;
	
	return ret;
}

float* get_real_ref(pointer P)
{
	assert(is_type(P, DT_Real));
	return (float*)get_value(P);
}

float get_real(pointer P)
{
	return *get_real_ref(P);
}

pointer create_real(float f)
{
	pointer ret = ploy_alloc(get_type(DT_Real));
	
	*get_real_ref(ret) = f;

	return ret;
}

char* get_char_ref(pointer P)
{
	assert(is_type(P, DT_Char));
	return (char*)get_value(P);
}

char get_char(pointer P)
{
	return *get_char_ref(P);
}

pointer create_char(char c)
{
	pointer ret = ploy_alloc(get_type(DT_Char));
	
	*get_char_ref(ret) = c;

	return ret;
}

char* get_string_ref(pointer P)
{
	assert(is_type(P, DT_String));
	return (char*)get_value(P);
}

const char* get_string(pointer P)
{
	return get_string_ref(P);
}

pointer alloc_string(size_t len)
{
	pointer ret = ploy_alloc(get_type(DT_String), len+1);
	(get_string_ref(ret))[0] = '\0'; // Unneeded if everywhere initializes it directlly.
	return ret;
}

pointer create_string(const char* str)
{
	return create_string(str, strlen(str));
}

pointer create_string(const char* str, size_t len)
{
	pointer ret = ploy_alloc(get_type(DT_String), len+1);
	
	strncpy(get_string_ref(ret), str, len);
	
	(get_string_ref(ret))[len+1] = '\0'; // not strictly nessacary while we are zeroing the mem, but for if/when we remove it.
	
	return ret;
}

void destroy_list(pointer P)
{
	if(is_type(P, DT_Pair))
	{
		if(pair_car(P) != NIL)
			destroy_list(pair_car(P));
		if(pair_cdr(P) != NIL)
			destroy_list(pair_cdr(P));
	}
	ploy_free(P);
}
