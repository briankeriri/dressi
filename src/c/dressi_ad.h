#ifndef DRESSI_C_AD_H
#define DRESSI_C_AD_H

#include "function.h"
#include "variable.h"

#define DRESSI_POOL_CAP 64

struct DressiAD {
    Variable vars[DRESSI_POOL_CAP];
    Function funcs[DRESSI_POOL_CAP];
    uint32_t n_vars;
    uint32_t n_funcs;
    uint64_t next_id;
};

DressiAD *dressi_ad_create(void);
void dressi_ad_destroy(DressiAD *ad);
DressiAD *dressi_active_ad(void);

Variable *dressi_create_variable(DressiAD *ad, VType vtype, ImgSize size);
Function *dressi_create_add(DressiAD *ad, Variable *a, Variable *b, Variable *out);

#endif /* DRESSI_C_AD_H */
