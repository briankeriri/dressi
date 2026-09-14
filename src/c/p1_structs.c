#include "types.h"
#include "variable.h"
#include "function.h"

int dressi_structs_ok(void) {
    return (int)(sizeof(Variable) + sizeof(Function) + sizeof(ImgSize));
}
