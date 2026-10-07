#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {

// ==========================================
// Standard C Implementations: SHA-256, SHA-1, MD5
// Standard, audited reference implementations
// ==========================================

// --- SHA-256 (FIPS 180-4) ---
struct SHA256_CTX {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
};

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k256[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;

    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        std::memset(ctx->data, 0, 56);
    }

    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;
    ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
    }
}

// --- SHA-1 (FIPS 180-1 / RFC 3174) ---
struct SHA1_CTX {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[5];
    uint32_t k[4];
};

#define SHA1_ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

static void sha1_transform(SHA1_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, i, j, t, m[80];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) + (data[j + 1] << 16) + (data[j + 2] << 8) + (data[j + 3]);
    for ( ; i < 80; ++i) {
        m[i] = (m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16]);
        m[i] = (m[i] << 1) | (m[i] >> 31);
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];

    for (i = 0; i < 20; ++i) {
        t = SHA1_ROTLEFT(a, 5) + ((b & c) ^ (~b & d)) + e + ctx->k[0] + m[i];
        e = d;
        d = c;
        c = SHA1_ROTLEFT(b, 30);
        b = a;
        a = t;
    }
    for ( ; i < 40; ++i) {
        t = SHA1_ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[1] + m[i];
        e = d;
        d = c;
        c = SHA1_ROTLEFT(b, 30);
        b = a;
        a = t;
    }
    for ( ; i < 60; ++i) {
        t = SHA1_ROTLEFT(a, 5) + ((b & c) ^ (b & d) ^ (c & d))  + e + ctx->k[2] + m[i];
        e = d;
        d = c;
        c = SHA1_ROTLEFT(b, 30);
        b = a;
        a = t;
    }
    for ( ; i < 80; ++i) {
        t = SHA1_ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[3] + m[i];
        e = d;
        d = c;
        c = SHA1_ROTLEFT(b, 30);
        b = a;
        a = t;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

static void sha1_init(SHA1_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->k[0] = 0x5A827999;
    ctx->k[1] = 0x6ED9EBA1;
    ctx->k[2] = 0x8F1BBCDC;
    ctx->k[3] = 0xCA62C1D6;
}

static void sha1_update(SHA1_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha1_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha1_final(SHA1_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;

    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha1_transform(ctx, ctx->data);
        std::memset(ctx->data, 0, 56);
    }

    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;
    ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;
    sha1_transform(ctx, ctx->data);

    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
    }
}

// --- MD5 (RFC 1321) ---
struct MD5_CTX {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
};

