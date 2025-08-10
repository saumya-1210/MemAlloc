#include "memallocator.h"

int main() {
    printf("=== TEST START ===\n");
	int *arr = (int*)memalloc(10 * sizeof(int));
	int *arr2 = (int*)memalloc(20 * sizeof(int));
    if (!arr || !arr2) {
        printf("Memory allocation failed\n");
        return 1;
    }
	printf("Array 1:\n");
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 2;
		printf("%d ", arr[i]);
	}
	printf("\n\nArray 2:\n");
	for (int i = 0; i < 20; ++i) {
		arr2[i] = i + 1;
		printf("%d ", arr2[i]);
	}
	printf("\n\n");
    print_mem_list();

	printf("\nMemory is freed\n");
    memfree(arr2);
	memfree(arr);
    print_mem_list();

    return 0;
}

