#ifndef ARENAS_H
#define ARENAS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#define ARENAS_ALIGNOF(t) __alignof(t)
#elif defined(__GNUC__)
#define ARENAS_ALIGNOF(t) __alignof__(t)
#elif __STDC_VERSION__ >= 201112L
#define ARENAS_ALIGNOF(t) _Alignof(t)
#else
#define ARENAS_ALIGNOF(t) \
	((uintptr_t)&((struct { char _arenas_alignof_char_; t _arenas_alignof_payload_; } *)NULL)->_arenas_alignof_payload_)
	// alr deep in the weeds of UB
#endif

typedef struct {
	void * begin;
	void * end;
	void * current;
} BufferedArena;

void buffered_arena_init(BufferedArena * arena, void * start, size_t size);
void * buffered_arena_alloc_bytes(BufferedArena * arena, size_t size, size_t align);
void * buffered_arena_alloc_bytes_n(BufferedArena * arena, size_t size, size_t align, size_t n);
void buffered_arena_reset(BufferedArena * arena);

#define buffered_arena_alloc(arena, type) \
	((type *)buffered_arena_alloc_bytes((arena), sizeof(type), ARENAS_ALIGNOF(type)))
#define buffered_arena_alloc_n(arena, type, n) \
	((type *)buffered_arena_alloc_bytes_n((arena), sizeof(type), ARENAS_ALIGNOF(type), (n)))

typedef struct {
	void * begin;
	void * end;
	void * current;
	void * commit;
} VMemArena;

bool vmem_arena_init(VMemArena * arena, size_t size);
void * vmem_arena_alloc_bytes(VMemArena * arena, size_t size, size_t align);
void * vmem_arena_alloc_bytes_n(VMemArena * arena, size_t size, size_t align, size_t n);
void vmem_arena_reset(VMemArena * arena);
void vmem_arena_free(VMemArena * arena);

#define vmem_arena_alloc(arena, type) \
	((type *)vmem_arena_alloc_bytes((arena), sizeof(type), ARENAS_ALIGNOF(type)))
#define vmem_arena_alloc_n(arena, type, n) \
	((type *)vmem_arena_alloc_bytes_n((arena), sizeof(type), ARENAS_ALIGNOF(type), (n)))

#endif // ARENAS_H
