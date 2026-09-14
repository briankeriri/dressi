#include "dressi_ad.h"

#include <stdlib.h>
#include <string.h>

static DressiAD *g_active_ad = NULL;

DressiAD *dressi_ad_create(void) {
    DressiAD *ad = (DressiAD *)calloc(1, sizeof(DressiAD));
    if (!ad) {
        return NULL;
    }
    ad->next_id = 1;
    g_active_ad = ad;
    return ad;
}

void dressi_ad_destroy(DressiAD *ad) {
    if (!ad) {
        return;
    }
    if (g_active_ad == ad) {
        g_active_ad = NULL;
    }
    free(ad);
}

DressiAD *dressi_active_ad(void) {
    return g_active_ad;
}

Variable *dressi_create_variable(DressiAD *ad, VType vtype, ImgSize size) {
    if (!ad) {
        return NULL;
    }
    if (ad->n_vars >= DRESSI_POOL_CAP) {
        return NULL;
    }
    Variable *v = &ad->vars[ad->n_vars++];
    memset(v, 0, sizeof(*v));
    v->id = ad->next_id++;
    v->vtype = vtype;
    v->size = size;
    v->is_dirty = 1;
    v->ad = ad;
    return v;
}

static Variable *dressi_add_bwd(Function *fn, Variable *gy, uint32_t bwd_idx) {
    (void)fn;
    (void)bwd_idx;
    return gy; /* paper A.4 Add: return gy; */
}

static void dressi_copy_fwd(Function *fn, const char *code) {
    size_t i = 0;
    while (code[i] != '\0' && i + 1 < sizeof(fn->fwd_buf)) {
        fn->fwd_buf[i] = code[i];
        i++;
    }
    fn->fwd_buf[i] = '\0';
    fn->fwd_code = fn->fwd_buf;
}

Function *dressi_create_add(DressiAD *ad, Variable *a, Variable *b, Variable *out) {
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
    dressi_copy_fwd(fn, "{y}={x0}+{x1};");
    fn->shader_type = FRAG;
    fn->bwd_fn = dressi_add_bwd;
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
