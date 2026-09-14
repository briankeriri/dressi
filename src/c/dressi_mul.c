#include "dressi_mul.h"

#include <string.h>

static Variable *dressi_mul_bwd(Function *fn, Variable *gy, uint32_t bwd_idx);

static void dressi_copy_fwd(Function *fn, const char *code) {
    size_t i = 0;
    while (code[i] != '\0' && i + 1 < sizeof(fn->fwd_buf)) {
        fn->fwd_buf[i] = code[i];
        i++;
    }
    fn->fwd_buf[i] = '\0';
    fn->fwd_code = fn->fwd_buf;
}

Function *dressi_create_mul(DressiAD *ad, Variable *a, Variable *b, Variable *out) {
    if (!ad || !a || !b || !out) {
        return NULL;
    }
    if (ad->n_funcs >= DRESSI_POOL_CAP) {
        return NULL;
    }
    if (a->n_users >= DRESSI_MAX_USERS || b->n_users >= DRESSI_MAX_USERS) {
        return NULL;
    }
    Function *fn = &ad->funcs[ad->n_funcs++];
    memset(fn, 0, sizeof(*fn));
    fn->id = ad->next_id++;
    dressi_copy_fwd(fn, "{y}={x0}*{x1};");
    fn->shader_type = FRAG;
    fn->bwd_fn = dressi_mul_bwd;
    fn->xs[0] = a;
    fn->xs[1] = b;
    fn->n_xs = 2;
    fn->y = out;
    fn->ad = ad;
    a->users[a->n_users++] = fn;
    b->users[b->n_users++] = fn;
    out->creator = fn;
    return fn;
}

Variable *dressi_mul(Variable *a, Variable *b) {
    DressiAD *ad = dressi_active_ad();
    if (!ad) {
        return NULL;
    }
    if (!a || !b) {
        return NULL;
    }
    Variable *out = dressi_create_variable(ad, a->vtype, a->size);
    if (!out) {
        return NULL;
    }
    if (!dressi_create_mul(ad, a, b, out)) {
        return NULL;
    }
    return out;
}

static Variable *dressi_mul_bwd(Function *fn, Variable *gy, uint32_t bwd_idx) {
    if (!fn || fn->n_xs < 2) {
        return NULL;
    }
    /* paper A.5: bwd_idx==0 -> gy * xs[1]; else gy * xs[0] */
    Variable *other = (bwd_idx == 0) ? fn->xs[1] : fn->xs[0];
    return dressi_mul(gy, other);
}
