#ifndef DRESSI_C_VARIABLE_H
#define DRESSI_C_VARIABLE_H

#include <stdint.h>
#include "types.h"

typedef struct Function Function;
typedef struct DressiAD DressiAD;
typedef struct Variable Variable;

#define DRESSI_MAX_USERS 8

struct Variable {
    uint64_t id;
    VType vtype;
    ImgSize size;
    Function *creator; /* paper m_creator; Empty = NULL */
    Function *users[DRESSI_MAX_USERS];
    uint32_t n_users;
    int requires_grad; /* paper m_req_grad; 0/1 */
    int is_dirty;      /* paper m_is_dirty; 0/1 */
    DressiAD *ad;
    float host_val;
    int has_host_val;
};

#endif /* DRESSI_C_VARIABLE_H */
