# Ploy

Ploy was designed as a statically-typed, systems programming language with a strong meta-programming capacity. Specificially designed to be used in video game engine development.

The current implementation is a very early one, it uses llvm as a backend, and can currentlly compile a resonable subset into llvm bitcode.

The project has been mostly orphaned with Rust aligning pretty well to many of the initial goals.

## Quirks

### Bootstrapping

Since we compile to llvm bytecode, and ploy uses C types and calling conventions, we can bootstrap in a pretty neat way.

The core of the compiler is written in C++, this implements a subset of the language. One that most strictly, doesn't do type inference or type checking, or any of the other fancy things that we want to do.

Then we link in bootstrap_shims.cpp, which provides no-ops for elements of the compiler which are not part of the restricted subset, this allows us to build a "bootstrap" compiler.

We then use the bootstrap compiler to compile any ploy sources, at this point, we can just link those in using the standard llvm linker instead of the bootstrap_shims.cpp to generate a final, and fully featured compiler.

### Entry functions

Every module creates an entry function based on any code that is in the top-level of the source, this code gets inlined into a function specified by --entry-func which defaults to "main" that works ok when compiling a single file, but will cause conflicts with multiple compilation units, and don't get called automatically.

## Running Ploy

Compiliation of ploy isn't very streamlined, and the ploy compiler only generates llvm .ll files. But in general, compilation can be tested like so:

    ploy -o <intermediate.ll> <input.ply>
    llc -o <outputfile> <intermediate.ll>

The jamfile used for the compiler could be used as an example of how to build projects with ploy.
