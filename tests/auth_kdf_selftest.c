/* Offline cross-check of UDPST's OpenSSL 3 KBKDF parameters against
 * an independent SP800-108 counter-mode HMAC-SHA256 construction. */
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(void) {
    const char *secret = "offline-test-key-not-a-production-secret";
    const char *context = "1700000000";
    unsigned char derived[64], expected[64], message[128], digest[32];
    unsigned int n = 0;
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "KBKDF", NULL);
    EVP_KDF_CTX *ctx = kdf ? EVP_KDF_CTX_new(kdf) : NULL;
    int one = 1, ok = 0;
    OSSL_PARAM params[9], *p = params;
    if (!ctx) goto done;
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_MODE, "COUNTER", 0);
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_MAC, "HMAC", 0);
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, "SHA256", 0);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, (void *)secret, strlen(secret));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, "UDPSTP", 6);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, (void *)context, strlen(context));
    *p++ = OSSL_PARAM_construct_int(OSSL_KDF_PARAM_KBKDF_USE_L, &one);
    *p++ = OSSL_PARAM_construct_int(OSSL_KDF_PARAM_KBKDF_USE_SEPARATOR, &one);
    *p++ = OSSL_PARAM_construct_end();
    if (EVP_KDF_derive(ctx, derived, sizeof(derived), params) != 1) goto done;
    for (unsigned int counter = 1; counter <= 2; ++counter) {
        size_t pos = 0;
        message[pos++] = 0; message[pos++] = 0; message[pos++] = 0; message[pos++] = (unsigned char)counter;
        memcpy(message + pos, "UDPSTP", 6); pos += 6;
        message[pos++] = 0;
        memcpy(message + pos, context, strlen(context)); pos += strlen(context);
        message[pos++] = 0; message[pos++] = 0; message[pos++] = 2; message[pos++] = 0; /* 512 bits */
        if (!HMAC(EVP_sha256(), secret, (int)strlen(secret), message, pos, digest, &n) || n != 32) goto done;
        memcpy(expected + (counter - 1) * 32, digest, 32);
    }
    if (memcmp(derived, expected, sizeof(derived)) != 0) goto done;
    /* Client and server halves must differ; verify HMAC tampering detection. */
    if (memcmp(derived, derived + 32, 32) == 0) goto done;
    if (!HMAC(EVP_sha256(), derived + 32, 32, (const unsigned char *)"response", 8, digest, &n)) goto done;
    {
        unsigned char changed[8] = {'r','e','s','p','o','n','s','X'}, other[32];
        if (!HMAC(EVP_sha256(), derived + 32, 32, changed, 8, other, &n)) goto done;
        if (memcmp(digest, other, 32) == 0) goto done;
    }
    ok = 1;
done:
    EVP_KDF_CTX_free(ctx); EVP_KDF_free(kdf);
    puts(ok ? "UDPST KBKDF/HMAC offline self-test PASS" : "UDPST KBKDF/HMAC offline self-test FAIL");
    return ok ? 0 : 1;
}
