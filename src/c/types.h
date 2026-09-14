#ifndef DRESSI_C_TYPES_H
#define DRESSI_C_TYPES_H

#include <stdint.h>

/* Paper A.4 enum VType { FLOAT, VEC2, ..., MAT2, ..., INT, IVEC2, ... };
   Ellipsis filled from include/dressi/types.h. Unprefixed FLOAT is the paper name. */
typedef enum VType {
    FLOAT = 0,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
    INT,
    IVEC2,
    IVEC3,
    IVEC4,
    UINT
} VType;

/* Paper A.4 ImgSize img_size = {1, 1}. Fields from include/dressi/types.h. */
typedef struct ImgSize {
    uint32_t w;
    uint32_t h;
} ImgSize;

/* Paper A.4 enum ShaderType { FRAG, COMP, RASTER }; */
typedef enum ShaderType {
    FRAG = 0,
    COMP,
    RASTER
} ShaderType;

#endif /* DRESSI_C_TYPES_H */
