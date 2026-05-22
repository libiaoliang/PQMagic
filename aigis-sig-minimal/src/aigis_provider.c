#include "pqmagic_aigis_provider.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int (*AigisKeypairFunc)(unsigned char *pk, unsigned char *sk);
typedef int (*AigisSignFunc)(unsigned char *sig, size_t *siglen, const unsigned char *m,
    size_t mlen, const unsigned char *ctx, size_t ctxLen, const unsigned char *sk);
typedef int (*AigisVerifyFunc)(const unsigned char *sig, size_t siglen, const unsigned char *m,
    size_t mlen, const unsigned char *ctx, size_t ctxLen, const unsigned char *pk);

typedef struct {
    int32_t algId;
    uint32_t pkLen;
    uint32_t skLen;
    uint32_t sigLen;
    AigisKeypairFunc keypair;
    AigisSignFunc sign;
    AigisVerifyFunc verify;
} AigisSigSpec;

typedef struct {
    const AigisSigSpec *spec;
    uint8_t pk[AIGIS_SIG3_PUBLICKEYBYTES];
    uint8_t sk[AIGIS_SIG3_SECRETKEYBYTES];
    uint8_t hasPub;
    uint8_t hasPrv;
} AigisPkeyCtx;

static const AigisSigSpec g_aigisSigSpecs[] = {
#ifdef PQMAGIC_AIGIS_ENABLE_SIG1
    {PQMAGIC_PKEY_AIGIS_SIG1, AIGIS_SIG1_PUBLICKEYBYTES, AIGIS_SIG1_SECRETKEYBYTES, AIGIS_SIG1_SIGBYTES,
        pqmagic_aigis_sig1_std_keypair, pqmagic_aigis_sig1_std_signature, pqmagic_aigis_sig1_std_verify},
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG2
    {PQMAGIC_PKEY_AIGIS_SIG2, AIGIS_SIG2_PUBLICKEYBYTES, AIGIS_SIG2_SECRETKEYBYTES, AIGIS_SIG2_SIGBYTES,
        pqmagic_aigis_sig2_std_keypair, pqmagic_aigis_sig2_std_signature, pqmagic_aigis_sig2_std_verify},
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG3
    {PQMAGIC_PKEY_AIGIS_SIG3, AIGIS_SIG3_PUBLICKEYBYTES, AIGIS_SIG3_SECRETKEYBYTES, AIGIS_SIG3_SIGBYTES,
        pqmagic_aigis_sig3_std_keypair, pqmagic_aigis_sig3_std_signature, pqmagic_aigis_sig3_std_verify},
#endif
};

static const AigisSigSpec *AigisFindSpec(int32_t algId)
{
    for (size_t i = 0; i < sizeof(g_aigisSigSpecs) / sizeof(g_aigisSigSpecs[0]); ++i) {
        if (g_aigisSigSpecs[i].algId == algId) {
            return &g_aigisSigSpecs[i];
        }
    }
    return NULL;
}

static const BSL_Param *AigisFindConstParam(const BSL_Param *params, int32_t key)
{
    if (params == NULL) {
        return NULL;
    }
    for (const BSL_Param *param = params; param->key != 0; ++param) {
        if (param->key == key) {
            return param;
        }
    }
    return NULL;
}

static BSL_Param *AigisFindParam(BSL_Param *params, int32_t key)
{
    if (params == NULL) {
        return NULL;
    }
    for (BSL_Param *param = params; param->key != 0; ++param) {
        if (param->key == key) {
            return param;
        }
    }
    return NULL;
}

