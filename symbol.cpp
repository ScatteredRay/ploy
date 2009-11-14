#include "symbol.h"
#include "types.h"
#include <cstdlib>
#include <assert.h>

// initial implementation is a b-tree consider a hashtable for faster 
// symbol->string yet slower resize if we can prevent a resize often, or ever.

struct symbol_leaf
{
};

struct symbol_node
{
	char C;
	symbol_node* Next;
	symbol_node* Child;
	int symbol_id;
};

struct symbol_table
{
	symbol_node* Root;
	char** Symbols;
	size_t NumSymbols;
	size_t MaxSymbols;
};

symbol_node* create_symbol_node(char C)
{
	symbol_node* node = new symbol_node();
	node->C = C;
	node->Next = NULL;
	node->Child = NULL;
	node->symbol_id = invalid_symbol;

    return node;
}

// does not remove the symbol from the id table.
void destroy_symbol_node(symbol_node* node)
{
	if(node->Next)
		destroy_symbol_node(node->Next);
	if(node->Child)
		destroy_symbol_node(node->Child);
	delete node;
}

symbol_table* init_symbol_table()
{
	symbol_table* tbl = new symbol_table();
	tbl->Root = create_symbol_node('a'); // search implementation assumes always a node.
	tbl->Symbols = (char**)malloc(sizeof(char*) * initial_symbol_table_size);
	tbl->NumSymbols = 0;
	tbl->MaxSymbols = initial_symbol_table_size;
}

void destroy_symbol_table(symbol_table* tbl)
{
	if(tbl->Root)
		destroy_symbol_node(tbl->Root);

	if(tbl->Symbols)
	{
		for(int i=0; i < tbl->NumSymbols; i++)
		{
			delete tbl->Symbols[i];
		}
		free(tbl->Symbols);
	}
	
	delete tbl;
	
}

void resize_symbol_table(symbol_table* tbl, size_t additional_needed)
{
	size_t NewMax = tbl->MaxSymbols * 2;
	if(NewMax < tbl->MaxSymbols + additional_needed)
		NewMax = tbl->MaxSymbols + additional_needed;
	
	tbl->Symbols = (char**)realloc(tbl->Symbols, sizeof(char*) * NewMax);
	tbl->MaxSymbols = NewMax;
	
}

int append_string_to_symbol_table(symbol_table* tbl, const char* string)
{
	if(tbl->MaxSymbols <= tbl->NumSymbols)
	{
		resize_symbol_table(tbl, 1);
	}
	
	tbl->Symbols[tbl->NumSymbols] = (char*)malloc(sizeof(char) * (strlen(string) + 1));
	strcpy(tbl->Symbols[tbl->NumSymbols], string);
	
	return tbl->NumSymbols++;
}

char char_to_lower(char C)
{
	if(C >= 'A' && C <= 'Z')
	{
		C += 'a' - 'A';
	}
	return C;
}

const char* string_from_symbol(symbol_table* tbl, symbol sym)
{
	return tbl->Symbols[sym.Id];
}

symbol symbol_from_string(symbol_table* tbl, const char* string, size_t len)
{
	//TODO: remove memcpy/alloc
	char* string_buf = new char[len+1];
	
	strncpy(string_buf, string, len);
	
	string_buf[len] = '\0';

	symbol ret = symbol_from_string(tbl, string_buf);

	delete [] string_buf;

	return ret;
}

symbol symbol_from_string(symbol_table* tbl, const char* string)
{
	symbol_node* Curr = tbl->Root;
	symbol_node** Parent = &tbl->Root;
	const char* CurrC = string;

	int Id = invalid_symbol;
	
	while(true)
	{
		char C = char_to_lower(*CurrC);
		
		assert(C != '\0');
		
		if(Curr == NULL)
		{
			// need to add a node
			assert(Parent);
			*Parent = create_symbol_node(C);
			Curr = *Parent;
			
		}
		else if(C < Curr->C) // we can assume an else, not implicit.
		{
			// need to add a node before current one.
			Curr = create_symbol_node(C);
			Curr->Next = *Parent;
			*Parent = Curr;
		} 
		
		if(C == Curr->C)
		{
			CurrC++;
			if(*CurrC == '\0')
			{
				// Found Node!
				if(Curr->symbol_id != invalid_symbol)
				{
					Id = Curr->symbol_id;
					break;
				}
				else
				{
					Id = append_string_to_symbol_table(tbl, string);
					Curr->symbol_id = Id;
					break;
				}
			}
			else
			{
				Parent = &Curr->Child;
				Curr = Curr->Child;
			}
			
		}
		else
		{
			Parent = &Curr->Next;
			Curr = Curr->Next;
		}
		
	}

	return symbol(Id);
}

