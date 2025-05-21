# compiler

## TODO

#### Frontend
 - convert to wchar
 - parse structs and classes
 - debug everything else
 - refactor so it looks more or less ok

#### Backend
 - assemble into NASM files

##### Misc
Multifile build:
 - assemble object source files into object files using NASM
 - then link using ld when specified

 ##### IR


store identifiers in IR
assign results of operations to new vars/registers like in LLVM IR

 - func 
    args
    ret val
 - func call
    - call args
    - where to put ret val
 - arithmetic
 - local var
 - conditional and uncondiitonal jumps (if/for/while)
