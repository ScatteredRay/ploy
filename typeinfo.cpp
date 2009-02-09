#include "typeinfo.h"
#include "types.h"
#include "symbol.h"

void set_car(pointer P, pointer V);
void set_cdr(pointer P, pointer V);

namespace llvm
{
	class Type;
}

class typeinfo
{
};

llvm::Type* typeinfo_get_llvm_type(typeinfo* type);

pointer eval_typeinfo(pointer P, symbol_table* tbl)
{
	return create_symbol(tbl, ":type");
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
