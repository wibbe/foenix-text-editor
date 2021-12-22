
#include <stdint.h>

static uint8_t *_heap;
static uint8_t *_heap_end;

static uint8_t *_heap_ptr;

void mem_init(uint8_t *heap, uint8_t *heap_end)
{
	_heap = heap;
	_heap_end = heap_end;
	_heap_ptr = _heap;
}

void *mem_alloc(unsigned long size)
{
	size = ((size / 4) + 1) * 4;
	if ((_heap_ptr + size) >= _heap_end)
		return 0;

	void *ptr = _heap_ptr;
	_heap_ptr = _heap_ptr + size;
	return ptr;
}

void mem_reset(void)
{
	_heap_ptr =_heap;
}

unsigned long mem_free(void)
{
	return (unsigned long)(_heap_end - _heap_ptr);
}
