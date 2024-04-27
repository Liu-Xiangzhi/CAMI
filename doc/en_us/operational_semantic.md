# Operational Semantic of C

## Formal Definition of Abstract Machine
### Overview
CAMI's operation semantics divide the storage space into three parts: operand stack, metadata area, and data area. The operation semantics revolve around the operand stack for computation, around the metadata area for behavior detection, while data read and written by the CAMI process are stored in the data area. The metadata area records program pointer, stack pointer, object designation register, runtime call stack information, and object metadatas. Object metadata information includes object name, type, address, status, etc.

The behaviors present in the operation semantics can be categorized into the following 7 types:
+ control flow control: used for intra-function jumps, function calls, and returns, changing program pointer and runtime call stack information
+ object designation: designating objects based on the top element of the operand stack or current instruction information and saving the object metadata in the object designation register
+ read, write object: reading from and writing to the value of the object designated by the object designation register, involving read and write operations on the operand stack and data area
+ new, delete object: creating, modifying, or deleting object metadata information
+ computation: popping the top(and sub-top) element(s) of the stack for computation and pushing the result back onto the stack
+ stack operation: stack push, pop, duplication, etc
+ halt

### Formal Definition
TBD

## Formal Translation Rules
The translation rules formally defines the map from AST of C program to the initial state(i.e. the instruct sequence and static program information, also the CAMI bytecode) of abstract machine.

TBD