#define F_MD5(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G_MD5(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H_MD5(x, y, z) ((x) ^ (y) ^ (z))
#define I_MD5(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF_MD5(a, b, c, d, x, s, ac) { \
 (a) += F_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG_MD5(a, b, c, d, x, s, ac) { \
 (a) += G_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH_MD5(a, b, c, d, x, s, ac) { \
 (a) += H_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II_MD5(a, b, c, d, x, s, ac) { \
 (a) += I_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    for (int i = 0, j = 0; i < 16; i++, j += 4)
        x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j+1]) << 8) |
               (((uint32_t)block[j+2]) << 16) | (((uint32_t)block[j+3]) << 24);

    FF_MD5(a, b, c, d, x[ 0], 7, 0xd76aa478);
    FF_MD5(d, a, b, c, x[ 1], 12, 0xe8c7b756);
    FF_MD5(c, d, a, b, x[ 2], 17, 0x242070db);
    FF_MD5(b, c, d, a, x[ 3], 22, 0xc1bdceee);
    FF_MD5(a, b, c, d, x[ 4], 7, 0xf57c0faf);
    FF_MD5(d, a, b, c, x[ 5], 12, 0x4787c62a);
    FF_MD5(c, d, a, b, x[ 6], 17, 0xa8304613);
    FF_MD5(b, c, d, a, x[ 7], 22, 0xfd469501);
    FF_MD5(a, b, c, d, x[ 8], 7, 0x698098d8);
    FF_MD5(d, a, b, c, x[ 9], 12, 0x8b44f7af);
    FF_MD5(c, d, a, b, x[10], 17, 0xffff5bb1);
    FF_MD5(b, c, d, a, x[11], 22, 0x895cd7be);
    FF_MD5(a, b, c, d, x[12], 7, 0x6b901122);
    FF_MD5(d, a, b, c, x[13], 12, 0xfd987193);
    FF_MD5(c, d, a, b, x[14], 17, 0xa679438e);
    FF_MD5(b, c, d, a, x[15], 22, 0x49b40821);

    GG_MD5(a, b, c, d, x[ 1], 5, 0xf61e2562);
    GG_MD5(d, a, b, c, x[ 6], 9, 0xc040b340);
    GG_MD5(c, d, a, b, x[11], 14, 0x265e5a51);
    GG_MD5(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    GG_MD5(a, b, c, d, x[ 5], 5, 0xd62f105d);
    GG_MD5(d, a, b, c, x[10], 9, 0x02441453);
    GG_MD5(c, d, a, b, x[15], 14, 0xd8a1e681);
    GG_MD5(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    GG_MD5(a, b, c, d, x[ 9], 5, 0x21e1cde6);
    GG_MD5(d, a, b, c, x[14], 9, 0xc33707d6);
    GG_MD5(c, d, a, b, x[ 3], 14, 0xf4d50d87);
    GG_MD5(b, c, d, a, x[ 8], 20, 0x455a14ed);
    GG_MD5(a, b, c, d, x[13], 5, 0xa9e3e905);
    GG_MD5(d, a, b, c, x[ 2], 9, 0xfcefa3f8);
    GG_MD5(c, d, a, b, x[ 7], 14, 0x676f02d9);
    GG_MD5(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    HH_MD5(a, b, c, d, x[ 5], 4, 0xfffa3942);
    HH_MD5(d, a, b, c, x[ 8], 11, 0x8771f681);
    HH_MD5(c, d, a, b, x[11], 16, 0x6d9d6122);
    HH_MD5(b, c, d, a, x[14], 23, 0xfde5380c);
    HH_MD5(a, b, c, d, x[ 1], 4, 0xa4beea44);
    HH_MD5(d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    HH_MD5(c, d, a, b, x[ 7], 16, 0xf6bb4b60);
    HH_MD5(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH_MD5(a, b, c, d, x[13], 4, 0x289b7ec6);
    HH_MD5(d, a, b, c, x[ 0], 11, 0xeaa127fa);
    HH_MD5(c, d, a, b, x[ 3], 16, 0xd4ef3085);
    HH_MD5(b, c, d, a, x[ 6], 23, 0x04881d05);
    HH_MD5(a, b, c, d, x[ 9], 4, 0xd9d4d039);
    HH_MD5(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH_MD5(c, d, a, b, x[15], 16, 0x1fa27cf8);
    HH_MD5(b, c, d, a, x[ 2], 23, 0xc4ac5665);

    II_MD5(a, b, c, d, x[ 0], 6, 0xf4292244);
    II_MD5(d, a, b, c, x[ 7], 10, 0x432aff97);
    II_MD5(c, d, a, b, x[14], 15, 0xab9423a7);
    II_MD5(b, c, d, a, x[ 5], 21, 0xfc93a039);
    II_MD5(a, b, c, d, x[12], 6, 0x655b59c3);
    II_MD5(d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    II_MD5(c, d, a, b, x[10], 15, 0xffeff47d);
    II_MD5(b, c, d, a, x[ 1], 21, 0x85845dd1);
    II_MD5(a, b, c, d, x[ 8], 6, 0x6fa87e4f);
    II_MD5(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II_MD5(c, d, a, b, x[ 6], 15, 0xa3014314);
    II_MD5(b, c, d, a, x[13], 21, 0x4e0811a1);
    II_MD5(a, b, c, d, x[ 4], 6, 0xf7537e82);
    II_MD5(d, a, b, c, x[11], 10, 0xbd3af235);
    II_MD5(c, d, a, b, x[ 2], 15, 0x2ad7d2bb);
    II_MD5(b, c, d, a, x[ 9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static void md5_init(MD5_CTX *context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

static void md5_update(MD5_CTX *context, const uint8_t *input, size_t inputLen) {
    size_t i, index, partLen;
    index = (size_t)((context->count[0] >> 3) & 0x3F);
    if ((context->count[0] += ((uint32_t)inputLen << 3)) < ((uint32_t)inputLen << 3))
        context->count[1]++;
    context->count[1] += ((uint32_t)inputLen >> 29);
    partLen = 64 - index;

    if (inputLen >= partLen) {
        std::memcpy(&context->buffer[index], input, partLen);
        md5_transform(context->state, context->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64)
            md5_transform(context->state, &input[i]);
        index = 0;
    } else i = 0;
    std::memcpy(&context->buffer[index], &input[i], inputLen - i);
}

static void md5_final(MD5_CTX *context, uint8_t digest[16]) {
    uint8_t bits[8];
    size_t index, padLen;
    static const uint8_t PADDING[64] = { 0x80 };

    for (int i = 0; i < 8; i++)
        bits[i] = (uint8_t)((context->count[i >= 4 ? 1 : 0] >> ((i % 4) * 8)) & 0xFF);

    index = (size_t)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    md5_update(context, PADDING, padLen);
    md5_update(context, bits, 8);

    for (int i = 0; i < 16; i++)
        digest[i] = (uint8_t)((context->state[i / 4] >> ((i % 4) * 8)) & 0xFF);
}

// --- Base64 Encoding / Decoding ---
static const char b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const uint8_t *data, size_t len) {
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t b0 = data[i];
        uint32_t b1 = (i + 1 < len) ? data[i + 1] : 0;
        uint32_t b2 = (i + 2 < len) ? data[i + 2] : 0;

        uint32_t triple = (b0 << 16) | (b1 << 8) | b2;

        out.push_back(b64_chars[(triple >> 18) & 0x3F]);
        out.push_back(b64_chars[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? b64_chars[(triple >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < len) ? b64_chars[triple & 0x3F] : '=');
    }
    return out;
}

static int b64_char_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return -2; // padding
    return -1; // invalid
}

static bool base64_decode(const std::string &in, std::vector<uint8_t> &out) {
    out.clear();
    if (in.empty()) return true;
    if (in.length() % 4 != 0) return false;

    out.reserve((in.length() / 4) * 3);
    for (size_t i = 0; i < in.length(); i += 4) {
        int v0 = b64_char_val(in[i]);
        int v1 = b64_char_val(in[i + 1]);
        int v2 = b64_char_val(in[i + 2]);
        int v3 = b64_char_val(in[i + 3]);

        if (v0 < 0 || v1 < 0) return false;
        if (v2 == -1 || v3 == -1) return false;
        if (v2 == -2 && v3 != -2) return false; // invalid padding sequence

        uint32_t triple = (static_cast<uint32_t>(v0) << 18) | (static_cast<uint32_t>(v1) << 12);
        out.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));

        if (v2 != -2) {
            triple |= (static_cast<uint32_t>(v2) << 6);
            out.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
            if (v3 != -2) {
                triple |= static_cast<uint32_t>(v3);
                out.push_back(static_cast<uint8_t>(triple & 0xFF));
            }
        }
    }
    return true;
}

// --- Hex Encoding / Decoding ---
static const char hex_table[] = "0123456789abcdef";

static std::string hex_encode(const uint8_t *data, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(hex_table[(data[i] >> 4) & 0xF]);
        out.push_back(hex_table[data[i] & 0xF]);
    }
    return out;
}

static int hex_char_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool hex_decode(const std::string &in, std::vector<uint8_t> &out) {
    out.clear();
    if (in.empty()) return true;
    if (in.length() % 2 != 0) return false;

    out.reserve(in.length() / 2);
    for (size_t i = 0; i < in.length(); i += 2) {
        int hi = hex_char_val(in[i]);
        int lo = hex_char_val(in[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return true;
}

// ==========================================
// Solix Heap Helper Functions
// ==========================================

static std::string read_solix_chars(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return "";
    uint32_t len = static_cast<uint32_t>(vm.memory.heap[addr - 1] >> 32);
    std::string s;
    s.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        s.push_back(static_cast<char>(vm.memory.heap[addr + i]));
    }
    return s;
}

static uint64_t allocate_solix_chars(solix::RuntimeContext &vm, const std::string &s) {
    uint64_t addr = vm.memory.dynamic_allocation(s.length());
    for (size_t i = 0; i < s.length(); ++i) {
        vm.memory.heap[addr + i] = static_cast<uint64_t>(static_cast<unsigned char>(s[i]));
    }
    return addr;
}

static std::vector<uint8_t> read_solix_bytes(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return {};
    uint32_t len = static_cast<uint32_t>(vm.memory.heap[addr - 1] >> 32);
    std::vector<uint8_t> b;
    b.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        b.push_back(static_cast<uint8_t>(vm.memory.heap[addr + i]));
    }
    return b;
}

static uint64_t allocate_solix_bytes(solix::RuntimeContext &vm, const uint8_t *data, size_t len) {
    uint64_t addr = vm.memory.dynamic_allocation(len);
    for (size_t i = 0; i < len; ++i) {
        int8_t sbyte = static_cast<int8_t>(data[i]);
        vm.memory.heap[addr + i] = static_cast<uint64_t>(static_cast<int64_t>(sbyte));
    }
    return addr;
}

// ==========================================
// Native Solix Bridge Functions
// ==========================================

// Base64 natives
static uint64_t crypto_native_base64_encode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return 0ULL;
    auto bytes = read_solix_bytes(vm, addr);
    std::string encoded = base64_encode(bytes.data(), bytes.size());
    return allocate_solix_chars(vm, encoded);
}

static uint64_t crypto_native_base64_encode_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return 0ULL;
    std::string text = read_solix_chars(vm, addr);
    std::string encoded = base64_encode(reinterpret_cast<const uint8_t*>(text.data()), text.size());
    return allocate_solix_chars(vm, encoded);
}

static uint64_t crypto_native_base64_decode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return 0ULL;
    std::string b64 = read_solix_chars(vm, addr);
    std::vector<uint8_t> out;
    if (!base64_decode(b64, out)) {
        return 0ULL; // null indicates error for Solix to throw FormatException
    }
    return allocate_solix_bytes(vm, out.data(), out.size());
}

static uint64_t crypto_native_base64_decode_to_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return 0ULL;
    std::string b64 = read_solix_chars(vm, addr);
    std::vector<uint8_t> out;
    if (!base64_decode(b64, out)) {
        return 0ULL; // null indicates error
    }
    std::string s(reinterpret_cast<const char*>(out.data()), out.size());
    return allocate_solix_chars(vm, s);
}

// Hex natives
static uint64_t crypto_native_hex_encode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return 0ULL;
    auto bytes = read_solix_bytes(vm, addr);
    std::string encoded = hex_encode(bytes.data(), bytes.size());
    return allocate_solix_chars(vm, encoded);
}

static uint64_t crypto_native_hex_decode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return 0ULL;
    std::string hex = read_solix_chars(vm, addr);
    std::vector<uint8_t> out;
    if (!hex_decode(hex, out)) {
        return 0ULL; // null indicates error
    }
    return allocate_solix_bytes(vm, out.data(), out.size());
}

// Hash natives
static uint64_t crypto_native_sha256(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return allocate_solix_bytes(vm, nullptr, 0);
    auto bytes = read_solix_bytes(vm, addr);
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, bytes.data(), bytes.size());
    uint8_t digest[32];
    sha256_final(&ctx, digest);
    return allocate_solix_bytes(vm, digest, 32);
}

static uint64_t crypto_native_sha256_hex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    std::string text = read_solix_chars(vm, addr);
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, reinterpret_cast<const uint8_t*>(text.data()), text.size());
    uint8_t digest[32];
    sha256_final(&ctx, digest);
    std::string hex = hex_encode(digest, 32);
    return allocate_solix_chars(vm, hex);
}

