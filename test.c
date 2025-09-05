#include <stdio.h>
#include <string.h>
#include "arenas.h"

int main(void) {
	VMemArena arena;
	if (!vmem_arena_init(&arena, 4 * 1024 * 1024)) {
		fprintf(stderr, "couldn't allocate memory for arena\n");
		return 1;
	}
	char * a = vmem_arena_alloc_n(&arena, char, 100);
	if (!a) {
		fprintf(stderr, "allocation failure\n");
		vmem_arena_free(&arena);
		return 1;
	}
	const char path[] = "The cake is a lie!";
	memcpy(a, path, sizeof(path) - 1);
	puts(a);
	vmem_arena_free(&arena);
}
