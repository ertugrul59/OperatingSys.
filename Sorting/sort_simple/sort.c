#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int my_sort(const void * a, const void * b) {
	const char *pa = *(const char**)a;
	const char *pb = *(const char**)b;

	return strcmp(pa,pb);
}

int main(int argc, char ** argv) {
	char ** lines = NULL;
	int lines_read = 0;
	int capacity = 4;
	int i;
	size_t bytes_read = 0;
	ssize_t result;
	lines = (char**) malloc(sizeof(char*)*capacity);
	if (lines == NULL) {
		// fprintf(stderr, "Could not allocate lines\n");
		return 1;
	}
	lines[0] = NULL;
	result = getline(&lines[0], &bytes_read, stdin);
	while(result != -1) {
		if (lines[lines_read][result-1] == '\n') {
			lines[lines_read][result-1] = 0;
		}
		lines_read++;
		if(lines_read == capacity) {
			capacity = capacity<<1;
			lines = realloc(lines, sizeof(char*)*capacity);
			if (lines == NULL) {
				// fprintf(stderr, "realloc failed\n");
				return 1;
			}
		}
		lines[lines_read] = NULL;
		bytes_read = 0;
		result = getline(&lines[lines_read], &bytes_read, stdin);
	}
	if (errno == ENOMEM) {
		// fprintf(stderr, "failed with ENOMEM\n");
		return 1;
	}
	qsort(lines, lines_read, sizeof(char*), my_sort);
	for (i = 0; i < lines_read; i++) {
		printf("%s\n", lines[i]);
	}
	return 0;
}
	
	
