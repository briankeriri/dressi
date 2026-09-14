#ifndef DRESSI_C_BUILD_BACKWARD_H
#define DRESSI_C_BUILD_BACKWARD_H

#include "variable.h"

#define DRESSI_BWD_MAX 8

typedef struct DressiBwdResult {
    Variable *inputs[DRESSI_BWD_MAX];
    Variable *grads[DRESSI_BWD_MAX];
    uint32_t n;
} DressiBwdResult;

Variable *dressi_float(float fv);
int dressi_build_backward(Variable *loss, DressiBwdResult *out);

#endif /* DRESSI_C_BUILD_BACKWARD_H */
