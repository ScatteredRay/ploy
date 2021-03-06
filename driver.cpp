// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include "symbol.h"
#include "parser.h"
#include "types.h"
#include "compiler.h"
#include "typeinfo.h"
#include "include.h"

#include "symbols.h"
#include "ploy_calls.h"

#include "stdio.h"
#include <map>
#include "llvm/Support/CommandLine.h"

symbol_table* sym_tbl;

static llvm::cl::opt<std::string> InputFile(llvm::cl::desc("<input source>"),
                                            llvm::cl::Positional,
                                            llvm::cl::Required);

static llvm::cl::opt<std::string> OutputFile("output-file",
                                             llvm::cl::desc("File to direct the output to."), 
                                             llvm::cl::init("out.ll"));

static llvm::cl::alias Output("o",
                              llvm::cl::aliasopt(OutputFile),
                              llvm::cl::desc("Alias for -output-file."));

static llvm::cl::opt<std::string> EntryFunc("entry-func",
                                            llvm::cl::desc("Name of functon to generate for module entry code"),
                                            llvm::cl::init("main"));


int main(int argc, char** argv)
{
    // Should ParseCommandLineOptions be able to accept a const argv?
    llvm::cl::ParseCommandLineOptions(argc, argv, "ploy compiler\n");

    const char* file_location = InputFile.c_str();

    symbol_table* tbl = sym_tbl = init_symbol_table();
    init_symbols(tbl);

    pointer ret = parse_file_to_tree(file_location, tbl);
    if(!ret)
        return 1;

    materialize_includes(&ret, tbl);

    ploy_do_compile(ret, tbl);
    type_map type_define_map;
    transform_tree_gen_typedef(ret, tbl, &type_define_map);
    transform_tree_gen_typeinfo(ret, tbl, &type_define_map);

    compiler* compile = init_compiler(tbl);

    compiler_compile_expression(compile, ret, EntryFunc.c_str());
    compiler_print_module(compile);
    compiler_write_asm_file(compile, OutputFile.c_str());

    destroy_compiler(compile);

    destroy_symbol_table(tbl);
    return 0;
}
