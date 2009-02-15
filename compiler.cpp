#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "compiler.h"
#include "types.h"
#include "typeinfo.h"
#include <stdint.h>
#include <map>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ValueSymbolTable.h>

using namespace llvm;

struct compiler_function
{
	llvm::Value* (*special_form)(compiler*, compile_block*, pointer);
	llvm::Function* function;
	compiler_function()
	{
		special_form = NULL;
		function = NULL;
	}
};

struct compiler
{
	symbol_table* sym_table;
	Module* module;
	std::map<symbol, compiler_function, sym_cmp> function_table;
};

struct compile_block
{
	Function* function;
	BasicBlock* block;
	IRBuilder<> builder;
	Value* last_exp;
};

void init_function_table(compiler* compile);

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

compile_block* compiler_create_function_block(compiler* compile, const char* Name="", const llvm::Type* RetType = Type::VoidTy, pointer Params=NIL)
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

	Function::arg_iterator args = block->function->arg_begin();

	P = Params;
	while(P != NIL)
	{
		Value* v = args++;
		assert(is_type(P, DT_Pair));
		assert(is_type(pair_car(P), DT_Symbol));
		assert(is_type(cadr(P), DT_TypeInfo));

		v->setName(string_from_symbol(compile->sym_table, *get_symbol(pair_car(P))));
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
	delete block;
}

compile_block* compiler_init_module(compiler* compile)
{
	compile->module = new Module("");

	return compiler_create_function_block(compile, "_Entry");
}

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

llvm::Value* compiler_add_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Add);
}

llvm::Value* compiler_mul_form(compiler* compile, compile_block* block, pointer P)
{
	return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Mul);
}

void init_function_table(compiler* compile)
{
	compile->function_table[symbol_from_string(compile->sym_table, "Define")].special_form = compiler_define_form;
	compile->function_table[symbol_from_string(compile->sym_table, "+")].special_form = compiler_add_form;
	compile->function_table[symbol_from_string(compile->sym_table, "*")].special_form = compiler_mul_form;
}

llvm::Value* compiler_resolve_variable(compiler* compile, compile_block* block, symbol S)
{
		const char* name = string_from_symbol(compile->sym_table, S);
		Value* var = block->function->getValueSymbolTable().lookup(name);
		if(var == NULL)
			var = compile->module->getGlobalVariable(name);
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
			compiler_function* fun = &compile->function_table[S];
			if(fun->special_form)
				return (*fun->special_form)(compile, block, P);
			else if(fun->function)
				function = fun->function;
			else
			{
				Value* var = compiler_resolve_variable(compile, block, S);
				if(var && isa<Function>(var))
					function = cast<Function>(var);
				else
					assert(false);
			}
		}
		else
		{
			Value* var = compiler_resolve_expression(compile, block, pair_car(P));
			if(var && isa<Function>(var))
				function = cast<Function>(var);
			else
				assert(false);
		}

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
