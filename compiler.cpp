#include "compiler_private.h"
#include "types.h"
#include "typeinfo.h"
#include <stdint.h>

using namespace llvm;

compiler* init_compiler(symbol_table* table)
{
	compiler* compile = new compiler();
	compile->sym_table = table;
	init_function_table(compile);
	return compile;
}

void destroy_compiler(compiler* compile)
{
	delete compile;
}

llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P);
llvm::Value* compiler_resolve_expression_list(compiler* compile, compile_block* block, pointer P);

compiler_scope* compiler_create_empty_scope(compiler_scope* parent_scope)
{
	compiler_scope* scope = new compiler_scope();
	scope->parent_scope = parent_scope;
	return scope;
}

void compiler_destroy_scope(compiler_scope* scope)
{
	delete scope;
}

void compiler_add_to_scope(compile_block* block, symbol sym, llvm::Value* val)
{
	// Error on redef of variable at same scope... not dynamically bound in compiliation mode.
	block->current_scope->scope_map[sym] = val;
}

llvm::Value* compiler_find_in_scope(compile_block* block, symbol sym)
{
	compiler_scope* scope = block->current_scope;
	while(scope != NULL)
	{
		std::map<symbol, llvm::Value*>::iterator it = scope->scope_map.find(sym);
		if(it != scope->scope_map.end())
			return it->second;
		scope = scope->parent_scope;
	}
	return NULL;
}

compile_block* compiler_create_function_block(compiler* compile, const char* Name, const llvm::Type* RetType, pointer Params, compile_block* parent_block)
{

	std::vector<const Type*> ParamTypes;

	pointer P = Params;

	while(P != NIL)
	{
		assert(is_type(P, DT_Pair));
		assert(is_type(pair_car(P), DT_Symbol));
		assert(is_type(cadr(P), DT_TypeInfo));

		ParamTypes.push_back(typeinfo_get_llvm_type(cadr(P)));
		P = cddr(P);
	}

	compile_block* block = new compile_block();

	FunctionType* ft = FunctionType::get(RetType, ParamTypes, false);

	Constant* c = compile->module->getOrInsertFunction(Name, ft);
	assert(isa<Function>(c));
	block->function = cast<Function>(c);
	block->function->setCallingConv(CallingConv::C);

	compiler_scope* parent_scope = NULL;
	if(parent_block)
		parent_scope = parent_block->current_scope;
	block->current_scope = compiler_create_empty_scope(parent_scope);

	Function::arg_iterator args = block->function->arg_begin();

	P = Params;
	while(P != NIL)
	{
		Value* v = args++;
		assert(is_type(P, DT_Pair));
		assert(is_type(pair_car(P), DT_Symbol));
		assert(is_type(cadr(P), DT_TypeInfo));

		symbol S = *get_symbol(pair_car(P));
		v->setName(string_from_symbol(compile->sym_table, S));
		compiler_add_to_scope(block, S, v);
		P = cddr(P);
	}

	block->block = BasicBlock::Create("entry", block->function);
	block->builder = IRBuilder<>(block->block);

	block->last_exp = NULL;

	return block;

	
}

void compiler_destroy_function_block(compile_block* block)
{
	// llvm should maintain it's own types, we just want to get rid of the block we pass around to build it.
	compiler_destroy_scope(block->current_scope);
	delete block;
}

compile_block* compiler_init_module(compiler* compile)
{
	compile->module = new Module("");

	return compiler_create_function_block(compile, "_Entry");
}

llvm::Value* compiler_resolve_variable(compiler* compile, compile_block* block, symbol S)
{
	Value* var = compiler_find_in_scope(block, S);
	assert(var);
	return var;
}

llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P)
{

	switch(get_type_id(P))
	{
	case DT_Pair:
	{
		Function* function = NULL;
		if(is_type(pair_car(P), DT_Symbol))
		{
			// Try to resolve special forms first!
			symbol S = *get_symbol(pair_car(P));
			compiler_special_form* fun = &compile->form_table[S];
			if(fun && fun->special_form)
				return (*fun->special_form)(compile, block, P);
		}

		Value* var = compiler_resolve_expression(compile, block, pair_car(P));
		if(var && isa<Function>(var))
			function = cast<Function>(var);
		else
			assert(false);

		SmallVector<Value*, 4> Params;
		P = pair_cdr(P);
		while(P != NIL)
		{
			Params.push_back(compiler_resolve_expression(compile, block, pair_car(P)));
			if(is_type(P, DT_Pair))
				P = pair_cdr(P);
			else
				P = NIL;
		}
		
		return block->builder.CreateCall(function, Params.begin(), Params.end());
	}
	case DT_Symbol:
	{
		// Resolve variables
		Value* var = compiler_resolve_variable(compile, block, *get_symbol(P));
		assert(var);
		return var;
	}
	case DT_Int:
		return ConstantInt::get(APInt(32, get_int(P), true));
	case DT_Real:
		return ConstantFP::get(APFloat(get_real(P)));
	case DT_String:
		//TODO: Needs Implementation
		assert(false);
		break;
	case DT_Char:
		return ConstantInt::get(APInt(8, get_char(P), false));
	default:
		assert(false);
		break;
	}

}

llvm::Value* compiler_resolve_expression_list(compiler* compile, compile_block* block, pointer P)
{
	llvm::Value* LastExp = NULL;
	while(P != NIL)
	{

		if(is_type(P, DT_Pair))
		{
			LastExp = compiler_resolve_expression(compile, block, pair_car(P));
			P = pair_cdr(P);
		}
		else
			P = NIL;
	}
	return LastExp;
}

compile_block* compiler_compile_expression(compiler* compile, pointer P)
{
	compile_block* block = compiler_init_module(compile);

	compiler_resolve_expression_list(compile, block, P);
	block->builder.CreateRetVoid();

	compiler_destroy_function_block(block);

	// Seperate the rest of this into pass and run modules.
	
	verifyModule(*compile->module, PrintMessageAction);

	PassManager PM;
	PM.add(createPrintModulePass(&llvm::outs()));
	PM.run(*compile->module);

	return NULL;
}
