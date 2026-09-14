#include "dressi_build_backward.h"

#include "dressi_ad.h"
#include "dressi_ops.h"
#include "function.h"

#include <stdio.h>
#include <string.h>

#define DRESSI_BWD_MAP 64

typedef struct FwdBwdEntry {
    Variable *fwd;
    Variable *gxs[DRESSI_MAX_XS];
    uint32_t n_gxs;
} FwdBwdEntry;

static Variable *dressi_float_bwd(Function *fn, Variable *gy, uint32_t bwd_idx) {
    (void)fn;
    (void)gy;
    (void)bwd_idx;
    return NULL;
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

Variable *dressi_float(float fv) {
    DressiAD *ad = dressi_active_ad();
    if (!ad) {
        return NULL;
    }
    ImgSize sz;
    sz.w = 1;
    sz.h = 1;
    Variable *out = dressi_create_variable(ad, FLOAT, sz);
    if (!out) {
        return NULL;
    }
    out->host_val = fv;
    out->has_host_val = 1;
    if (ad->n_funcs >= DRESSI_POOL_CAP) {
        return NULL;
    }
    Function *fn = &ad->funcs[ad->n_funcs++];
    memset(fn, 0, sizeof(*fn));
    fn->id = ad->next_id++;
    {
        char tmp[64];
        (void)snprintf(tmp, sizeof(tmp), "{y}=float(%.9g);", (double)fv);
        dressi_copy_fwd(fn, tmp);
    }
    fn->shader_type = FRAG;
    fn->bwd_fn = dressi_float_bwd;
    fn->n_xs = 0;
    fn->y = out;
    fn->ad = ad;
    fn->const_f = fv;
    fn->has_const = 1;
    out->creator = fn;
    return out;
}

static FwdBwdEntry *map_find(FwdBwdEntry *map, uint32_t n, Variable *v) {
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (map[i].fwd == v) {
            return &map[i];
        }
    }
    return NULL;
}

static FwdBwdEntry *map_get(FwdBwdEntry *map, uint32_t *n, Variable *v) {
    FwdBwdEntry *e = map_find(map, *n, v);
    if (e) {
        return e;
    }
    if (*n >= DRESSI_BWD_MAP) {
        return NULL;
    }
    e = &map[*n];
    (*n)++;
    e->fwd = v;
    e->n_gxs = 0;
    return e;
}

static int map_erase(FwdBwdEntry *map, uint32_t *n, Variable *v) {
    uint32_t i;
    for (i = 0; i < *n; i++) {
        if (map[i].fwd == v) {
            map[i] = map[*n - 1];
            (*n)--;
            return 1;
        }
    }
    return 0;
}

static Variable *sum_contribs(Variable **gxs, uint32_t n) {
    uint32_t i;
    Variable *acc;
    if (n == 0) {
        return NULL;
    }
    acc = gxs[0];
    for (i = 1; i < n; i++) {
        acc = dressi_add(acc, gxs[i]);
        if (!acc) {
            return NULL;
        }
    }
    return acc;
}

static int seen_func(uint64_t *seen, uint32_t n_seen, uint64_t id) {
    uint32_t i;
    for (i = 0; i < n_seen; i++) {
        if (seen[i] == id) {
            return 1;
        }
    }
    return 0;
}

static Function *q_pop_max(Function **q, uint32_t *n) {
    uint32_t i, best;
    Function *f;
    if (*n == 0) {
        return NULL;
    }
    best = 0;
    for (i = 1; i < *n; i++) {
        if (q[i]->id > q[best]->id) {
            best = i;
        }
    }
    f = q[best];
    q[best] = q[*n - 1];
    (*n)--;
    return f;
}

int dressi_build_backward(Variable *loss, DressiBwdResult *out) {
    FwdBwdEntry map[DRESSI_BWD_MAP];
    uint32_t n_map = 0;
    Function *queue[DRESSI_BWD_MAP];
    uint32_t nq = 0;
    uint64_t seen[DRESSI_BWD_MAP];
    uint32_t n_seen = 0;
    FwdBwdEntry *e;
    Variable *seed;
    uint32_t i;

    if (!loss || !out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    seed = dressi_float(1.f);
    if (!seed) {
        return -1;
    }
    e = map_get(map, &n_map, loss);
    if (!e || e->n_gxs >= DRESSI_MAX_XS) {
        return -1;
    }
    e->gxs[e->n_gxs++] = seed;

    if (loss->creator) {
        if (nq >= DRESSI_BWD_MAP) {
            return -1;
        }
        queue[nq++] = loss->creator;
    }

    while (nq > 0) {
        Function *func = q_pop_max(queue, &nq);
        Variable *y;
        Variable *gy;
        uint32_t x_idx;
        if (!func) {
            continue;
        }
        if (seen_func(seen, n_seen, func->id)) {
            continue;
        }
        if (n_seen >= DRESSI_BWD_MAP) {
            return -1;
        }
        seen[n_seen++] = func->id;

        y = func->y;
        e = map_find(map, n_map, y);
        if (!e) {
            continue;
        }
        gy = sum_contribs(e->gxs, e->n_gxs);
        map_erase(map, &n_map, y);
        if (!gy) {
            continue;
        }

        for (x_idx = 0; x_idx < func->n_xs; x_idx++) {
            Variable *x = func->xs[x_idx];
            Variable *gx;
            FwdBwdEntry *xe;
            if (!x || !x->requires_grad) {
                continue;
            }
            gx = dressi_build_bwd(func, gy, x_idx);
            if (!gx) {
                continue; /* this-repo: paper wrote if (!gy) */
            }
            xe = map_get(map, &n_map, x);
            if (!xe || xe->n_gxs >= DRESSI_MAX_XS) {
                return -1;
            }
            xe->gxs[xe->n_gxs++] = gx;
            if (x->creator) {
                if (nq >= DRESSI_BWD_MAP) {
                    return -1;
                }
                queue[nq++] = x->creator;
            }
        }
    }

    for (i = 0; i < n_map; i++) {
        Variable *g;
        if (map[i].fwd == loss) {
            continue;
        }
        if (out->n >= DRESSI_BWD_MAX) {
            return -1;
        }
        g = sum_contribs(map[i].gxs, map[i].n_gxs);
        if (!g) {
            return -1;
        }
        out->inputs[out->n] = map[i].fwd;
        out->grads[out->n] = g;
        out->n++;
    }
    return 0;
}
