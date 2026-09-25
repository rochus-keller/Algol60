## About this project

Algol 60 is the mother of nearly all imperative programming languages existing today. And it turns 60 years this year (2020) which is a good reason to take again a closer look at it. The original Algol 60 language report appeared in May 1960 in the venerable Communications of the ACM. For several years I have been thinking about building a compiler for the language Simula 67 which is a superset of Algol 60. To build a parser (and possibly a compiler) for Algol 60 is therefore on the way to the Simula compiler. So here we go.

Algol was the first language which was specified using the "Backus–Naur form" (BNF), yet another pioneering achievment. I took the BNF from the revised report and converted it in an LL(1) EBNF using my EbnfStudio tool (see https://github.com/rochus-keller/EbnfStudio, which I had to extend a bit to handle the unusual unicode symbols used by Algol). 

The generated parser successfully reads the examples of Marst, Katwijk-algol-60, racket-algol60 and swornimgrg-algol60; it also successfully parses all Marst test cases besides the ones with intentional syntax errors. The AlgLc application can be used to parse all algol files in a directory. 

I also implemented a syntax highlighter and a little Algol60 editor based on Qt (called AlgLjEditor, see screenshot). I added a LuaJIT terminal and bytecode viewer in case I will implement an Algol 60 to LuaJIT bytecode compiler (as I already did e.g. in https://github.com/rochus-keller/Oberon). This is work in progress.

![Overview](http://software.rochus-keller.ch/AlgLjEditor_screenshot_1.png)

There is now also a hand-crafted parser which generates an AST, and a validator which evaluates the semantics and resolves symbols, as in the Simula and ActiveOberon projects.

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
Next an AIR to LuaJIT renderer will be implemented.

### Binary versions

Not yet available.

### Build Steps

Follow these steps if you want to build the application yourself:

1. Make sure a Qt 5.x (libraries and headers) version compatible with your C++ compiler is installed on your system.
1. Download the source code from https://github.com/rochus-keller/Algol60/archive/master.zip and unpack it.
1. Goto the unpacked directory and execute `QTDIR/bin/qmake AlgLjEditor.pro` (see the Qt documentation concerning QTDIR).
1. Run make; after a couple of seconds you will find the executable in the build directory.

Alternatively you can open AlgLjEditor.pro using QtCreator and build it there.

## Support
If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/Algol60/issues or send an email to the author.



 
