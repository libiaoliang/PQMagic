#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pqmagic_api.h"

#define MESSAGE_LEN 64
#define RUN_SECONDS 10.0
#define BATCH_SIZE 16

typedef int (*bench_op_fn)(void *ctx);

struct bench_result {
    uint64_t count;
    double elapsed;
    unsigned int checksum;
};

struct sig_ctx {
    const unsigned char *sk;
    const unsigned char *pk;
    const unsigned char *msg;
    size_t msglen;
    unsigned char *sig;
    size_t *siglen;
    unsigned int checksum;
};

static double now_seconds(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }

    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static int sign_once(void *arg)
{
    struct sig_ctx *ctx = arg;
    int ret;

    *ctx->siglen = 0;
    ret = pqmagic_aigis_sig1_std_signature(
        ctx->sig, ctx->siglen, ctx->msg, ctx->msglen, NULL, 0, ctx->sk);
    if (ret != 0) {
        return ret;
    }

    ctx->checksum ^= ctx->sig[0];
    return 0;
}

static int verify_once(void *arg)
{
    struct sig_ctx *ctx = arg;
    int ret;

    ret = pqmagic_aigis_sig1_std_verify(
        ctx->sig, *ctx->siglen, ctx->msg, ctx->msglen, NULL, 0, ctx->pk);
    if (ret != 0) {
        return ret;
    }

    ctx->checksum ^= ctx->sig[0];
    return 0;
}

static int run_benchmark(const char *name, bench_op_fn op, void *ctx,
    struct bench_result *result)
{
    const double start = now_seconds();
    double elapsed = 0.0;

    result->count = 0;
    result->elapsed = 0.0;
    result->checksum = 0;

    if (start == 0.0) {
        fprintf(stderr, "clock_gettime failed\n");
        return 1;
    }

    do {
        for (unsigned int i = 0; i < BATCH_SIZE; i++) {
            int ret = op(ctx);
            if (ret != 0) {
                fprintf(stderr, "%s failed after %llu operations: %d\n",
                    name, (unsigned long long)result->count, ret);
                return 1;
            }
            result->count++;
        }

        elapsed = now_seconds() - start;
    } while (elapsed < RUN_SECONDS);

    result->elapsed = elapsed;
    return 0;
}

static void print_result(const char *name, const struct bench_result *result)
{
    printf("%s elapsed seconds: %.6f\n", name, result->elapsed);
    printf("%s operations: %llu\n", name, (unsigned long long)result->count);
    printf("%s operations/sec: %.3f\n",
        name, (double)result->count / result->elapsed);
}

int main(void)
{
    unsigned char pk[AIGIS_SIG1_PUBLICKEYBYTES];
    unsigned char sk[AIGIS_SIG1_SECRETKEYBYTES];
    unsigned char sig[AIGIS_SIG1_SIGBYTES];
    unsigned char msg[MESSAGE_LEN];
    struct sig_ctx ctx;
    struct bench_result sign_result;
    struct bench_result verify_result;
    size_t siglen = 0;
    int ret;

    memset(msg, 0x42, sizeof(msg));

    ret = pqmagic_aigis_sig1_std_keypair(pk, sk);
    if (ret != 0) {
        fprintf(stderr, "keypair failed: %d\n", ret);
        return 1;
    }

    ret = pqmagic_aigis_sig1_std_signature(
        sig, &siglen, msg, sizeof(msg), NULL, 0, sk);
    if (ret != 0) {
        fprintf(stderr, "sign warmup failed: %d\n", ret);
        return 1;
    }

    ret = pqmagic_aigis_sig1_std_verify(
        sig, siglen, msg, sizeof(msg), NULL, 0, pk);
    if (ret != 0) {
        fprintf(stderr, "verify warmup signature failed: %d\n", ret);
        return 1;
    }

    ctx.sk = sk;
    ctx.pk = pk;
    ctx.msg = msg;
    ctx.msglen = sizeof(msg);
    ctx.sig = sig;
    ctx.siglen = &siglen;
    ctx.checksum = 0;

    ret = run_benchmark("sign", sign_once, &ctx, &sign_result);
    if (ret != 0) {
        return ret;
    }
    sign_result.checksum = ctx.checksum;

    ret = pqmagic_aigis_sig1_std_signature(
        sig, &siglen, msg, sizeof(msg), NULL, 0, sk);
    if (ret != 0) {
        fprintf(stderr, "sign before verify benchmark failed: %d\n", ret);
        return 1;
    }

    ctx.checksum = 0;
    ret = run_benchmark("verify", verify_once, &ctx, &verify_result);
    if (ret != 0) {
        return ret;
    }
    verify_result.checksum = ctx.checksum;

    printf("AIGIS-SIG1 sign/verify demo\n");
    printf("message bytes: %d\n", MESSAGE_LEN);
    printf("signature bytes: %zu\n", siglen);
    print_result("sign", &sign_result);
    print_result("verify", &verify_result);
    printf("checksum: %u\n", sign_result.checksum ^ verify_result.checksum);

    return 0;
}
