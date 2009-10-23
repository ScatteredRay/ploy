#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "typeinfo.h"
#include "types.h"
#include "symbol.h"
#include <assert.h>
#include <sstream>
#include <llvm/Type.h>
#include <llvm/LLVMContext.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Support/raw_os_ostream.h>

// internal types classes.
void set_car(pointer P, pointer V);
void set_cdr(pointer P, pointer V);
void* get_value(pointer P);
pointer ploy_alloc(const dynamic_type* type, size_t size);
pointer ploy_alloc(const dynamic_type* type);
pointer ploy_static_alloc(const dynamic_type* type, size_t size);
pointer ploy_static_alloc(const dynamic_type* type);
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
		std::ostringstream std_desc;
        llvm::raw_os_ostream desc(std_desc);
		llvm_type->print(desc);
		fputs(std_desc.str().c_str(), out);
	}
};

class struct_typeinfo : primitive_typeinfo
{
	std::vector<const char*> Members; // References to symbol strings, don't need to manage the memory.
public:
	struct_typeinfo(std::vector<const char*>& Params, const llvm::Type* typ) : primitive_typeinfo(typ), Members(Params)
	{}

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

pointer make_primitive_typeinfo(const llvm::Type* llvm_T)
{
	pointer P = ploy_alloc(get_type(DT_TypeInfo), sizeof(primitive_typeinfo));
	typeinfo* T = get_typeinfo(P);
		
	new(T) primitive_typeinfo(llvm_T);
	return P;
}


pointer get_int_typeinfo(llvm::LLVMContext& Context)
{
	static pointer P = NULL;

	if(!P)
	{
		P = ploy_static_alloc(get_type(DT_TypeInfo), sizeof(primitive_typeinfo));
		typeinfo* T = get_typeinfo(P);
		
		new(T) primitive_typeinfo(llvm::Type::getInt32Ty(Context));
	}

	return P;
}

pointer get_float_typeinfo(llvm::LLVMContext& Context)
{
	static pointer P = NULL;

	if(!P)
	{
		P = ploy_static_alloc(get_type(DT_TypeInfo), sizeof(primitive_typeinfo));
		typeinfo* T = get_typeinfo(P);
		
		new(T) primitive_typeinfo(llvm::Type::getFloatTy(Context));
	}

	return P;
}

pointer make_struct_typeinfo(llvm::LLVMContext& Context, std::vector<const char*>& ParamNames, std::vector<const llvm::Type*> ParamTypes)
{
	pointer P = ploy_alloc(get_type(DT_TypeInfo), sizeof(struct_typeinfo));
	typeinfo* T = get_typeinfo(P);
	
	new(T) struct_typeinfo(ParamNames, llvm::StructType::get(Context, ParamTypes, false));
	return P;
}

pointer eval_typeinfo(pointer P, symbol_table* tbl, type_map* type_define_map)
{
	if(!is_type(P, DT_Pair))
	{
		// Either an alias or a primitive.
		assert(is_type(P, DT_Symbol));
		symbol S = *get_symbol(P);
		std::map<symbol, pointer>::iterator it = type_define_map->find(S);
		if(it != type_define_map->end())
			return it->second;

		const llvm::Type* llvm_T;
		if(S == symbol_from_string(tbl, "int"))
			llvm_T = llvm::Type::getInt32Ty(llvm::getGlobalContext());
		else if(S == symbol_from_string(tbl, "float"))
			llvm_T = llvm::Type::getFloatTy(llvm::getGlobalContext());
		else
			assert(false);
		return make_primitive_typeinfo(llvm_T);
	}
	else if(is_type(pair_car(P), DT_Pair))
	{
		// Try to collapse
		return eval_typeinfo(pair_car(P), tbl, type_define_map);
	}
	else if(*get_symbol(car(P)) == symbol_from_string(tbl, "int"))
	{
		//TODO: merge with other int code.
		int bitlen = 32;
		if(is_type(cdr(P), DT_Pair) &&
		   is_type(cadr(P), DT_Int))
			bitlen = get_int(cadr(P));

		return make_primitive_typeinfo(llvm::IntegerType::get(llvm::getGlobalContext(), bitlen));
	}
	// Composite types don't mantain proper non-llvm typeinfo of the members, as they just store members in the llvm type, FIXME.
	else if(*get_symbol(car(P)) == symbol_from_string(tbl, "tuple"))
	{
		std::vector<const llvm::Type*> Params;
		pointer Param = cdr(P);
		while(Param != NIL)
		{
			assert(is_type(Param, DT_Pair));
			pointer cP = eval_typeinfo(car(Param), tbl, type_define_map);
			Params.push_back(typeinfo_get_llvm_type(cP));
			destroy_list(cP);
			Param = cdr(Param);
		}

		return make_primitive_typeinfo(llvm::StructType::get(llvm::getGlobalContext(), Params, false));
	}
	else if(*get_symbol(car(P)) == symbol_from_string(tbl, "struct"))
	{
		std::vector<const llvm::Type*> Params;
		std::vector<const char*> ParamNames;
		pointer Param = cdr(P);
		while(Param != NIL)
		{
			assert(is_type(Param, DT_Pair));
			assert(is_type(car(Param), DT_Pair));
			assert(is_type(caar(Param), DT_String));
			
			pointer cP = eval_typeinfo(cdar(Param), tbl, type_define_map);
			Params.push_back(typeinfo_get_llvm_type(cP));
			destroy_list(cP);
			ParamNames.push_back(string_from_symbol(tbl, *get_symbol(caar(Param))));
			Param = cdr(Param);
		}
		return make_struct_typeinfo(llvm::getGlobalContext(), ParamNames, Params);
	}
	else if(cdr(P) == NIL)
	{
		// Try to collapse
		return eval_typeinfo(pair_car(P), tbl, type_define_map);
	}
	else
		assert(false);
}

void transform_tree_gen_typedef(pointer P, symbol_table* tbl, type_map* type_define_map)
{
	if(P != NIL)
	{
		if(is_type(P, DT_Pair))
		{
			if(is_type(car(P), DT_Symbol) && *get_symbol(car(P)) == symbol_from_string(tbl, "define-type"))
			{
				(*type_define_map)[*get_symbol(cadr(P))] = eval_typeinfo(cddr(P), tbl, type_define_map);
				return;
			}
			
			while(P != NIL && is_type(P, DT_Pair))
			{
				transform_tree_gen_typedef(car(P), tbl, type_define_map);
				P = cdr(P);
			}
		}
	}
}

void transform_tree_gen_typeinfo(pointer P, symbol_table* tbl, type_map* type_define_map)
{
	if(P != NIL)
	{
		if(is_type(P, DT_Pair))
		{
			if(is_type(pair_car(P), DT_Symbol))
			{
				if(*get_symbol(pair_car(P)) == symbol_from_string(tbl, ":"))
				{
					set_car(P, eval_typeinfo(cadr(P), tbl, type_define_map));
					set_cdr(P, cddr(P));
				}
				else if(string_from_symbol(tbl, *get_symbol(pair_car(P)))[0] == ':')
				{
					pointer T = create_symbol(tbl, string_from_symbol(tbl, *get_symbol(pair_car(P)))+1);
					set_car(P, eval_typeinfo(T, tbl, type_define_map));
					destroy_list(T);
				}
			}
			else
			{
				transform_tree_gen_typeinfo(pair_car(P), tbl, type_define_map);
			}

			transform_tree_gen_typeinfo(pair_cdr(P), tbl, type_define_map);
		}
	}
}
