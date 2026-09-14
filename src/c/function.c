#include "function.h"
#include "dressi_ad.h"
#include <stddef.h>

Variable *dressi_build_fwd(Function *fn, Variable **xs, uint32_t n) {
    if (!fn || !fn->ad) {
        return NULL;
    }
    if (n > DRESSI_MAX_XS) {
        return NULL;
    }
    if (n > 0 && !xs) {
        return NULL;
    }
    fn->n_xs = n;
    for (uint32_t i = 0; i < n; i++) {
        if (!xs[i]) {
            return NULL;
        }
        if (xs[i]->n_users >= DRESSI_MAX_USERS) {
            return NULL;
        }
        fn->xs[i] = xs[i];
        xs[i]->users[xs[i]->n_users++] = fn;
    }
    VType vt = FLOAT;
    ImgSize sz;
    sz.w = 1;
    sz.h = 1;
    if (n > 0) {
        vt = xs[0]->vtype;
        sz = xs[0]->size;
    }
    Variable *y = dressi_create_variable(fn->ad, vt, sz);
    if (!y) {
        return NULL;
    }
    y->creator = fn;
    fn->y = y;
    return y;
}

Variable *dressi_build_bwd(Function *fn, Variable *gy, uint32_t bwd_idx) {
    if (!fn || !fn->bwd_fn) {
        return NULL;
    }
    return fn->bwd_fn(fn, gy, bwd_idx);
}
