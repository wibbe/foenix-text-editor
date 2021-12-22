
#ifndef MEM_H
#define MEM_H

void *mem_alloc(unsigned long size);
void mem_reset(void);

unsigned long mem_free(void);

#endif