static int32_t AigisCopyFromParam(uint8_t *dst, uint32_t dstLen, const BSL_Param *params, int32_t key)
{
    const BSL_Param *param = AigisFindConstParam(params, key);
    if (dst == NULL || param == NULL || param->value == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    if (param->valueType != BSL_PARAM_TYPE_OCTETS || param->valueLen < dstLen) {
        return PQMAGIC_AIGIS_ERR_INVALID_ARG;
    }
    memcpy(dst, param->value, dstLen);
    return PQMAGIC_AIGIS_SUCCESS;
}

static int32_t AigisCopyToParam(const uint8_t *src, uint32_t srcLen, BSL_Param *params, int32_t key)
{
    BSL_Param *param = AigisFindParam(params, key);
    if (src == NULL || param == NULL || param->value == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    param->useLen = srcLen;
    if (param->valueType != BSL_PARAM_TYPE_OCTETS) {
        return PQMAGIC_AIGIS_ERR_INVALID_ARG;
    }
    if (param->valueLen < srcLen) {
        return PQMAGIC_AIGIS_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(param->value, src, srcLen);
    return PQMAGIC_AIGIS_SUCCESS;
}

static void *AigisPkeyNewCtx(void *provCtx, int32_t algId)
{
    (void)provCtx;
    const AigisSigSpec *spec = AigisFindSpec(algId);
    if (spec == NULL) {
        return NULL;
    }
    AigisPkeyCtx *ctx = (AigisPkeyCtx *)calloc(1, sizeof(AigisPkeyCtx));
    if (ctx == NULL) {
        return NULL;
    }
    ctx->spec = spec;
    return ctx;
}

static void AigisPkeyFreeCtx(void *ctx)
{
    free(ctx);
}

static int32_t AigisPkeyGenKey(void *ctx)
{
    AigisPkeyCtx *aigisCtx = (AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    if (aigisCtx->spec->keypair(aigisCtx->pk, aigisCtx->sk) != 0) {
        return PQMAGIC_AIGIS_ERR_INVALID_ARG;
    }
    aigisCtx->hasPub = 1;
    aigisCtx->hasPrv = 1;
    return PQMAGIC_AIGIS_SUCCESS;
}

static int32_t AigisPkeySetPrv(void *ctx, const BSL_Param *params)
{
    AigisPkeyCtx *aigisCtx = (AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    int32_t ret = AigisCopyFromParam(aigisCtx->sk, aigisCtx->spec->skLen, params, PQMAGIC_AIGIS_PARAM_PRVKEY);
    if (ret != PQMAGIC_AIGIS_SUCCESS) {
        return ret;
    }
    aigisCtx->hasPrv = 1;
    return PQMAGIC_AIGIS_SUCCESS;
}

static int32_t AigisPkeySetPub(void *ctx, const BSL_Param *params)
{
    AigisPkeyCtx *aigisCtx = (AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    int32_t ret = AigisCopyFromParam(aigisCtx->pk, aigisCtx->spec->pkLen, params, PQMAGIC_AIGIS_PARAM_PUBKEY);
    if (ret != PQMAGIC_AIGIS_SUCCESS) {
        return ret;
    }
    aigisCtx->hasPub = 1;
    return PQMAGIC_AIGIS_SUCCESS;
}

static int32_t AigisPkeyGetPrv(const void *ctx, BSL_Param *params)
{
    const AigisPkeyCtx *aigisCtx = (const AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL || aigisCtx->hasPrv == 0) {
        return PQMAGIC_AIGIS_ERR_KEY_NOT_SET;
    }
    return AigisCopyToParam(aigisCtx->sk, aigisCtx->spec->skLen, params, PQMAGIC_AIGIS_PARAM_PRVKEY);
}

static int32_t AigisPkeyGetPub(const void *ctx, BSL_Param *params)
{
    const AigisPkeyCtx *aigisCtx = (const AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL || aigisCtx->hasPub == 0) {
        return PQMAGIC_AIGIS_ERR_KEY_NOT_SET;
    }
    return AigisCopyToParam(aigisCtx->pk, aigisCtx->spec->pkLen, params, PQMAGIC_AIGIS_PARAM_PUBKEY);
}

static int32_t AigisPkeySign(void *ctx, int32_t mdAlgId, const uint8_t *data, uint32_t dataLen,
    uint8_t *sign, uint32_t *signLen)
{
    (void)mdAlgId;
    AigisPkeyCtx *aigisCtx = (AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL || data == NULL || sign == NULL || signLen == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    if (aigisCtx->hasPrv == 0) {
        return PQMAGIC_AIGIS_ERR_KEY_NOT_SET;
    }
    if (*signLen < aigisCtx->spec->sigLen) {
        *signLen = aigisCtx->spec->sigLen;
        return PQMAGIC_AIGIS_ERR_BUFFER_TOO_SMALL;
    }
    size_t realSigLen = *signLen;
    int ret = aigisCtx->spec->sign(sign, &realSigLen, data, dataLen, NULL, 0, aigisCtx->sk);
    if (ret != 0) {
        return PQMAGIC_AIGIS_ERR_INVALID_ARG;
    }
    *signLen = (uint32_t)realSigLen;
    return PQMAGIC_AIGIS_SUCCESS;
}

static int32_t AigisPkeyVerify(const void *ctx, int32_t mdAlgId, const uint8_t *data, uint32_t dataLen,
    uint8_t *sign, uint32_t signLen)
{
    (void)mdAlgId;
    const AigisPkeyCtx *aigisCtx = (const AigisPkeyCtx *)ctx;
    if (aigisCtx == NULL || aigisCtx->spec == NULL || data == NULL || sign == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    if (aigisCtx->hasPub == 0) {
        return PQMAGIC_AIGIS_ERR_KEY_NOT_SET;
    }
    if (signLen != aigisCtx->spec->sigLen) {
        return PQMAGIC_AIGIS_ERR_INVALID_ARG;
    }
    if (aigisCtx->spec->verify(sign, signLen, data, dataLen, NULL, 0, aigisCtx->pk) != 0) {
        return PQMAGIC_AIGIS_ERR_VERIFY_FAIL;
    }
    return PQMAGIC_AIGIS_SUCCESS;
}

const CRYPT_EAL_Func g_pqmagicAigisKeyMgmt[] = {
    {CRYPT_EAL_IMPLPKEYMGMT_NEWCTX, (void *)AigisPkeyNewCtx},
    {CRYPT_EAL_IMPLPKEYMGMT_GENKEY, (void *)AigisPkeyGenKey},
    {CRYPT_EAL_IMPLPKEYMGMT_SETPRV, (void *)AigisPkeySetPrv},
    {CRYPT_EAL_IMPLPKEYMGMT_SETPUB, (void *)AigisPkeySetPub},
    {CRYPT_EAL_IMPLPKEYMGMT_GETPRV, (void *)AigisPkeyGetPrv},
    {CRYPT_EAL_IMPLPKEYMGMT_GETPUB, (void *)AigisPkeyGetPub},
    {CRYPT_EAL_IMPLPKEYMGMT_FREECTX, (void *)AigisPkeyFreeCtx},
    CRYPT_EAL_FUNC_END,
};

const CRYPT_EAL_Func g_pqmagicAigisSign[] = {
    {CRYPT_EAL_IMPLPKEYSIGN_SIGN, (void *)AigisPkeySign},
    {CRYPT_EAL_IMPLPKEYSIGN_VERIFY, (void *)AigisPkeyVerify},
    CRYPT_EAL_FUNC_END,
};

static CRYPT_EAL_AlgInfo g_pqmagicAigisKeyMgmtInfo[] = {
#ifdef PQMAGIC_AIGIS_ENABLE_SIG1
    {PQMAGIC_PKEY_AIGIS_SIG1, g_pqmagicAigisKeyMgmt, PQMAGIC_AIGIS_PROVIDER_NAME},
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG2
    {PQMAGIC_PKEY_AIGIS_SIG2, g_pqmagicAigisKeyMgmt, PQMAGIC_AIGIS_PROVIDER_NAME},
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG3
    {PQMAGIC_PKEY_AIGIS_SIG3, g_pqmagicAigisKeyMgmt, PQMAGIC_AIGIS_PROVIDER_NAME},
#endif
    CRYPT_EAL_ALGINFO_END,
};

static CRYPT_EAL_AlgInfo g_pqmagicAigisSignInfo[] = {
#ifdef PQMAGIC_AIGIS_ENABLE_SIG1
    {PQMAGIC_PKEY_AIGIS_SIG1, g_pqmagicAigisSign, PQMAGIC_AIGIS_PROVIDER_NAME},
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG2
    {PQMAGIC_PKEY_AIGIS_SIG2, g_pqmagicAigisSign, PQMAGIC_AIGIS_PROVIDER_NAME},
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG3
    {PQMAGIC_PKEY_AIGIS_SIG3, g_pqmagicAigisSign, PQMAGIC_AIGIS_PROVIDER_NAME},
#endif
    CRYPT_EAL_ALGINFO_END,
};

static int32_t AigisProviderQuery(void *provCtx, int32_t operaId, CRYPT_EAL_AlgInfo **algInfos)
{
    (void)provCtx;
    if (algInfos == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    switch (operaId) {
        case CRYPT_EAL_OPERAID_KEYMGMT:
            *algInfos = g_pqmagicAigisKeyMgmtInfo;
            return PQMAGIC_AIGIS_SUCCESS;
        case CRYPT_EAL_OPERAID_SIGN:
            *algInfos = g_pqmagicAigisSignInfo;
            return PQMAGIC_AIGIS_SUCCESS;
        default:
            *algInfos = NULL;
            return PQMAGIC_AIGIS_ERR_NOT_SUPPORT;
    }
}

static CRYPT_EAL_Func g_pqmagicAigisProviderFuncs[] = {
    {CRYPT_EAL_PROVCB_QUERY, (void *)AigisProviderQuery},
    CRYPT_EAL_FUNC_END,
};

int32_t PQMagic_AIGIS_ProviderInit(CRYPT_EAL_ProvMgrCtx *mgrCtx, BSL_Param *param,
    CRYPT_EAL_Func *capFuncs, CRYPT_EAL_Func **outFuncs, void **provCtx)
{
    (void)mgrCtx;
    (void)param;
    (void)capFuncs;
    if (outFuncs == NULL || provCtx == NULL) {
        return PQMAGIC_AIGIS_ERR_NULL_INPUT;
    }
    *outFuncs = g_pqmagicAigisProviderFuncs;
    *provCtx = NULL;
    return PQMAGIC_AIGIS_SUCCESS;
}

