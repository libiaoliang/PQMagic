#ifndef CRYPT_TYPES_H
#define CRYPT_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t id;
    void *func;
} CRYPT_EAL_Func;

typedef void CRYPT_EAL_ProvMgrCtx;

#ifdef __cplusplus
}
#endif

#endif

