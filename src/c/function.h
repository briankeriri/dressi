#ifndef DRESSI_C_FUNCTION_H
#define DRESSI_C_FUNCTION_H

#include <stdint.h>
#include "types.h"
#include "variable.h"

typedef struct Function Function;

/* Paper BwdFunc = Variable(xs, y, gy, bwd_idx). C: self holds xs and y. */
typedef struct Variable *(*DressiBwdFn)(struct Function *fn, struct Variable *gy,
                                       uint32_t bwd_idx);

#define DRESSI_MAX_XS 4

struct Function {
    uint64_t id;
    char fwd_buf[64];
    const char *fwd_code; /* paper m_fwd_code; points at fwd_buf */
    ShaderType shader_type; /* paper m_type */
    DressiBwdFn bwd_fn;     /* paper m_bwd_func; NULL = no backward */
    Variable *xs[DRESSI_MAX_XS];
    uint32_t n_xs;
    Variable *y;
    DressiAD *ad;
    float const_f;
    int has_const;
};

#endif /* DRESSI_C_FUNCTION_H */
