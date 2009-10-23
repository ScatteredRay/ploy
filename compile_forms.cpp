#include "compiler_private.h"
#include "types.h"
#include "typeinfo.h"
#include <stdint.h>

using namespace llvm;

typedef llvm::Value* (*combine_op)(compile_block* block, llvm::Value* lhv, llvm::Value* rhv, void* UP);

llvm::Value* compiler_resolve_list_op(compiler* compile, compile_block* block, pointer P, combine_op opfunc, void* UP)
{
	Value* lhv = NULL;
	while(P != NIL)
	{
		if(!lhv)
			lhv = compiler_resolve_expression(compile, block, pair_car(P));
		else
		{
			Value* rhv = compiler_resolve_expression(compile, block, pair_car(P));
			lhv = opfunc(block, lhv, rhv, UP);
		}
		
		if(is_type(P, DT_Pair))
			P = pair_cdr(P);
		else
			P = NIL;
	}
	return lhv;
}

llvm::Value* combine_bin_op(compile_block* block, llvm::Value* lhv, llvm::Value* rhv, void* UP)
{
	return block->builder->CreateBinOp(*(Instruction::BinaryOps*)UP, lhv, rhv);
}

llvm::Value* compiler_resolve_bin_op(compiler* compile, compile_block* block, pointer P, Instruction::BinaryOps binop)
{
	return compiler_resolve_list_op(compile, block, P, combine_bin_op, &binop);
}

llvm::Value* compiler_add_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Add);
}

llvm::Value* compiler_sub_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Sub);
}

llvm::Value* compiler_mul_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Mul);
}

llvm::Value* combine_div_op(compile_block* block, llvm::Value* lhv, llvm::Value* rhv, void* UP)
{
	if(lhv->getType()->isIntOrIntVector() && rhv->getType()->isIntOrIntVector())
		return block->builder->CreateBinOp(Instruction::SDiv, lhv, rhv);
	else if(lhv->getType()->isFPOrFPVector() && rhv->getType()->isFPOrFPVector())
		return block->builder->CreateBinOp(Instruction::FDiv, lhv, rhv);
	else
		assert(false);
}

llvm::Value* compiler_div_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_list_op(compile, block, pair_cdr(P), combine_div_op, NULL);
}

llvm::Value* compiler_define_form(compiler* compile, compile_block* block, pointer P)
{
	assert(block);
	pointer Def = cadr(P);
	llvm::Value* Ret = NULL;
	symbol var_sym;
	if(is_type(Def, DT_Pair))
	{
		// Function define
		var_sym = *get_symbol(pair_car(Def));
		const char* fun_name = string_from_symbol(compile->sym_table, var_sym);
		assert(is_type(caddr(P), DT_TypeInfo));
		compile_block* FunctionBlock = compiler_create_function_block(compile, fun_name, typeinfo_get_llvm_type(caddr(P)), pair_cdr(Def), block);
        llvm::Value* LastExp = compiler_resolve_expression_list(compile, FunctionBlock, cdddr(P));
		FunctionBlock->builder->CreateRet(LastExp);
		Ret = FunctionBlock->function;
		compiler_destroy_function_block(FunctionBlock);
	}
	else
	{
		assert(false);
		// Var define
	}

	compiler_add_to_scope(block, var_sym, Ret);
	return Ret;
}

llvm::Value* compiler_typedef_form(compiler* compile, compile_block* block, pointer P)
{
	// typedefs are handled in a prior typing pass. just ignore them.
	return NULL; // hopefully this won't break anything expecting a real value.
}

llvm::Value* compiler_declare_form(compiler* compile, compile_block* block, pointer P)
{
	assert(block);
	pointer Def = cadr(P);
	llvm::Value* Ret = NULL;
	symbol var_sym;
	if(is_type(Def, DT_Pair))
	{
		var_sym = *get_symbol(pair_car(Def));
		const char* fun_name = string_from_symbol(compile->sym_table, var_sym);
		assert(is_type(caddr(P), DT_TypeInfo));
		FunctionType* ft = FunctionType::get(typeinfo_get_llvm_type(caddr(P)), compiler_populate_param_types(compile, cdr(Def)), false);
		Function* f = Function::Create(ft, Function::ExternalLinkage, fun_name, compile->module);
		Ret = f;
		if(strcmp(f->getNameStr().c_str(), fun_name) != 0)
		{
			f->eraseFromParent(); // Don't mess up on redeclerations.
			Ret = compile->module->getFunction(fun_name);
		}
		
	}
	else
	{
		assert(false);
	}

	compiler_add_to_scope(block, var_sym, Ret);
	return Ret;
}

llvm::Value* compiler_make_tuple_form(compiler* compile, compile_block* block, pointer P)
{
	assert(block);
	std::vector<llvm::Value*> Params;
	P = cdr(P);
	while(P != NIL)
	{
		assert(is_type(P, DT_Pair));
		Params.push_back(compiler_resolve_expression(compile, block, car(P)));
		P = cdr(P);
	}
	
	std::vector<const llvm::Type*> Types;
	for(std::vector<llvm::Value*>::iterator it = Params.begin(); it != Params.end(); it++)
		Types.push_back((*it)->getType());

	llvm::Value* ret = llvm::UndefValue::get(llvm::StructType::get(llvm::getGlobalContext(), Types, false));
	unsigned int i = 0;
	for(std::vector<llvm::Value*>::iterator it = Params.begin(); it != Params.end(); it++)
		ret = block->builder->CreateInsertValue(ret, *it, i++);

	return ret;
}

llvm::Value* compiler_index_form(compiler* compile, compile_block* block, pointer P)
{
	assert(block);
	assert(is_type(P, DT_Pair) && is_type(cdr(P), DT_Pair) && is_type(cddr(P), DT_Pair));
	assert(is_type(cadr(P), DT_Int));
	llvm::Value* Record = compiler_resolve_expression(compile, block, caddr(P));
	return block->builder->CreateExtractValue(Record, get_int(cadr(P)));
}

void declare_form(compiler* compile, const char* sym, form_compile_func cfunc)
{
	compile->form_table[symbol_from_string(compile->sym_table, sym)].special_form = cfunc;
}

void init_function_table(compiler* compile)
{
	declare_form(compile, "define", compiler_define_form);
	declare_form(compile, "declare", compiler_declare_form);
	declare_form(compile, "define-type", compiler_typedef_form);
	declare_form(compile, "make-tuple", compiler_make_tuple_form);
	declare_form(compile, "#", compiler_index_form);
	declare_form(compile, "+", compiler_add_form);
	declare_form(compile, "*", compiler_mul_form);
	declare_form(compile, "/", compiler_div_form);
}
