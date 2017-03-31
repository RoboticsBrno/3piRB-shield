#ifndef NEW_H_
#define NEW_H_

size_t total_allocated_memory = 0;

void * operator new(size_t size)
{
	total_allocated_memory += size;
	return malloc(size);
}

void operator delete(void * ptr)
{
	free(ptr);
}


#endif /* NEW_H_ */