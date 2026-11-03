#ifndef SAM_H
#define SAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
	SAM_STATUS_MORE,
	SAM_STATUS_DONE,
	SAM_STATUS_ERROR
} SAM_Status;

bool SAM_Begin(const char *text);
SAM_Status SAM_GenerateChunk(uint8_t *pcm, size_t capacity, size_t *produced);
void SAM_Cancel(void);

#endif
