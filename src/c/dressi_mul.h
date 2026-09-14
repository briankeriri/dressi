#ifndef DRESSI_C_MUL_H
#define DRESSI_C_MUL_H

#include "dressi_ad.h"
#include "variable.h"

Function *dressi_create_mul(DressiAD *ad, Variable *a, Variable *b, Variable *out);
Variable *dressi_mul(Variable *a, Variable *b);

#endif /* DRESSI_C_MUL_H */
