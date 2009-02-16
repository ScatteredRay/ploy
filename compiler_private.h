#ifndef _compiler_private_h_
#define _compiler_private_h_

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "types.h"
#include "compiler.h"
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

struct compile_block
{
	llvm::Function* function;
	llvm::BasicBlock* block;
	llvm::IRBuilder<> builder;
	llvm::Value* last_exp;
};

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
	llvm::Module* module;
	std::map<symbol, compiler_function, sym_cmp> function_table;
};

void init_function_table(compiler* compile);

compile_block* compiler_create_function_block(compiler* compile, const char* Name="", const llvm::Type* RetType = llvm::Type::VoidTy, pointer Params=NIL);
void compiler_destroy_function_block(compile_block* block);
llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P);
llvm::Value* compiler_resolve_expression_list(compiler* compile, compile_block* block, pointer P);

#endif //_compiler_private_h_

