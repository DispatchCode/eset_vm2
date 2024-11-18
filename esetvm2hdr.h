#ifndef ESETVM2_HDR
#define ESETVM2_HDR

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#define MAGIC "ESET-VM2"

#define CODE_OFFSET 0x14
#define CODE_OFFSET_BIT  (CODE_OFFSET * 8)

struct esetvm2hdr
{
	uint8_t  magic[8];
	uint32_t code_size;         // bytes
	uint32_t data_size;         // bytes
	uint32_t initial_data_size;	// bytes
};

bool is_header_valid();
uint32_t code_size();
uint32_t data_size();

int file_size(FILE *fp);
void print_task_hdr(struct esetvm2hdr *);
#endif
