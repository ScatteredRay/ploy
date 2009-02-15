#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "typeinfo.h"
#include "types.h"
#include "symbol.h"
#include <assert.h>
#include <sstream>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>

// internal types classes.
void set_car(pointer P, pointer V);
void set_cdr(pointer P, pointer V);
void* get_value(pointer P);
pointer ploy_alloc(const dynamic_type* type, size_t size);
pointer ploy_alloc(const dynamic_type* type);
void ploy_free(pointer P);

class typeinfo
{
public:
	virtual const llvm::Type* get_llvm_type() = 0;
	virtual void print_description(FILE* out) = 0;
	virtual ~typeinfo()
	{}
};

class primitive_typeinfo : typeinfo
{
	const llvm::Type* llvm_type;
public:
	primitive_typeinfo(const llvm::Type* typ) : llvm_type(typ)
	{}
	virtual const llvm::Type* get_llvm_type()
	{
		return llvm_type;
	}
	virtual void print_description(FILE* out)
	{
		std::ostringstream desc;
		llvm_type->print(desc);
		fputs(desc.str().c_str(), out);
	}
};

typeinfo* get_typeinfo(pointer P)
{
	assert(is_type(P, DT_TypeInfo));
	return (typeinfo*)get_value(P);
}

void typeinfo_finish(pointer P)
{
	// Call Destructor
	get_typeinfo(P)->~typeinfo();
}

void print_typeinfo(pointer P, symbol_table* tbl, FILE* out)
{
	fputs("#Type:", out);
	get_typeinfo(P)->print_description(out);
}

const size_t type_info_size = sizeof(typeinfo);

const llvm::Type* typeinfo_get_llvm_type(pointer type)
{
	return get_typeinfo(type)->get_llvm_type();
}

pointer make_primitive_typeinfo(symbol sym, symbol_table* tbl)
{
	pointer P = ploy_alloc(get_type(DT_TypeInfo), sizeof(primitive_typeinfo));
	typeinfo* T = get_typeinfo(P);
	const llvm::Type* llvm_T;
	
	if(sym == symbol_from_string(tbl, "int"))
		llvm_T = llvm::Type::Int32Ty;
	else if(sym == symbol_from_string(tbl, "float"))
	    llvm_T = llvm::Type::FloatTy;
	else
		assert(false);
	
	new(T) primitive_typeinfo(llvm_T);
	return P;
}

pointer eval_typeinfo(pointer P, symbol_table* tbl)
{
	if(!is_type(P, DT_Pair))
	{
		// Either an alias or a primitive.
		assert(is_type(P, DT_Symbol));
		return make_primitive_typeinfo(*get_symbol(P), tbl);
	}
	else if(pair_cdr(P) == NIL)
	{
		// Try to collapse
		return eval_typeinfo(pair_car(P), tbl);
	}
	else
		assert(false);
}

void transform_tree_gen_typeinfo(pointer P, symbol_table* tbl)
{
	if(P != NIL)
	{
		if(is_type(P, DT_Pair))
		{
			if(is_type(pair_car(P), DT_Symbol))
			{
				if(*get_symbol(pair_car(P)) == symbol_from_string(tbl, ":"))
				{
					set_car(P, eval_typeinfo(cadr(P), tbl));
					set_cdr(P, cddr(P));
				}
				else if(string_from_symbol(tbl, *get_symbol(pair_car(P)))[0] == ':')
				{
					pointer T = create_symbol(tbl, string_from_symbol(tbl, *get_symbol(pair_car(P)))+1);
					set_car(P, eval_typeinfo(T, tbl));
					destroy_list(T);
				}
			}
			else
			{
				transform_tree_gen_typeinfo(pair_car(P), tbl);
			}

			transform_tree_gen_typeinfo(pair_cdr(P), tbl);
		}
	}
}
