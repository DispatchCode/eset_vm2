#ifndef CONFIG_H
#define CONFIG_H

/* Print disassembly */
//#define DEBUG_PRINT_INSTR

/* Print internal format */
//#define DEBUG_INSTR_FORMAT

/* Enable disassembly mode (no exec)*/

#ifdef DEBUG_PRINT_INSTR
	#define ESETVM2_DISASSEMBLY
#endif

/* Print CPU registers content */
//#define VM_PRINT_STATE

#endif
