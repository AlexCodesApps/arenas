#include "arenas.h"

static bool add_size(size_t a, size_t b, size_t * out) {
	if (a > SIZE_MAX - b) {
		return false;
	}
	*out = a + b;
	return true;
}
static bool add_ptr(void * a, size_t b, void ** out) {
	uintptr_t _a = (uintptr_t)a;
	if (_a > SIZE_MAX - b) {
		return false;
	}
	*out = (void *)(_a + b);
	return true;
}

static bool align_size(size_t a, size_t b, size_t * out) {
	size_t mask = b - 1;
	size_t r;
	if (!add_size(a, mask, &r)) {
		return false;
	}
	*out = r & ~mask;
	return true;
}
static bool align_ptr(void * a, size_t b, void ** out) {
	size_t mask = b - 1;
	void * r;
	if (!add_ptr(a, mask, &r)) {
		return false;
	}
	*out = (void *)((uintptr_t)r & ~mask);
	return true;
}

static bool multiply_size(size_t a, size_t b, size_t * out) {
	if (a > SIZE_MAX / b) {
		return false;
	}
	*out = a * b;
	return true;
}

#ifdef _WIN32
#include <windows.h>
#define INVALID_PAGE NULL

static size_t get_page_size() {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwPageSize;
}
static void * allocate_pages(size_t size) {
	return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}
static bool commit_pages(void * start, size_t size) {
	return VirtualAlloc(start, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}
static void free_pages(void * start, size_t size) {
	VirtualFree(start, 0, MEM_RELEASE);
}
#else
#include <unistd.h>
#include <sys/mman.h>
#define INVALID_PAGE MAP_FAILED

static size_t get_page_size() {
	long pg_size = sysconf(_SC_PAGESIZE);
	if (pg_size == -1) {
		return 4096; // default
	}
	return pg_size;
}
static void * allocate_pages(size_t size) {
	return mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
}
static bool commit_pages(void * start, size_t size) {
	return mprotect(start, size, PROT_READ | PROT_WRITE) == 0;
}
static void free_pages(void * start, size_t size) {
	munmap(start, size);
}
#endif


void buffered_arena_init(BufferedArena * arena, void * start, size_t size) {
	arena->begin = start;
	arena->end = (uint8_t *)start + size;
	arena->current = start;
}
void * buffered_arena_alloc_bytes(BufferedArena * arena, size_t size, size_t align) {
	void * alloc_start, * current;
	bool ok = align_ptr(arena->begin, align, &alloc_start);
	ok &= add_ptr(alloc_start, size, &current);
	ok &= current < arena->end;
	if (!ok) {
		return NULL;
	}
	arena->current = current;
	return alloc_start;
}
void * buffered_arena_alloc_bytes_n(BufferedArena * arena, size_t size, size_t align, size_t n) {
	if (!multiply_size(size, n, &size)) {
		return NULL;
	}
	return buffered_arena_alloc_bytes(arena, size, align);
}
void buffered_arena_reset(BufferedArena * arena) {
	arena->current = arena->begin;
}

bool vmem_arena_init(VMemArena * arena, size_t size) {
	size_t pg_size = get_page_size();
	if (!align_size(size, pg_size, &size)) {
		return false;
	}
	void * ptr = allocate_pages(size);
	if (ptr == INVALID_PAGE) {
		return false;
	}
	arena->begin = ptr;
	arena->end = (uint8_t *)ptr + size;
	arena->current = ptr;
	arena->commit = ptr;
	return true;
}
void * vmem_arena_alloc_bytes(VMemArena * arena, size_t size, size_t align) {
	void * alloc_start, * current;
	bool ok = align_ptr(arena->begin, align, &alloc_start);
	ok &= add_ptr(alloc_start, size, &current);
	ok &= current < arena->end;
	void * commit = arena->current;
	if (commit < current) {
		size_t pg_size = get_page_size();
		ok &= align_ptr(current, pg_size, &commit);
		if (!ok) {
			return NULL;
		}
		ok = commit_pages(arena->commit, (uintptr_t)commit - (uintptr_t)arena->commit);
	}
	if (!ok) {
		return NULL;
	}
	arena->current = current;
	arena->commit = commit;
	return alloc_start;
}
void * vmem_arena_alloc_bytes_n(VMemArena * arena, size_t size, size_t align, size_t n) {
	if (!multiply_size(size, n, &size)) {
		return NULL;
	}
	return vmem_arena_alloc_bytes(arena, size, align);
}
void vmem_arena_reset(VMemArena * arena) {
	arena->current = arena->begin;
}
void vmem_arena_free(VMemArena * arena) {
	free_pages(arena->begin, (uintptr_t)arena->end - (uintptr_t)arena->begin);
}
