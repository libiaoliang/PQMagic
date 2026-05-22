#ifndef BSL_PARAMS_H
#define BSL_PARAMS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSL_PARAM_END {0, 0, NULL, 0, 0}

typedef enum {
    BSL_PARAM_TYPE_UINT32_PTR,
    BSL_PARAM_TYPE_OCTETS_PTR,
    BSL_PARAM_TYPE_FUNC_PTR,
    BSL_PARAM_TYPE_CTX_PTR,
    BSL_PARAM_TYPE_UINT8,
    BSL_PARAM_TYPE_UINT16,
    BSL_PARAM_TYPE_UINT32,
    BSL_PARAM_TYPE_BOOL,
    BSL_PARAM_TYPE_INT32,
    BSL_PARAM_TYPE_OCTETS,
    BSL_PARAM_TYPE_UTF8_STR,
    BSL_PARAM_TYPE_SIZE_T,
    BSL_PARAM_TYPE_SIZE_T_PTR,
    BSL_PARAM_TYPE_UINT64,
} BSL_PARAM_VALUE_TYPE;

typedef struct BslParam {
    int32_t key;
    uint32_t valueType;
    void *value;
    uint32_t valueLen;
    uint32_t useLen;
} BSL_Param;

#ifdef __cplusplus
}
#endif

#endif

