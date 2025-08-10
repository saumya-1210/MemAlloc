#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

typedef char ALIGN[16];

union header {
	struct {
		size_t size;
		unsigned short int is_free;
		union header* next;
	} s;

	ALIGN stub;
};

typedef union header header_t;

header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_malloc_lock = PTHREAD_MUTEX_INITIALIZER;

header_t* get_free_block(size_t size) {
	header_t* curr = head;
	while (curr) {
		if (curr->s.is_free && curr->s.size >= size)
			return curr;

		curr = curr->s.next;
	}

	return NULL;
}

void memfree(void* block) {
	header_t* header;
	header_t* tmp;
	void* programbreak;

	if (!block)
		return;

	pthread_mutex_lock(&global_malloc_lock);
	header = (header_t*)block - 1;

	//programbreak receives current program break address
	programbreak = sbrk(0);
	if ((char*)block + header->s.size == programbreak) {
		if (head == tail) {
			head = tail = NULL;
		}
		else  {
			tmp = head;
			while (tmp) {
				if (tmp->s.next == tail) {
					tmp->s.next = NULL;
					tail = tmp;
				}
				tmp = tmp->s.next;
			}
		}
		sbrk(0 - header->s.size - sizeof(header_t));
		pthread_mutex_unlock(&global_malloc_lock);
		return;
	}

	header->s.is_free = 1;
	pthread_mutex_unlock(&global_malloc_lock);
}

void* memalloc(size_t size) {
	size_t total_size;
	void* block;
	header_t* header;

	if (!size)
		return NULL;

	pthread_mutex_lock(&global_malloc_lock);
	header = get_free_block(size);
	if (header) {
		header->s.is_free = 0;
		pthread_mutex_unlock(&global_malloc_lock);

		return (void*)(header + 1);
	}

	total_size = sizeof(header_t) + size;
	block = sbrk(total_size);
	if (block == (void*) -1) {
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}

	header = (header_t*)block;
	header->s.size = size;
	header->s.is_free = 0;
	header->s.next = NULL;
	if (!head) {
		head = header;
	}
	if (tail) {
		tail->s.next = header;
	}
	tail = header;
	pthread_mutex_unlock(&global_malloc_lock);

	return (void*)(header + 1);
}

void* memcalloc(size_t num, size_t nsize) {
	size_t size;
	void* block;

	if (!num || !nsize) {
		return NULL;
	}

	size = num * nsize;
	// checking for mult overflow
	if (nsize != size / num) {
		return NULL;
	}

	block = memalloc(size);
	if (!block) {
		return NULL;
	}
	memset(block, 0, size);

	return block;
}

void* memrealloc(void* block, size_t size) {
	header_t* header;
	void* ret;

	if (!block || !size) {
		return memalloc(size);
	}

	header = (header_t*)block - 1;
	if (header->s.size >= size) {
		return block;
	}

	ret = memalloc(size);
	if (ret) {
		memcpy(ret, block, header->s.size);
		memfree(block);
	}

	return ret;
}

void print_mem_list() {
	header_t* curr = head;
	printf("Memory list:\n");
	printf("head = %p, tail = %p\n", (void*)head, (void*)tail);

	while (curr) {
		printf("Addr = %p, Size = %zu, Decimal block size = %zu, Hexadecimal block size = %x, Free = %u, next = %p\n",
		(void*)curr, curr->s.size, sizeof(header_t) + curr->s.size,
		sizeof(header_t) + curr->s.size, curr->s.is_free, (void*)curr->s.next);

		curr = curr->s.next;
	}
}