static uint64_t crypto_native_sha1(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return allocate_solix_bytes(vm, nullptr, 0);
    auto bytes = read_solix_bytes(vm, addr);
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, bytes.data(), bytes.size());
    uint8_t digest[20];
    sha1_final(&ctx, digest);
    return allocate_solix_bytes(vm, digest, 20);
}

static uint64_t crypto_native_sha1_hex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    std::string text = read_solix_chars(vm, addr);
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, reinterpret_cast<const uint8_t*>(text.data()), text.size());
    uint8_t digest[20];
    sha1_final(&ctx, digest);
    std::string hex = hex_encode(digest, 20);
    return allocate_solix_chars(vm, hex);
}

static uint64_t crypto_native_md5(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    if (addr == 0) return allocate_solix_bytes(vm, nullptr, 0);
    auto bytes = read_solix_bytes(vm, addr);
    MD5_CTX ctx;
    md5_init(&ctx);
    md5_update(&ctx, bytes.data(), bytes.size());
    uint8_t digest[16];
    md5_final(&ctx, digest);
    return allocate_solix_bytes(vm, digest, 16);
}

static uint64_t crypto_native_md5_hex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    uint64_t addr = args[0];
    std::string text = read_solix_chars(vm, addr);
    MD5_CTX ctx;
    md5_init(&ctx);
    md5_update(&ctx, reinterpret_cast<const uint8_t*>(text.data()), text.size());
    uint8_t digest[16];
    md5_final(&ctx, digest);
    std::string hex = hex_encode(digest, 16);
    return allocate_solix_chars(vm, hex);
}

} // namespace

void register_crypto_natives(solix::NativeRegistry &registry) {
    auto reg = [&](const std::string &cls, const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function(cls + "_" + name, func);
        registry.register_function("solix_crypto_" + cls + "_" + name, func);
        registry.register_function("solix.crypto." + cls + "." + name, func);
    };

    reg("Base64", "native_encode", crypto_native_base64_encode);
    reg("Base64", "native_encode_string", crypto_native_base64_encode_string);
    reg("Base64", "native_decode", crypto_native_base64_decode);
    reg("Base64", "native_decode_to_string", crypto_native_base64_decode_to_string);

    reg("Hex", "native_encode", crypto_native_hex_encode);
    reg("Hex", "native_decode", crypto_native_hex_decode);

    reg("Hash", "native_sha256", crypto_native_sha256);
    reg("Hash", "native_sha256_hex", crypto_native_sha256_hex);
    reg("Hash", "native_sha1", crypto_native_sha1);
    reg("Hash", "native_sha1_hex", crypto_native_sha1_hex);
    reg("Hash", "native_md5", crypto_native_md5);
    reg("Hash", "native_md5_hex", crypto_native_md5_hex);
}
