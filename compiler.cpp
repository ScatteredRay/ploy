#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "compiler.h"
#include <stdint.h>
#include "types.h"

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

struct compiler
{
	symbol_table* sym_table;
	Module* module;
};

struct compile_block
{
	Function* function;
	BasicBlock* block;
	IRBuilder<> builder;
	Value* last_exp;
};

compiler* init_compiler(symbol_table* table)
{
	compiler* compile = new compiler();
	compile->sym_table = table;
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
		if(is_type(P, DT_Pair))
		{
			assert(!is_type(pair_car(P), DT_Pair));
			ParamTypes.push_back(IntegerType::get(32));
			P = pair_cdr(P);
		}
		else
		{
			ParamTypes.push_back(IntegerType::get(32));
			P = NIL;
		}
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
		if(is_type(P, DT_Pair))
		{
			assert(is_type(pair_car(P), DT_Symbol));
			v->setName(string_from_symbol(compile->sym_table, *get_symbol(pair_car(P))));
			P = pair_cdr(P);
		}
		else
		{
			assert(is_type(P, DT_Symbol));
			v->setName(string_from_symbol(compile->sym_table, *get_symbol(P)));
			P = NIL;
		}
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

llvm::Value* compiler_run_special_form(compiler* compile, compile_block* block, pointer P)
{
	symbol S = *get_symbol(pair_car(P));
	if(S == symbol_from_string(compile->sym_table, "Define"))
	{
		pointer Def = pair_car(pair_cdr((P)));
		if(is_type(Def, DT_Pair))
		{
			// Function define
			
			compile_block* FunctionBlock = compiler_create_function_block(compile, string_from_symbol(compile->sym_table, *get_symbol(pair_car(Def))), IntegerType::get(32), pair_cdr(Def));
			compiler_resolve_expression_list(compile, FunctionBlock, pair_cdr(pair_cdr(P)));
			FunctionBlock->builder.CreateRet(FunctionBlock->last_exp);
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
	else if(S == symbol_from_string(compile->sym_table, "+"))
	{
		return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Add);
	}
	else if(S == symbol_from_string(compile->sym_table, "*"))
	{
		return compiler_resolve_bin_op(compile, block, pair_cdr(P), Instruction::Mul);
	}
}

llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P)
{

	switch(get_type_id(P))
	{
	case DT_Pair:
		if(is_type(pair_car(P), DT_Symbol))
		{
			// Try to resolve special forms first!
			return compiler_run_special_form(compile, block, P);
		}
		else
		{
			assert(false);
			Value* function = compiler_resolve_expression(compile, block, pair_car(P));
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
			
		}
		
		break;
	case DT_Symbol:
		// Resolve variables
		const char* name = string_from_symbol(compile->sym_table, *get_symbol(P));
		Value* var = block->function->getValueSymbolTable().lookup(name);
		if(var == NULL)
			var = compile->module->getGlobalVariable(name);
		assert(var);
		return var;
	case DT_Int:
		return ConstantInt::get(APInt(32, get_int(pair_car(P)), true));
	case DT_Real:
		return ConstantFP::get(APFloat(get_real(pair_car(P))));
	case DT_String:
		//TODO: Needs Implementation
		assert(false);
		break;
	case DT_Char:
		return ConstantInt::get(APInt(8, get_char(pair_car(P)), false));
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
