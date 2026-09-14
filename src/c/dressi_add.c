#include "dressi_ops.h"
#include "dressi_ad.h"
#include <stddef.h>

Variable *dressi_add(Variable *a, Variable *b) {
    DressiAD *ad = dressi_active_ad();
    if (!ad) {
        return NULL; /* C: no exceptions. ASSUMPTION: C++ threw; we return NULL. */
    }
    if (!a || !b) {
        return NULL;
    }
    Variable *out = dressi_create_variable(ad, a->vtype, a->size);
    if (!out) {
        return NULL;
    }
    if (!dressi_create_add(ad, a, b, out)) {
        return NULL;
    }
    return out;
}
