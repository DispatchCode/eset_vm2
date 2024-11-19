# ESETVM2 Emulator

ESET specifics: [Task.pdf](https://github.com/user-attachments/files/17654238/Task.pdf)

### Features (present or not)

✅ opcode, address, constant and arguments decoding<br>
✅ multithread<br>
✅ debug informations<br>
✅ disassembly<br>
✔️  emulation<br>


> ✅ - feature complete<br>
  ✔️  - feature still incomplete<br>
  ❌ - not yet present<br>

### Bugs
> 🐛 code padding not detected properly (ie sometime an instruction not present will be decoded).
	It should not affect the execution because every thread/program ends with `hlt` to stop the thread.

## How it works?
ESET developed code samples and a compiler that takes an "easm" file as input (which is a type of assembly language, as demonstrated in the examples) and produces a compiled output with the ".evm" extension.

The emulator takes an "evm" file as input and emulates it according to the specifications. The expected output for each code sample is outlined in the **samples/samples.txt** file.

The VM reads the entire input file and creates code sections as described in the documentation: one section for the code, another for the data, and a file header. 
The boot process starts creating the main thread. Each new thread is created using the `pthread_create()` function, with its own private context (registers). Each thread independently decodes and executes the code without waiting for other threads.

## Examples

### `samples/fibonacci_loop.easm`

```
.dataSize 0
.code

loadConst 0, r1 # first
loadConst 1, r2 # second

loadConst 1, r14 # loop helper

consoleRead r3

loop:
	jumpEqual end, r3, r15

	add r1, r2, r4
	mov r2, r1
	mov r4, r2
	
	consoleWrite r1
	
	sub r3, r14, r3
	jump loop
end:
	hlt
```

Output using the disassembler:<br>
![Istantanea_2024-11-19_17-45-38](https://github.com/user-attachments/assets/c1478466-c4e8-4f5a-80cd-32056399c590)

Note: the last instruction is wrongly decoded because of the bug mentioned on top.
It has been compiled with:
```
5 #define DEBUG_PRINT_INSTR
12 #define ESETVM2_DISASSEMBLY
```

Emulation, commenting the above lines:<br>
![Istantanea_2024-11-19_17-49-14](https://github.com/user-attachments/assets/5ca8b229-6b3f-4972-8954-44d5ea2d931c)

### Self made example: `thread.evm`

```
  1 .dataSize 18
  2 .code     
  3 
  4 loadConst 1, r1
  5 createThread threadProc, r0
  6 createThread threadProcTest, r15
  7 
  8 loadConst 75, r14          
  9 sleep r14                  
 10 
 11 loadConst 8, r1            
 12 loadConst 0x34, r2         
 13 mov r2, qword[r1]          
 14 
 15 consoleWrite qword[r1]     
 16 
 17 joinThread r0              
 18 joinThread r15             
 19 
 20 hlt
 21 
 22 threadProc:
 23     loadConst 0x12, r2
 24 
 25     loadConst 150, r3
 26     sleep r3
 27 
 28     mov r2, qword[r1]
 29     consoleWrite qword[r1]
 30 
 31     hlt
 32 
 33 threadProcTest:
 34     loadConst 25, r10
 35     loadConst 0x7788, r7
 36     
 37     loadConst 100, r8
 38     sleep r8
 39 
 40     mov r7, dword[r10]
 41     consoleWrite dword[r10]
 42 
 43     hlt
```

Multiple executions in order to see changes in the output (because of multi-threading):<br>
![Istantanea_2024-11-19_17-57-01](https://github.com/user-attachments/assets/5f91678d-2dba-4893-a082-ff7b5d8b90e6)

