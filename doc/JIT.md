The JIT compiler consists of two parts, the jit module and the `jitcomp`
program.  The jit module is loaded by Hydra or DGD:
```
    modules = ([ "/home/user/lpc-ext/jit.1.5" : "/home/user/lpc-ext/src/jit" ]);
```
The directory `/home/user/lpc-ext/jit` specifies where the `jitcomp` program
can be found, and where the JIT compiler cache will be created.

The jit module saves LPC bytecode programs in the cache.  The compiler then
decompiles the bytecode into basic blocks, performs flow analysis,
determines the types of variables and expressions, and emits LLVM IR as a `.ll`
file in the cache.  Clang is executed to compile the `.ll` file into a shared
object, and if the compilation succeeds the jit module will load the shared
object and make the code available to the LPC runtime.

Running the `jitcomp` program independently ensures that any memory leaks and
crashes will not affect the main program.  Storing the JIT-compiled objects in
a cache makes them available for re-use.

Decompiling to LLVM IR, and using clang to compile that to a shared object,
simplifies JIT compilation considerably.  Decompiling to LLVM bitcode, and
compiling that using the LLVM libraries, is left as an exercise to the reader.
