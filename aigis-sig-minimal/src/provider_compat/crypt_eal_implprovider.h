#ifndef CRYPT_EAL_IMPLPROVIDER_H
#define CRYPT_EAL_IMPLPROVIDER_H

#include <stdint.h>
#include "bsl_params.h"
#include "crypt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRYPT_EAL_FUNC_END {0, NULL}
#define CRYPT_EAL_ALGINFO_END {0, NULL, NULL}

typedef struct {
    int32_t algId;
    const CRYPT_EAL_Func *implFunc;
    const char *attr;
} CRYPT_EAL_AlgInfo;

#define CRYPT_EAL_PROVCB_FREE 1
#define CRYPT_EAL_PROVCB_QUERY 2
#define CRYPT_EAL_PROVCB_CTRL 3
#define CRYPT_EAL_PROVCB_GETCAPS 4

#define CRYPT_EAL_OPERAID_SYMMCIPHER 1
#define CRYPT_EAL_OPERAID_KEYMGMT 2
#define CRYPT_EAL_OPERAID_SIGN 3
#define CRYPT_EAL_OPERAID_ASYMCIPHER 4
#define CRYPT_EAL_OPERAID_KEYEXCH 5
#define CRYPT_EAL_OPERAID_KEM 6
#define CRYPT_EAL_OPERAID_HASH 7
#define CRYPT_EAL_OPERAID_MAC 8
#define CRYPT_EAL_OPERAID_KDF 9
#define CRYPT_EAL_OPERAID_RAND 10
#define CRYPT_EAL_OPERAID_DECODER 11
#define CRYPT_EAL_OPERAID_SELFTEST 12

typedef int32_t (*CRYPT_EAL_ProvQueryCb)(void *provCtx, int32_t operaId, CRYPT_EAL_AlgInfo **algInfos);

typedef int32_t (*CRYPT_EAL_ImplProviderInit)(CRYPT_EAL_ProvMgrCtx *mgrCtx, BSL_Param *param,
    CRYPT_EAL_Func *capFuncs, CRYPT_EAL_Func **outFuncs, void **provCtx);

#define CRYPT_EAL_IMPLPKEYMGMT_NEWCTX 1
#define CRYPT_EAL_IMPLPKEYMGMT_SETPARAM 2
#define CRYPT_EAL_IMPLPKEYMGMT_GETPARAM 3
#define CRYPT_EAL_IMPLPKEYMGMT_GENKEY 4
#define CRYPT_EAL_IMPLPKEYMGMT_SETPRV 5
#define CRYPT_EAL_IMPLPKEYMGMT_SETPUB 6
#define CRYPT_EAL_IMPLPKEYMGMT_GETPRV 7
#define CRYPT_EAL_IMPLPKEYMGMT_GETPUB 8
#define CRYPT_EAL_IMPLPKEYMGMT_DUPCTX 9
#define CRYPT_EAL_IMPLPKEYMGMT_CHECK 10
#define CRYPT_EAL_IMPLPKEYMGMT_COMPARE 11
#define CRYPT_EAL_IMPLPKEYMGMT_CTRL 12
#define CRYPT_EAL_IMPLPKEYMGMT_FREECTX 13
#define CRYPT_EAL_IMPLPKEYMGMT_COPYPARAM 14
#define CRYPT_EAL_IMPLPKEYMGMT_IMPORT 15
#define CRYPT_EAL_IMPLPKEYMGMT_EXPORT 16

typedef void *(*CRYPT_EAL_ImplPkeyMgmtNewCtx)(void *provCtx, int32_t algId);
typedef int32_t (*CRYPT_EAL_ImplPkeyMgmtGenKey)(void *ctx);
typedef int32_t (*CRYPT_EAL_ImplPkeyMgmtSetPrv)(void *ctx, const BSL_Param *param);
typedef int32_t (*CRYPT_EAL_ImplPkeyMgmtSetPub)(void *ctx, const BSL_Param *param);
typedef int32_t (*CRYPT_EAL_ImplPkeyMgmtGetPrv)(const void *ctx, BSL_Param *param);
typedef int32_t (*CRYPT_EAL_ImplPkeyMgmtGetPub)(const void *ctx, BSL_Param *param);
typedef void (*CRYPT_EAL_ImplPkeyMgmtFreeCtx)(void *ctx);

#define CRYPT_EAL_IMPLPKEYSIGN_SIGN 1
#define CRYPT_EAL_IMPLPKEYSIGN_SIGNDATA 2
#define CRYPT_EAL_IMPLPKEYSIGN_VERIFY 3
#define CRYPT_EAL_IMPLPKEYSIGN_VERIFYDATA 4
#define CRYPT_EAL_IMPLPKEYSIGN_RECOVER 5
#define CRYPT_EAL_IMPLPKEYSIGN_BLIND 6
#define CRYPT_EAL_IMPLPKEYSIGN_UNBLIND 7

typedef int32_t (*CRYPT_EAL_ImplPkeySign)(void *ctx, int32_t mdAlgId, const uint8_t *data, uint32_t dataLen,
    uint8_t *sign, uint32_t *signLen);
typedef int32_t (*CRYPT_EAL_ImplPkeySignData)(void *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t *sign, uint32_t *signLen);
typedef int32_t (*CRYPT_EAL_ImplPkeyVerify)(const void *ctx, int32_t mdAlgId, const uint8_t *data, uint32_t dataLen,
    uint8_t *sign, uint32_t signLen);
typedef int32_t (*CRYPT_EAL_ImplPkeyVerifyData)(const void *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t *sign, uint32_t signLen);

#ifdef __cplusplus
}
#endif

#endif

