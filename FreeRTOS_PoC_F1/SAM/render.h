#ifndef RENDER_H
#define RENDER_H

#include <stddef.h>

void Render();
void SetMouthThroat(unsigned char mouth, unsigned char throat);
void RenderResetStream(void);
int RenderNext(void);
size_t RenderReadSamples(unsigned char *pcm, size_t capacity);
void RenderFlushFinalSamples(void);

#endif
