#include "dressi_ad.h"
#include "dressi_build_backward.h"
#include "dressi_mul.h"
#include "dressi_ops.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int streq(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }
    return strcmp(a, b) == 0;
}

static float eval_var(Variable *v) {
    if (!v) {
        return 0.f;
    }
    if (v->creator && v->creator->has_const) {
        return v->creator->const_f;
    }
    if (v->creator && streq(v->creator->fwd_code, "{y}={x0}*{x1};")) {
        return eval_var(v->creator->xs[0]) * eval_var(v->creator->xs[1]);
    }
    if (v->creator && streq(v->creator->fwd_code, "{y}={x0}+{x1};")) {
        return eval_var(v->creator->xs[0]) + eval_var(v->creator->xs[1]);
    }
    if (v->has_host_val) {
        return v->host_val;
    }
    return 0.f;
}

static Variable *grad_of(DressiBwdResult *r, Variable *x) {
    uint32_t i;
    for (i = 0; i < r->n; i++) {
        if (r->inputs[i] == x) {
            return r->grads[i];
        }
    }
    return NULL;
}

int main(void) {
    DressiAD *ad = dressi_ad_create();
    ImgSize sz;
    Variable *x0, *x1, *y, *c, *y_add, *gx0, *gx1;
    DressiBwdResult r;
    sz.w = 1;
    sz.h = 1;
    if (!ad) {
        fprintf(stderr, "ad NULL\n");
        return 1;
    }
    x0 = dressi_create_variable(ad, FLOAT, sz);
    x1 = dressi_create_variable(ad, FLOAT, sz);
    if (!x0 || !x1) {
        fprintf(stderr, "leaves NULL\n");
        return 1;
    }
    x0->requires_grad = 1;
    x1->requires_grad = 1;
    x0->host_val = 2.f;
    x0->has_host_val = 1;
    x1->host_val = 3.f;
    x1->has_host_val = 1;

    y = dressi_mul(x0, x1);
    if (!y) {
        fprintf(stderr, "mul NULL\n");
        return 1;
    }
    c = dressi_float(1.f);
    y_add = dressi_add(y, c);
    if (!c || !y_add) {
        fprintf(stderr, "add(mul,c) NULL\n");
        return 1;
    }

    if (dressi_build_backward(y, &r) != 0) {
        fprintf(stderr, "build_backward failed\n");
        return 1;
    }
    if (r.n != 2) {
        fprintf(stderr, "n=%u want 2\n", r.n);
        return 1;
    }
    gx0 = grad_of(&r, x0);
    gx1 = grad_of(&r, x1);
    if (!gx0 || !gx1) {
        fprintf(stderr, "missing grad\n");
        return 1;
    }
    /* d(x0*x1)/dx0 = x1 = 3; d(x0*x1)/dx1 = x0 = 2; seed is 1 */
    if (fabsf(eval_var(gx0) - 3.f) > 1e-6f) {
        fprintf(stderr, "gx0=%g want 3\n", (double)eval_var(gx0));
        return 1;
    }
    if (fabsf(eval_var(gx1) - 2.f) > 1e-6f) {
        fprintf(stderr, "gx1=%g want 2\n", (double)eval_var(gx1));
        return 1;
    }
    dressi_ad_destroy(ad);
    printf("ok\n");
    return 0;
}
