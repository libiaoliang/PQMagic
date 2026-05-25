#include "pqmagic_aigis_provider.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void *FindFunc(const CRYPT_EAL_Func *funcs, int32_t id)
{
    if (funcs == NULL) {
        return NULL;
    }
    for (const CRYPT_EAL_Func *func = funcs; func->id != 0; ++func) {
        if (func->id == id) {
            return func->func;
        }
    }
    return NULL;
}

static const CRYPT_EAL_Func *FindAlg(CRYPT_EAL_AlgInfo *infos, int32_t algId)
{
    if (infos == NULL) {
        return NULL;
    }
    for (CRYPT_EAL_AlgInfo *info = infos; info->algId != 0; ++info) {
        if (info->algId == algId) {
            return info->implFunc;
        }
    }
    return NULL;
}

static int RunOne(int32_t algId, uint32_t pkLen, uint32_t skLen, uint32_t sigLen)
{
    CRYPT_EAL_Func *outFuncs = NULL;
    void *provCtx = NULL;
    if (PQMagic_AIGIS_ProviderInit(NULL, NULL, NULL, &outFuncs, &provCtx) != 0) {
        return 1;
    }

    CRYPT_EAL_ProvQueryCb query = (CRYPT_EAL_ProvQueryCb)FindFunc(outFuncs, CRYPT_EAL_PROVCB_QUERY);
    if (query == NULL) {
        return 2;
    }

    CRYPT_EAL_AlgInfo *keyInfos = NULL;
    CRYPT_EAL_AlgInfo *signInfos = NULL;
    if (query(provCtx, CRYPT_EAL_OPERAID_KEYMGMT, &keyInfos) != 0 ||
        query(provCtx, CRYPT_EAL_OPERAID_SIGN, &signInfos) != 0) {
        return 3;
    }

    const CRYPT_EAL_Func *keyFuncs = FindAlg(keyInfos, algId);
    const CRYPT_EAL_Func *signFuncs = FindAlg(signInfos, algId);
    if (keyFuncs == NULL || signFuncs == NULL) {
        return 4;
    }

    CRYPT_EAL_ImplPkeyMgmtNewCtx newCtx =
        (CRYPT_EAL_ImplPkeyMgmtNewCtx)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_NEWCTX);
    CRYPT_EAL_ImplPkeyMgmtGenKey genKey =
        (CRYPT_EAL_ImplPkeyMgmtGenKey)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_GENKEY);
    CRYPT_EAL_ImplPkeyMgmtGetPub getPub =
        (CRYPT_EAL_ImplPkeyMgmtGetPub)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_GETPUB);
    CRYPT_EAL_ImplPkeyMgmtGetPrv getPrv =
        (CRYPT_EAL_ImplPkeyMgmtGetPrv)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_GETPRV);
    CRYPT_EAL_ImplPkeyMgmtSetPub setPub =
        (CRYPT_EAL_ImplPkeyMgmtSetPub)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_SETPUB);
    CRYPT_EAL_ImplPkeyMgmtCtrl ctrl =
        (CRYPT_EAL_ImplPkeyMgmtCtrl)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_CTRL);
    CRYPT_EAL_ImplPkeyMgmtFreeCtx freeCtx =
        (CRYPT_EAL_ImplPkeyMgmtFreeCtx)FindFunc(keyFuncs, CRYPT_EAL_IMPLPKEYMGMT_FREECTX);
    CRYPT_EAL_ImplPkeySign sign =
        (CRYPT_EAL_ImplPkeySign)FindFunc(signFuncs, CRYPT_EAL_IMPLPKEYSIGN_SIGN);
    CRYPT_EAL_ImplPkeyVerify verify =
        (CRYPT_EAL_ImplPkeyVerify)FindFunc(signFuncs, CRYPT_EAL_IMPLPKEYSIGN_VERIFY);

    if (newCtx == NULL || genKey == NULL || getPub == NULL || getPrv == NULL || ctrl == NULL ||
        setPub == NULL || freeCtx == NULL || sign == NULL || verify == NULL) {
        return 5;
    }

    void *signCtx = newCtx(provCtx, algId);
    void *verifyCtx = newCtx(provCtx, algId);
    if (signCtx == NULL || verifyCtx == NULL) {
        return 6;
    }

    uint8_t pk[AIGIS_SIG3_PUBLICKEYBYTES];
    uint8_t sk[AIGIS_SIG3_SECRETKEYBYTES];
    uint8_t sig[AIGIS_SIG3_SIGBYTES];
    uint32_t signLen = sigLen;
    uint32_t ctrlPkLen = 0;
    uint32_t ctrlSkLen = 0;
    const uint8_t msg[] = "pqmagic aigis provider smoke";

    BSL_Param getPubParams[] = {
        {PQMAGIC_AIGIS_PARAM_PUBKEY, BSL_PARAM_TYPE_OCTETS, pk, pkLen, 0},
        BSL_PARAM_END,
    };
    BSL_Param getPrvParams[] = {
        {PQMAGIC_AIGIS_PARAM_PRVKEY, BSL_PARAM_TYPE_OCTETS, sk, skLen, 0},
        BSL_PARAM_END,
    };
    BSL_Param setPubParams[] = {
        {PQMAGIC_AIGIS_PARAM_PUBKEY, BSL_PARAM_TYPE_OCTETS, pk, pkLen, 0},
        BSL_PARAM_END,
    };

    int ret = 0;
    if (ctrl(signCtx, PQMAGIC_AIGIS_CTRL_GET_PUBKEY_LEN, &ctrlPkLen, sizeof(ctrlPkLen)) != 0 ||
        ctrl(signCtx, PQMAGIC_AIGIS_CTRL_GET_PRVKEY_LEN, &ctrlSkLen, sizeof(ctrlSkLen)) != 0 ||
        ctrlPkLen != pkLen ||
        ctrlSkLen != skLen ||
        genKey(signCtx) != 0 ||
        getPub(signCtx, getPubParams) != 0 ||
        getPrv(signCtx, getPrvParams) != 0 ||
        getPubParams[0].useLen != pkLen ||
        getPrvParams[0].useLen != skLen ||
        setPub(verifyCtx, setPubParams) != 0 ||
        sign(signCtx, 0, msg, (uint32_t)strlen((const char *)msg), sig, &signLen) != 0 ||
        signLen != sigLen ||
        verify(verifyCtx, 0, msg, (uint32_t)strlen((const char *)msg), sig, signLen) != 0) {
        ret = 7;
    }

    freeCtx(signCtx);
    freeCtx(verifyCtx);
    return ret;
}

int main(void)
{
#ifdef PQMAGIC_AIGIS_ENABLE_SIG1
    if (RunOne(PQMAGIC_PKEY_AIGIS_SIG1, AIGIS_SIG1_PUBLICKEYBYTES, AIGIS_SIG1_SECRETKEYBYTES,
        AIGIS_SIG1_SIGBYTES) != 0) {
        return 1;
    }
    puts("provider mode 1: PASS");
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG2
    if (RunOne(PQMAGIC_PKEY_AIGIS_SIG2, AIGIS_SIG2_PUBLICKEYBYTES, AIGIS_SIG2_SECRETKEYBYTES,
        AIGIS_SIG2_SIGBYTES) != 0) {
        return 2;
    }
    puts("provider mode 2: PASS");
#endif
#ifdef PQMAGIC_AIGIS_ENABLE_SIG3
    if (RunOne(PQMAGIC_PKEY_AIGIS_SIG3, AIGIS_SIG3_PUBLICKEYBYTES, AIGIS_SIG3_SECRETKEYBYTES,
        AIGIS_SIG3_SIGBYTES) != 0) {
        return 3;
    }
    puts("provider mode 3: PASS");
#endif
    return 0;
}
