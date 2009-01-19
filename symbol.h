#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include <cstring>

struct symbol_table;

const int invalid_symbol = -1;
const char initial_symbol_table_size = 20;

struct symbol
{
	int Id;
	symbol(int i) : Id(i)
	{}
	symbol() : Id(invalid_symbol)
	{}

	bool operator==(const symbol& Othr)
	{
		return Id == Othr.Id;
	}
};

symbol_table* init_symbol_table();
void destroy_symbol_table(symbol_table* tbl);
symbol symbol_from_string(symbol_table* tbl, const char* string);
symbol symbol_from_string(symbol_table* tbl, const char* string, size_t len);
const char* string_from_symbol(symbol_table* tbl, symbol sym);

#endif //_SYMBOL_H_

