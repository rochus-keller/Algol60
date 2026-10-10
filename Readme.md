## About this project

Algol 60 is the mother of nearly all imperative programming languages existing today. And it turns 60 years this year (2020) which is a good reason to take again a closer look at it. The original Algol 60 language report appeared in May 1960 in the venerable Communications of the ACM. For several years I have been thinking about building a compiler for the language Simula 67 which is a superset of Algol 60. To build a parser (and possibly a compiler) for Algol 60 is therefore on the way to the Simula compiler. So here we go.

Algol was the first language which was specified using the "Backus–Naur form" (BNF), yet another pioneering achievment. I took the BNF from the revised report and converted it in an LL(1) EBNF using my EbnfStudio tool (see https://github.com/rochus-keller/EbnfStudio, which I had to extend a bit to handle the unusual unicode symbols used by Algol). 

The generated parser successfully reads the examples of Marst, Katwijk-algol-60, racket-algol60 and swornimgrg-algol60; it also successfully parses all Marst test cases besides the ones with intentional syntax errors. The AlgLc application can be used to parse all algol files in a directory. 

I also implemented a syntax highlighter and a little Algol60 editor based on Qt (called AlgLjEditor, see screenshot). I added a LuaJIT terminal and bytecode viewer in case I will implement an Algol 60 to LuaJIT bytecode compiler (as I already did e.g. in https://github.com/rochus-keller/Oberon). This is work in progress.

![Overview](http://software.rochus-keller.ch/AlgLjEditor_screenshot_1.png)

There is now also a hand-crafted parser which generates an AST, and a validator which evaluates the semantics and resolves symbols, as in the Simula and ActiveOberon projects,
and in addition a LuaJIT bytecode generator and integrated VM, so algc can not only compile, but also directly run Algol programs.

### Status on Sept. 16, 2026

Motivated by the success of the Micron Intermediate language (MIL) which helped to make the various backends much simpler compared to my experience with Oberon+ and Luon,
I decided to create a higher-level intermediate representation for languages in the Algol lineage (like my Oberon and Luon) which don't support taking the address
of variables, fields, elements and parameters, and thus are easier to integrate with a LuaJIT backend. My various attempts with adding a LuaJIT backend to MIL
were not successful (eventually because of the low performance due to my own stack mangagement and the required type conversions when using LuaJIT as a "C interpreter").
This was enough motivation to create an alternative representation which better suits the lowering level on which LuaJIT bytecode operates, instead of directly
lowering to MIL (the latter is still planned). My Algol60 adventure started in 2020 (originally to better understand the Simula syntax and because of the anniversary)
and I used the implementation for various experiments since, and now as the home and first implementation of the _Algol Intermediate Representation_ (AIR).
There is a first version of the specification in the docs directory, and the infrastructure to generate it (using an equivalent design I implemented for 
the Micron project). AIR is designed for reusability with (hopefully) all the features required for Oberon+, Luon, ActiveOberon and Simula 67.

### Status on Sept. 25, 2026

The AIR infrastructure is implemented. It's essentially a replica of the MIL emitter/renderer/AST/validator/serializer infrastructure. 
In contrast to MIL there is no AIR lexer/parser; the AST is directly generated via AST renderer and the text rendering is just for debugging.

### Status on Oct. 1, 2026

The AIR to LuaJIT renderer and an algol runtime written in Lua is implemented and there is a new compiler driver.
Many of marst success work; also quite many of the other programs of the corpus work.
The problem is that the standard doesn't include I/O features, which leads to incompatibilities
I need to add more built-in I/O features to cover more programs (which e.g. miss "newline")

### Status on Oct. 8, 2026

Refactored built-in approach so the can be registered and used dynamically. Added built-ins via runtime to cover 
a larger part of the corpus. Also added support for FFI generation of foreign procs to call C from libraries;
the argument types are limited to basic and string types; the generated chunk passes the C declaration derived from the AIR signature.
A performance improvement when creating records and initializing fields; a prototype is created once when its module is initialized
and must not be the first instantiation compiles a constructor which has the default values built in.
The compiler and integrated VM seem to have reached MVP status: of the 195 files in the corpus (Katwijk-algol-60,
marst, swornimgrg-algol60) 164 run; the ones that don't work are from racket-algol60 and all of the marst intended errors.

### Binary versions

The following precompiled versions are available at this time:

- [Linux x64](https://software.rochus-keller.ch/algc_linux_x64.tar.gz)
- [Windows x86](https://software.rochus-keller.ch/algc_windows_x86.zip)

### Build Steps

Follow these steps if you want to build algc yourself. The build is using LeanQt and the BUSY build system. Note that compiling on 
Linux requires the build essentials. On Mac and Windows there are no additional requirements than a toolchain.

1. Create a new directory; we call it the root directory here
1. Download https://github.com/rochus-keller/Algol60/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "Algol60".
1. Download https://software.rochus-keller.ch/LuaJIT-2.1-patched.zip and unpack it to the root directory; rename the resulting directory to "LuaJIT-2.1" if necessary.
1. Download https://github.com/rochus-keller/LjTools/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LjTools".
1. Download https://github.com/rochus-keller/LeanQt/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanQt".
1. Download https://github.com/rochus-keller/BUSY/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "build".
1. Open a command line in the "LuaJIT-2.1" directory and follow the instructions there to build LuaJIT for your platform. 
1. Open a command line in the "build" directory and type `cc *.c -O2 -lm -o lua` or `cl /O2 /MD /Fe:lua.exe *.c` depending on whether you are on a Unix or Windows machine; wait a few seconds until the Lua executable is built.
1. Now type `./lua build.lua ../Luon` (or `lua build.lua ../Algol60` on Windows); wait until the algc executable is built; you find it in the output subdirectory.

Instead of the command line you can run the build using [LeanCreator](https://github.com/rochus-keller/LeanCreator) which uses multiple cores and thus builds faster.

Alternatively you can build AlgCompiler.pro using Qt5 and qmake or QtCreator. You still need to build LuaJIT as indicated above.

## Support
If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/Algol60/issues or send an email to the author.



 
