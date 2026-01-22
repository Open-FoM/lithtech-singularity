#ifndef LTJS_DILIGENT_RENDERER_LIFECYCLE_H
#define LTJS_DILIGENT_RENDERER_LIFECYCLE_H

struct RenderStructInit;

int diligent_Init(RenderStructInit* init);
void diligent_Term(bool full);

#endif
