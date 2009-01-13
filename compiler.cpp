#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "compiler.h"
#include <stdint.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

struct compiler
{
};

struct compiled
{
	Module* module;
};

compiler* init_compiler(symbol_table* table)
{
	return new compiler();
}

void destroy_compiler(compiler* compile)
{
	delete compile;
}

compiled* compiler_create_compiled(compiler* compile)
{
	compiled* comped = new compiled();
	comped->module = new Module("");
	
	Constant* c = comped->module->getOrInsertFunction("mul_add", 
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

	BasicBlock* block = BasicBlock::Create("entry", mul_add);
	IRBuilder<> builder(block);

	Value* tmp = builder.CreateBinOp(Instruction::Mul, x, y, "tmp");
	Value* tmp2 = builder.CreateBinOp(Instruction::Add, tmp, z, "tmp2");
	builder.CreateRet(tmp2);

	return comped;
}

compiled* compiler_compile_expression(compiler* compile, pointer P)
{
	compiled* comped = compiler_create_compiled(compile);

	// Seperate the rest of this into pass and run modules.
	
	verifyModule(*comped->module, PrintMessageAction);

	PassManager PM;
	PM.add(createPrintModulePass(&llvm::outs()));
	PM.run(*comped->module);

	return NULL;
}
