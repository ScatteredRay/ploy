#include "compiler_private.h"
#include "types.h"
#include "typeinfo.h"
#include <stdint.h>

using namespace llvm;

llvm::Value* compiler_resolve_bin_op(compiler* compile, compile_block* block, pointer P, Instruction::BinaryOps binop)
{
	Value* lhv = NULL;
	while(P != NIL)
	{
		if(!lhv)
			lhv = compiler_resolve_expression(compile, block, pair_car(P));
		else
		{
			Value* rhv = compiler_resolve_expression(compile, block, pair_car(P));
			lhv = block->builder.CreateBinOp(binop, lhv, rhv);
		}
		
		if(is_type(P, DT_Pair))
			P = pair_cdr(P);
		else
			P = NIL;
	}
	block->last_exp = lhv;
	return lhv;
}

llvm::Value* compiler_add_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Add);
}

llvm::Value* compiler_mul_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Mul);
}

llvm::Value* compiler_define_form(compiler* compile, compile_block* block, pointer P)
{
		pointer Def = cadr(P);
		if(is_type(Def, DT_Pair))
		{
			// Function define
			symbol fun_sym = *get_symbol(pair_car(Def));
			const char* fun_name = string_from_symbol(compile->sym_table, fun_sym);
			assert(is_type(caddr(P), DT_TypeInfo));
			compile_block* FunctionBlock = compiler_create_function_block(compile, fun_name, typeinfo_get_llvm_type(caddr(P)), pair_cdr(Def));
			compiler_resolve_expression_list(compile, FunctionBlock, cdddr(P));
			FunctionBlock->builder.CreateRet(FunctionBlock->last_exp);
			compile->function_table[fun_sym].function = FunctionBlock->function;
			llvm::Value* Ret = FunctionBlock->function;
			compiler_destroy_function_block(FunctionBlock);
			return Ret;
		}
		else
		{
			assert(false);
			// Var define
			return NULL;
		}
}

void init_function_table(compiler* compile)
{
	compile->function_table[symbol_from_string(compile->sym_table, "Define")].special_form = compiler_define_form;
	compile->function_table[symbol_from_string(compile->sym_table, "+")].special_form = compiler_add_form;
	compile->function_table[symbol_from_string(compile->sym_table, "*")].special_form = compiler_mul_form;
}
