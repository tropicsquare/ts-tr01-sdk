#include "common.h"
#include "sha256.h"

#include "scb.h"

#define SHA256_PADDING_CHARACTER 0x80

void sha256_init(sha256_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

void sha256_suspend(void)
{
    scb_suspend();
}

void sha256_wakeup(void)
{
    scb_wakeup();
}

void sha256_update(sha256_t *ctx, const u8 *data, size_t length)
{
    const u8 *pdata = data;

    OS_SANITY_NULL(ctx);
    OS_SANITY_NULL(data);
    OS_SANITY(ctx->buf_length < SHA256_CHUNK_SIZE);
    
    while (length--)
    {
        ctx->buf[ctx->buf_length++] = *pdata++;
        if (ctx->buf_length >= SHA256_CHUNK_SIZE)
        {
            ts_bool init = (ctx->bit_length == 0) ? TS_TRUE : TS_FALSE;

            scb_sha256_round(init, ctx->buf);
            
            ctx->buf_length = 0;
            ctx->bit_length += (SHA256_CHUNK_SIZE * 8);
        }
    }
}

void sha256_final(sha256_t *ctx, u8 *hash)
{
    // Padding to CHUNK_SIZE:
    //  1) add byte 0x80 (PADDING_CHARACTER)
    //  2) fill by 0x00 up to CHUNK_SIZE - 8
    //  3) rest 8 byte is length in bits (whole message size)
    
    size_t l;
    u8 *pbuf; 
    u32 bits;
    ts_bool init;
    
    OS_SANITY_NULL(ctx);
    OS_SANITY_NULL(hash);

    l = ctx->buf_length;
    pbuf = ctx->buf;
    OS_ASSERT(l < SHA256_CHUNK_SIZE);

    init = (ctx->bit_length == 0) ? TS_TRUE : TS_FALSE; // may be only one chunk

    // Clear rest of buffer
    memset(pbuf + l, 0, SHA256_CHUNK_SIZE-l);

    // Update length by last incomplete chunk
    ctx->bit_length += (l << 3);
    bits = ctx->bit_length;

    // Add the padding character
    pbuf[l] = SHA256_PADDING_CHARACTER;

    if (l > (SHA256_CHUNK_SIZE - (sizeof(u64) + 1))) // 64bit length field + padding character
    {   // We need one more sha256_update block
        // Process the block
        scb_sha256_round(init, pbuf);
        init = TS_FALSE;
        // Prepare the last chunk
        memset(pbuf, 0, SHA256_CHUNK_SIZE);
    }

    // There is 8B space for length, we use only u32 value
    // the number is BIG endian (it is SHA specification).
    pbuf[SHA256_CHUNK_SIZE-1] = bits & 0xFF;
    pbuf[SHA256_CHUNK_SIZE-2] = (bits >> 8) & 0xFF;
    pbuf[SHA256_CHUNK_SIZE-3] = (bits >> 16) & 0xFF;
    pbuf[SHA256_CHUNK_SIZE-4] = (bits >> 24) & 0xFF;

    scb_sha256_round(init, pbuf);
    scb_sha256_read(hash);
}
