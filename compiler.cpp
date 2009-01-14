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

using namespace llvm;

struct compiler
{
	Module* module;
};

struct compile_block
{
	BasicBlock* block;
	IRBuilder<> builder;
};

compiler* init_compiler(symbol_table* table)
{
	return new compiler();
}

void destroy_compiler(compiler* compile)
{
	delete compile;
}

compile_block* compiler_init_module(compiler* compile)
{
	compile->module = new Module("");
	
	Constant* c = compile->module->getOrInsertFunction("mul_add", 
													   IntegerType::get(32),
													   IntegerType::get(32),
													   IntegerType::get(32),
													   IntegerType::get(32),
													   NULL);

	Function* mul_add = cast<Function>(c);
	mul_add->setCallingConv(CallingConv::C);

	Function::arg_iterator args = mul_add->arg_begin();
	Value* x = args++;
	Value* y = args++;
	Value* z = args++;
	
	x->setName("x");
	y->setName("y");
	z->setName("z");

	compile_block* block = new compile_block();

	block->block = BasicBlock::Create("entry", mul_add);
	block->builder = IRBuilder<>(block->block);

	Value* tmp = block->builder.CreateBinOp(Instruction::Mul, x, y, "tmp");
	Value* tmp2 = block->builder.CreateBinOp(Instruction::Add, tmp, z, "tmp2");
	block->builder.CreateRet(tmp2);

	return block;
}

llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P)
{

	switch(get_type_id(P))
	{
	case DT_Pair:
		if(is_type(pair_car(P), DT_Symbol))
		{
			// Try to resolve special forms first!
		}
		else
		{
			Value* function = compiler_resolve_expression(compile, block, pair_car(P));
			SmallVector<Value*, 4> Params;
			while(P != NIL)
			{
				Params.push_back(compiler_resolve_expression(compile, block, pair_cdr(P)));
				if(is_type(P, DT_Pair))
					P = pair_cdr(P);
				else
					P = NIL;
			}
		}
		
		break;
	case DT_Symbol:
		// Resolve variables
		break;
	case DT_Int:
		return ConstantInt::get(APInt(32, get_int(pair_car(P)), true));
		break;
	case DT_Real:
		return ConstantFP::get(APFloat(get_real(pair_car(P))));
	case DT_String:
		//TODO: Needs Implementation
		break;
	case DT_Char:
		return ConstantInt::get(APInt(8, get_char(pair_car(P)), false));
		break;
	default:
		assert(false);
		break;
	}

}

compile_block* compiler_compile_expression(compiler* compile, pointer P)
{
	compile_block* block = compiler_init_module(compile);

	compiler_resolve_expression(compile, block, P);

	// Seperate the rest of this into pass and run modules.
	
	verifyModule(*compile->module, PrintMessageAction);

	PassManager PM;
	PM.add(createPrintModulePass(&llvm::outs()));
	PM.run(*compile->module);

	return NULL;
}
