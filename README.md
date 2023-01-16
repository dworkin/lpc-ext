The overaching license for the LPC extensions is the Unlicense.  Some modules
or portions thereof in the `src/kfun` directory have their own open source
license; please consult the relevant code for the details.

-   doc  
    documentation
-   src  
    API glue to compile with your module
-   src/dbase  
    example database module
-   src/jit  
    JIT compiler module
-   src/kfun  
    kfun modules

To build modules for most kfuns, just type `make` (or whatever your GNU make
command is) in the src directory.  On Windows, use Visual Studio 2010 with
the solution file `lpc-ext.sln`, or use `make EXT=dll` with Cygwin.

Use `make zlib` to build the zlib module.  Requires zlib 1.2.11 dev headers.

Use `make dbase` to create the example database module for Hydra.

Use `make jit` to build the JIT compiler module.  This requires clang to
be installed (any version from 3.7 onward will work).  The jit module
will start the actual JIT compiler as a separate program.  It must be
configured with the directory that contains the JIT compiler, and where
the JIT cache will be created:
```
    modules = ([
		 "/home/server/lpc-ext/jit.1.4" :	/* JIT module */
		 "/home/server/lpc-ext/src/jit"	/* where jitcomp resides */
	      ]);
```
On Windows, there are different solution files for the components of the JIT
compiler.  `lpc-ext.sln` is used to build the JIT extension, `jitcomp.sln` is
used to build the actual jit compiler which runs as a separate program, and
which depends on clang.  Clang can be installed as a component of Visual Studio
Community 2022.  Note that jitcomp.sln is a VS2019 solution, whereas
`lpc-ext.sln` can be built with any Visual Studio version from 2010 onward.
