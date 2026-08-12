/**
 * @file scb.c
 * @author Tropic Square
 * @brief SCB (Secure Channel Block) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "scb.h"
#include "kdb.h"
#include "cpb.h"

#define PROTOCOL_NAME "Noise_KK1_25519_AESGCM_SHA256\x00\x00\x00"
#define SHA256_PADDING_CHARACTER 0x80

#include "soc_ctrl.h"
#include "tassic_defs.h"
#include "scb_regs.h"
#include "spect.h"
#include "io_ops.h"

#include "log.h"
LOG_DEF("SCB");

#define _SCB_ERR_ISR 1

#define _LOG_DEBUG(...) // LOG_DEBUG(__VA_ARGS__)

#ifdef SIMULATION_IO_OPS
  #warning "SIMULATION_IO_OPS enabled"
#endif

#define _SCB_REG_WRITE(offset, value) IO_WRITE_32(TROPIC01_MEMORY_MAP_SCB_BASE_ADDR+offset, value)
#define _SCB_REG_READ(offset) IO_READ_32(TROPIC01_MEMORY_MAP_SCB_BASE_ADDR+offset)

#define FORM_OP(op, src, dst, cidr, shin, aeiv, aesl, aedr, data_size) \
    ((op) << SCB_MODE_OP_POS)                                     |    \
    ((src) << SCB_MODE_SRC_POS)                                   |    \
    ((dst) << SCB_MODE_DST_POS)                                   |    \
    ((cidr) << SCB_MODE_CIDR_POS)                                 |    \
    ((shin) << SCB_MODE_SHIN_POS)                                 |    \
    ((aeiv) << SCB_MODE_AEIV_POS)                                 |    \
    ((aesl) << SCB_MODE_AESL_POS)                                 |    \
    ((aedr) << SCB_MODE_AEDR_POS)                                 |    \
    ((data_size) << SCB_MODE_DATA_SIZE_POS)                       |    \
    ((_ctx.force_aes_mask) << SCB_MODE_FORCE_AES_MASK_POS)

#define FORM_OP_MOV(src, dst)       FORM_OP(SCB_MODE_OP_MOV,    src, dst, 0, 0, 0, 0, 0, 0)
#define FORM_OP_KRD()               FORM_OP(SCB_MODE_OP_KRD,    0, 0, 0, 0, 0, 0, 0, 0)
#define FORM_OP_CI(cidr)            FORM_OP(SCB_MODE_OP_CI,     0, 0, cidr, 0, 0, 0, 0, 0)
#define FORM_OP_SHA(shin)           FORM_OP(SCB_MODE_OP_SHA,    0, 0, 0, shin, 0, 0, 0, 0)
#define FORM_OP_XOR(src, dst)       FORM_OP(SCB_MODE_OP_XOR,    src, dst, 0, 0, 0, 0, 0, 0)
#define FORM_OP_AES_AI(iv,aedr)     FORM_OP(SCB_MODE_OP_AES_AI, 0, 0, 0, 0, iv, 0, aedr, 0)
#define FORM_OP_AES_AD(aesl,size)   FORM_OP(SCB_MODE_OP_AES_AD, 0, 0, 0, 0, 0, aesl, 0, size)
#define FORM_OP_AES_ED(aedr,size)   FORM_OP(SCB_MODE_OP_AES_ED, 0, 0, 0, 0, 0, 0, aedr, size)

#define FORM_KBUS_PARAMS(kslot, ktype, koff)                \
    ((kslot) << SCB_KBUS_PARAMS_KSLOT_POS) | \
    ((ktype) << SCB_KBUS_PARAMS_KTYPE_POS)                | \
    ((koff) << SCB_KBUS_PARAMS_KOFF_POS)


#define _HMAC_OPAD_VALUE 0x5c // opad = 64 bytes of 0x5C
#define _HMAC_IPAD_VALUE 0x36 // ipad = 64 bytes of 0x36

#define _SCB_DEFAULT_TIMEOUT 5000 // [us]

#define _SCB_REG_SIZE 32 // number of bytes

#define BITS_TO_BYTES(bits) ((bits) >> 3)

typedef struct {
    scb_handshake_context_t *hsk;
    scb_task_kind_e     task_kind;
    int                 hsk_step;
    int                 force_aes_mask;
    ts_bool             op_done;
} t_scb_ctx;

t_scb_ctx _ctx;

typedef void (*t_step_handler[SCB_HSK_NUM_STEPS])(void);
static ts_bool _scb_mode_cpb;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions
////////////////////////////////////////////////////////////////////////////////////////////////////

static void _copy_regs_to_mem(u8 *dest, u32 addr, size_t size)
{
    size = (size + 3) & ~3; // align to u32
    sys_copy_regs_to_mem(dest, TROPIC01_MEMORY_MAP_SCB_BASE_ADDR + addr, size);
}

static void _copy_mem_to_regs(u32 addr, u8 *src, size_t size)
{
    size = (size + 3) & ~3; // align to u32
    sys_copy_mem_to_regs(TROPIC01_MEMORY_MAP_SCB_BASE_ADDR + addr, src, size);
}

static void _start_op(u32 params)
{
    _ctx.op_done = TS_FALSE;
    _SCB_REG_WRITE(SCB_MODE_ADDR, params);
    _SCB_REG_WRITE(SCB_COMMAND_ADDR, (SCB_COMMAND_EXEC_ACTION << SCB_COMMAND_OPEXEC_POS) +
                                     (SCB_COMMAND_NO_ACTION << SCB_COMMAND_NONINC_POS) +
                                     (SCB_COMMAND_NO_ACTION << SCB_COMMAND_NONCLR_POS));
}

static void _set_comp_data(u8 *data)
{
    _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, data, 32);
}

static void _fill_data_in(u8 value)
{
    u32 value32 = (value << 24) + (value << 16) + (value << 8) + value;

    for (int addr = SCB_COMP_DATA_IN_0_ADDR; addr <= SCB_COMP_DATA_IN_7_ADDR; addr += sizeof(u32))
    {
        _SCB_REG_WRITE(addr, value32);
    }
}

static void _set_sha256_data_len(size_t data_length)
{
    u32 tmp;
    data_length <<= 3; // convert number of bytes to number of bits
    // convert little endian u16 to BIG endian u32 (here we support max 16 bit number)
    tmp = ((data_length & 0xFF00) << 8) + ((data_length & 0x00FF) << 24);
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_7_ADDR, tmp);
}

static void _set_padding_data(size_t data_length)
{   // whole 32B as SHA256 padding
    // 1) add padding character 0x80
    // 2) fill 0x00 except space for length
    // 3) add number as data length in bits
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, SHA256_PADDING_CHARACTER);

    for (int addr = SCB_COMP_DATA_IN_1_ADDR; addr <= SCB_COMP_DATA_IN_6_ADDR; addr += sizeof(u32))
    {
        _SCB_REG_WRITE(addr, 0);
    }
    _set_sha256_data_len(data_length);
}

static void _set_empty_padding(void)
{   // only padding character and clear everything else
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, SHA256_PADDING_CHARACTER);
    for (int addr = SCB_COMP_DATA_IN_1_ADDR; addr <= SCB_COMP_DATA_IN_7_ADDR; addr += sizeof(u32))
    {
        _SCB_REG_WRITE(addr, 0);
    }
}


static ts_bool _condition_op_done(void)
{
    return (_ctx.op_done);
}

static ts_bool _process_op(u32 op)
{
    _start_op(op);
    return (os_wait_for(_condition_op_done, _SCB_DEFAULT_TIMEOUT));
}

static void _load_data_chunk(u8 *data, size_t len)
{
    if (len >= _SCB_REG_SIZE)
    {
        _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, data, _SCB_REG_SIZE);
    }
    else
    {   // less than 32B of data
        u8 buf[_SCB_REG_SIZE];
        size_t l;
        // copy the data
        for (l=0; l<len; l++)
        {
            buf[l] = data[l];
        }
        // add padding character
        buf[l++] = SHA256_PADDING_CHARACTER;
        // fill rest of space by zero
        for (; l<_SCB_REG_SIZE; l++)
        {
            buf[l] = 0;
        }
        _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, buf, _SCB_REG_SIZE);
    }
}

void _load_hmac_message_data(u8 *data, size_t len)
{   // load data to RA and RB + padding
    // here will be only 3 types of 'len': 1 or 32 or 33
    if (len > (2*_SCB_REG_SIZE))
        return; // does not fit into RA + RB

    _load_data_chunk(data, len);
    // put first part to RA
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA));

    data+=_SCB_REG_SIZE;

    if (len == (2*_SCB_REG_SIZE))
    {
        _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, data, _SCB_REG_SIZE);
        _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB));
        return; // no padding, data only
    }
    else if (len >= _SCB_REG_SIZE)
    {   // more than 32B of data
        _load_data_chunk(data, len-_SCB_REG_SIZE);
    }
    else
    {
        _fill_data_in(0);
    }
    _set_sha256_data_len(64 + len);
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB));
}

#define CHECK(func)  if((func) != TS_TRUE){return;}

static void _step_sha(void)
{   // HASH step 1 
    // 1. COMP_DATA_IN_* = protocol_name.
    _set_comp_data((u8*)PROTOCOL_NAME);
    // 2. OP_MOV (src = COMP_DATA_IN, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA)));
    // 3. COMP_DATA_IN_* = (1 << 255) ∨ sha256_padding(256)
    _set_padding_data(BITS_TO_BYTES(256));
    // 4. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 5. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // result stored in RC we use in next step

    // HASH step 2
    // 1. OP_MOV (src = RC, dst = RA) we use the result from previous step
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RA)));
    // 2. OP_KRD(ktype = s_hipub , kslot = PairingKeySlot, koffset = 0)
    _SCB_REG_WRITE(SCB_KBUS_PARAMS_ADDR, FORM_KBUS_PARAMS(_ctx.hsk->pkey_index, KDB_KEY_TYPE_SHIPUB, 0));
    CHECK( _process_op(FORM_OP_KRD()));
    // 3. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // 4. COMP_DATA_IN_* = 0x80
    _set_empty_padding();
    // 5. OP_MOV (src = COMP_DATA_IN, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA)));
    // 6. COMP_DATA_IN_* = sha256_padding(512)
    _set_padding_data(BITS_TO_BYTES(512));
    // overwrite first word where is padding character, because we have padding character in previous block
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, 0);
    // 7. OP_MOV (src = COM P_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 8. OP_SHA(init = 0)
    CHECK( _process_op(FORM_OP_SHA(0)));
    // result stored in RC we use in next step

    // HASH step 3
    // 1. OP_MOV (src = RC, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RA)));
    // 2. OP_KRD(ktype = s_tpub , kslot = 0, koffset = 0)
    _SCB_REG_WRITE(SCB_KBUS_PARAMS_ADDR, FORM_KBUS_PARAMS(0, KDB_KEY_TYPE_STPUB, 0));
    CHECK( _process_op(FORM_OP_KRD()));
    // 3. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // 4. COMP_DATA_IN_* = 0x80
    _set_empty_padding();
    // 5. OP_MOV (src = COMP_DATA_IN, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA)));
    // 6. COMP_DATA_IN_* = sha256_padding(512)
    _set_padding_data(BITS_TO_BYTES(512));
    // overwrite first word where is padding character, because we have padding character in previous block
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, 0);
    // 7. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 8. OP_SHA(init = 0)
    CHECK( _process_op(FORM_OP_SHA(0)));
    // result stored in RC we use in next step

    // HASH step 4
    // 1. OP_MOV (src = RC, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RA)));
    // 2. COMP_DATA_IN_* = e_hpub
    _set_comp_data(_ctx.hsk->e_hpub);
    // 3. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 4. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // 5. COMP_DATA_IN_* = 0x80 0x00 ...
    _set_empty_padding();
    // 6. OP_MOV (src = COMP_DATA_IN, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA)));
    // 7. COMP_DATA_IN_* = sha256_padding(512)
    _set_padding_data(BITS_TO_BYTES(512));
    // overwrite first word where is padding character, because we have padding character in previous block
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, 0);
    // 8. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 9. OP_SHA(init = 0)
    CHECK( _process_op(FORM_OP_SHA(0)));
    // result stored in RC we use in next step

    // HASH step 5
    // 1. OP_MOV (src = RC, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RA)));
    // 2. COMP_DATA_IN_* = PKEY_INDEX || padding
    _set_padding_data(BITS_TO_BYTES(256+8));
    // overwrite first word and update padding character
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, _ctx.hsk->pkey_index | (SHA256_PADDING_CHARACTER << 8));
    // 3. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 4. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // result stored in RC we use in next step

    // HASH step 6
    // 1. OP_MOV (src = RC, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RA)));
    // 2. COMP_DATA_IN_* = e_tpub
    _set_comp_data(_ctx.hsk->e_tpub);
    // 3. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 4. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // 5. COMP_DATA_IN_* = 0x80 0x00 ...
    _set_empty_padding();
    // 6. OP_MOV (src = COMP_DATA_IN, dst = RA)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA)));
    // 7. COMP_DATA_IN_* = sha256_padding(512)
    _set_padding_data(BITS_TO_BYTES(512));
    // overwrite first word where is padding character, because we have padding character in previous block
    _SCB_REG_WRITE(SCB_COMP_DATA_IN_0_ADDR, 0);
    // 8. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 9. OP_SHA(init = 0)
    CHECK( _process_op(FORM_OP_SHA(0)));
    // 10. OP_MOV (src = RC, dst = HSK_HASH)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_HSH_HASH)));
    // After Step 6 Secure Channel Handshake Hash can be read from HSK_HASH
    _copy_regs_to_mem((u8 *)&_ctx.hsk->hash, SCB_HSK_HASH_0_ADDR, SCB_HASH_SIZE);
    // not yet TAG ... continue HKDF :(
}


static void _hmac_sha256(u8 *message, size_t len)
{   // HMAC−SHA256(K, message) = SHA256((K XOR  opad) || SHA256((K XOR ipad) || message))
    // assume 'K' is already placed in RA
    // 1. OP_MOV (src = RA, dst = RD) Backup K to RD
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_RD)));
    // 2. COMP_DATA_IN_* = ipad
    _fill_data_in(_HMAC_IPAD_VALUE);
    // 3. OP_XOR(src = RA, dst = RA) RA = RA XOR ipad
    CHECK( _process_op(FORM_OP_XOR(SCB_MODE_SRC_RA, SCB_MODE_DST_RA)));
    // 4. OP_MOV (src = COMP_DATA_IN, dst = RB) RB = ipad
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 5. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // 6. Load 'message' and SHA256 padding to RA nad RB
    _load_hmac_message_data(message, len);
    // 7. OP_SHA(init = 0) RC = SHA256(K XOR ipad ∥ message)
    CHECK( _process_op(FORM_OP_SHA(0)));
    // 8. OP_MOV (src = RD, dst = RA)  Restore RD to RA
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RD, SCB_MODE_DST_RA)));
    // 9. COMP_DATA_IN_* = opad
    _fill_data_in(_HMAC_OPAD_VALUE);
    // 10. OP_XOR(src = RA, dst = RA) RA = RA XOR opad
    CHECK( _process_op(FORM_OP_XOR(SCB_MODE_SRC_RA, SCB_MODE_DST_RA)));
    // 11. OP_MOV (src = COMP_DATA_IN, dst = RB) RB = opad
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 12. OP_MOV (src = RC, dst = RD) Backup RC (result of inner SHA) to RD
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RD)));
    // 13. OP_SHA(init = 1)
    CHECK( _process_op(FORM_OP_SHA(1)));
    // 14. OP_MOV (src = RD, dst = RA) Restore RD (result of inner SHA) to RA
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RD, SCB_MODE_DST_RA)));
    // 15. COMP_DATA_IN_* = {B0 = 0x80, B30 = 0x03, Others = 0x0} Padding (768 bits)
    _set_padding_data(BITS_TO_BYTES(768));
    // 16. OP_MOV (src = COMP_DATA_IN, dst = RB)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB)));
    // 17. OP_SHA(init = 0) RC = HMAC-SHA256(K, message)
    CHECK( _process_op(FORM_OP_SHA(0)));
    // 18. OP_MOV (src = RC, dst = RA) RA = RC
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RA)));
}

static void _hkdf(u8 *message, size_t len, int nouts)
{
    u8 tmp[SCB_KEY_SIZE + 1];

    // assume RA = ck

    // Compute HMAC−SHA256(K = ck, m = input) RA = tmp
    _hmac_sha256(message, len);
    // OP_MOV (src = RA, dst = RE) Backup tmp to RE.
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_RE)));
    // Compute HMAC−SHA256(K = tmp, m = 0x01)  RA = HMAC−SHA256(tmp, 0x01)
    _hmac_sha256((u8 *)"\x01", 1);

    // If nouts == 1, then finish. Result output_1 is now in RA. It does not need to be read
    if (nouts == 1)
    {
        return; // result "output_1" is in RA
    }
    // from SCB since it is only needed as input for follow-up HKDF calculations.
    // read the output_1
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_COMP_DATA_OUT)));
    _copy_regs_to_mem(tmp, SCB_COMP_DATA_OUT_0_ADDR, SCB_KEY_SIZE);
    tmp[SCB_KEY_SIZE] = 0x02; // concatenate output_1 and character 0x02
    // OP_MOV (src = RA, dst = RC) RC=output_1
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_RC)));
    // OP_MOV (src = RE, dst = RA) RA=tmp
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RE, SCB_MODE_DST_RA)));
    // OP_MOV (src = RC, dst = RE) RE=output_1 (backup)
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_RE)));
    // Compute HMAC−SHA256(K = tmp, m = output_1 ∥ 0x02) as in 8.4. For this
    _hmac_sha256(tmp, SCB_KEY_SIZE + 1);
    // computation tmp is now in RA and output_1 is in RE. Thus FW loading message m
    // proceeds as in the case when loading 33 byte long message. After hits computa-
    // tion RA contains output_2.
    // OP_MOV (src = RA, dst = RB) RB=output_2
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_RB)));
    // OP_MOV (src = RE, dst = RA) RA=output_1
    CHECK( _process_op(FORM_OP_MOV(SCB_MODE_SRC_RE, SCB_MODE_DST_RA)));
    // result "output_1" is in RA and "output_2" in RB
}
#undef CHECK

static void _step_hkdf_1(void)
{
    // copy 'ck' to RA as 'K'
    _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, (u8 *)PROTOCOL_NAME, SCB_KEY_SIZE);
    if (_process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA)) != TS_TRUE)
    {
        return;
    }
    _hkdf(_ctx.hsk->x25519, SCB_KEY_SIZE, 1);
    // keep result in RA
}

static void _step_hkdf_2(void)
{
    // we have 'ck' in RA from previous step
    // new data in _ctx.hsk->x25519
    _hkdf(_ctx.hsk->x25519, SCB_KEY_SIZE, 1);
}

static void _step_hkdf_3(void)
{
    // we have 'ck' in RA from previous step
    // new data in _ctx.hsk->x25519
    _hkdf(_ctx.hsk->x25519, SCB_KEY_SIZE, 2);
    // copy k_auth to its destination
    // OP_MOV (src = RB, dst = k_res ) k_res = RB
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_RB, SCB_MODE_DST_KRES));
    // OP_MOV (src = RA, dst = RE) Backup ck to RE.
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_RE));
}

static void _step_hkdf_4(void)
{
    // move 'ck' to RA
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_RE, SCB_MODE_DST_RA));
    // use _ctx.hsk->x25519 as buffer
    memset(_ctx.hsk->x25519, 0, SCB_KEY_SIZE); // "emptystring"
    _hkdf(_ctx.hsk->x25519, 0, 2);
    // RA is k_cmd, RB is k_res
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_RA, SCB_MODE_DST_KCMD));
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_RB, SCB_MODE_DST_KRES));
    // RA and RB contain keys, clear them
    _fill_data_in(0);
    // RA is k_cmd, RB is k_res
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_SRC_RA));
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_SRC_RB));
    // Clear nonce
    scb_nonce_clear();
}

static void _step_tag_1(void)
{
    // 1. COMP_DATA_IN_* = (0,0,...,0,0)
    _fill_data_in(0);
    // 2. OP _M OV (src = COMP_DATA_IN, dst = RA)
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA));
    // 3. OP_AES_AI(iv = 0, enc/aedr = 1)
    _process_op(FORM_OP_AES_AI(0, SCB_ENCRYPT));
    // 4. OP_MOV (src = HSK_HASH, dst = RA)
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_HSK_HASH, SCB_MODE_DST_RA));
    // 5. OP_AES_AD(size = 16, sel = 0)
    _process_op(FORM_OP_AES_AD(0, SCB_AES_CHUNK_SIZE));
    // 6. OP_AES_AD(size = 16, sel = 1)
    _process_op(FORM_OP_AES_AD(1, SCB_AES_CHUNK_SIZE));
    // 7. OP_AES_TG()
    _process_op(SCB_MODE_OP_AES_TG << SCB_MODE_OP_POS);
}

const t_step_handler _hsk_steps = {
    _step_sha,
    _step_hkdf_1,
    _step_hkdf_2,
    _step_hkdf_3,
    _step_tag_1,
    _step_hkdf_4
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////////////////////////

void scb_init(void)
{
    memset(&_ctx, 0, sizeof(_ctx));
    _scb_mode_cpb = TS_FALSE;

    // SCB has its own clock enable bit
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_SCBCLKEN_MASK | SOC_CTRL_CLK_EN_KDBCLKEN_MASK | SOC_CTRL_CLK_EN_EDBCLKEN_MASK);

    _SCB_REG_WRITE(SCB_INT_EN_ADDR, (1 << SCB_INT_EN_OPDEN_POS));
    //
    _LOG_DEBUG("init 0x%x",  _SCB_REG_READ(SCB_BLOCK_ID_ADDR));

    scb_suspend();
}

void scb_reset(void)
{
    soc_ctrl_reset(SOC_CTRL_UTRESET_SCBRST_MASK);
}

void scb_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_SCBCLKEN_MASK);
}

void scb_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_SCBCLKEN_MASK);
}


ts_bool scb_handshake_step(scb_handshake_context_t *ctx, scb_handshake_step_e step)
{   // there is multiple steps needed for performing handshake
    // these steps must be combined with SPECT operation steps
    // so external application (secchnl.c) drives these steps

    OS_ASSERT(step >= 0);
    OS_SANITY_NULL(ctx);

    if (step >= SCB_HSK_NUM_STEPS)
    {
        return (TS_FALSE);
    }
    _ctx.task_kind = SCB_TASK_HANDSHAKE;
    _ctx.hsk_step = step;
    _ctx.hsk = ctx;

    _LOG_DEBUG("hsk step %d", step);
    _hsk_steps[_ctx.hsk_step]();

    if (_ctx.op_done != TS_TRUE)
    {
        _LOG_DEBUG("failed, %lx, %lx", _SCB_REG_READ(SCB_STATUS_ADDR), _SCB_REG_READ(SCB_COMMAND_ADDR));
    }
    _ctx.task_kind = SCB_TASK_NONE;
    return (_ctx.op_done);
}

void scb_tag_read(u8 tag[SCB_TAG_SIZE])
{
    OS_SANITY_NULL(tag);
     _copy_regs_to_mem(tag, SCB_TAG_0_ADDR, SCB_TAG_SIZE);
}

void scb_enc_dec_init(scb_ed_dir_e ed_dir)
{
    OS_SANITY((ed_dir == SCB_DECRYPT) || (ed_dir == SCB_ENCRYPT));

    _ctx.task_kind = SCB_TASK_ENC_DEC;
    // 1. OP_AES_AI(iv = 1)
    _process_op(FORM_OP_AES_AI(1, ed_dir));
}

void scb_decrypt_data(u8 *plaintext, u8 *ciphertext, size_t size)
{
    u32 offset = 0;

    OS_SANITY_NULL(ciphertext);
    // NOTE: plaintext == NULL is valid value

    for (size_t i=0; i<size; i += SCB_AES_CHUNK_SIZE, ciphertext += SCB_AES_CHUNK_SIZE)
    {
        int chunk_size = size-i;
        if (chunk_size > SCB_AES_CHUNK_SIZE)
        {
            chunk_size = SCB_AES_CHUNK_SIZE;
        }
        cpb_command_pointer(offset);

        // 2. COMP_DATA_IN_* = Up-to 16-byte chunk of L3 Command to encrypt.
        _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, ciphertext, chunk_size);
        // 3. OP_MOV (src = COMP_DATA_IN, dst = RA)
        _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA));
        // 4. OP_AES_ED(enc = 0, size = chunk_size, 0)
        _process_op(FORM_OP_AES_ED(0, chunk_size));
        // 5. OP_CI(dir = 1) Execute transfer to Command Interface
        _process_op(FORM_OP_CI(1));
       
        // read the data only in case they are needed for CPU processing
        if ((_scb_mode_cpb == TS_FALSE) && (plaintext != NULL))
        {
            cpb_read_data(plaintext, offset, chunk_size);
            plaintext += SCB_AES_CHUNK_SIZE;
        }
        if (offset <= (CPB_COMMAND_BUFFER_SIZE - 2*SCB_AES_CHUNK_SIZE))
        {
            offset += SCB_AES_CHUNK_SIZE;
        }
        // NOTE: we keep maximum data what fits into CPB in buffer
    }
}

void scb_encrypt_data(u8 *ciphertext, u8 *plaintext, size_t size)
{
    u32 offset = 0;

    OS_SANITY_NULL(ciphertext);

    for (size_t i=0; i<size; i += SCB_AES_CHUNK_SIZE, ciphertext += SCB_AES_CHUNK_SIZE)
    {
        size_t chunk_size = size-i;
        if (chunk_size > SCB_AES_CHUNK_SIZE)
        {
            chunk_size = SCB_AES_CHUNK_SIZE;
        }
        cpb_result_pointer(offset);

        // copy data only in case they are not in CPB already
        if ((_scb_mode_cpb == TS_FALSE) && (plaintext != NULL))
        {
            cpb_write_data(plaintext, offset, chunk_size);
            plaintext += SCB_AES_CHUNK_SIZE;
        }
        // 2. OP_CI(dir = 0) Execute transfer from Command Interface
        _process_op(FORM_OP_CI(0));
        // 3. OP_AES_ED(enc = 1, size = chunk_size, 0)
        _process_op(FORM_OP_AES_ED(1, chunk_size));
        // 4. OP_MOV (src = RC, dst = COMP_DATA_OUT )
        _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_COMP_DATA_OUT));
        // 5. Read encrypted result chunk from COMP_DATA_OUT_*.
        _copy_regs_to_mem(ciphertext, SCB_COMP_DATA_OUT_0_ADDR, chunk_size);

        if (offset <= (CPB_RESULT_BUFFER_SIZE - 2*SCB_AES_CHUNK_SIZE))
        {
            offset += SCB_AES_CHUNK_SIZE;
        }
    }
    // return operation mode to default
    _scb_mode_cpb = TS_FALSE; 
}

void scb_enc_dec_finish(u8 result_tag[SCB_TAG_SIZE])
{
    OS_SANITY_NULL(result_tag);
    
     if (_process_op(SCB_MODE_OP_AES_TG << SCB_MODE_OP_POS) != TS_TRUE)
     {
        return;
     }
     // TAG ready from the decrypted L3 Command.
     scb_tag_read(result_tag);
    _ctx.task_kind = SCB_TASK_NONE;
}

void scb_set_cpb_mode(ts_bool state)
{
    _scb_mode_cpb = state;
}

void scb_nonce_increment(void)
{
    _SCB_REG_WRITE(SCB_COMMAND_ADDR, (SCB_COMMAND_EXEC_ACTION << SCB_COMMAND_NONINC_POS) +
                                     (SCB_COMMAND_NO_ACTION << SCB_COMMAND_OPEXEC_POS) +
                                     (SCB_COMMAND_NO_ACTION << SCB_COMMAND_NONCLR_POS));
}

void scb_nonce_clear(void)
{
    _SCB_REG_WRITE(SCB_COMMAND_ADDR, (SCB_COMMAND_EXEC_ACTION << SCB_COMMAND_NONCLR_POS) +
                                     (SCB_COMMAND_NO_ACTION << SCB_COMMAND_OPEXEC_POS) +
                                     (SCB_COMMAND_NO_ACTION << SCB_COMMAND_NONINC_POS));
}

u32 scb_nonce_get(void)
{
    return (_SCB_REG_READ(SCB_NONCE_ADDR));
}

void scb_sha256_round(ts_bool init, u8 data[SCB_HASH_ROUND_SIZE])
{
    OS_SANITY_NULL(data);

    // copy data to RA || RB registers
    _ctx.task_kind = SCB_TASK_SHA;
    _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, data, _SCB_REG_SIZE);
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RA));
    _copy_mem_to_regs(SCB_COMP_DATA_IN_0_ADDR, data+_SCB_REG_SIZE, _SCB_REG_SIZE);
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, SCB_MODE_DST_RB));

    // execute operation, result to RC
    if (init == TS_TRUE)
    {
        _process_op(FORM_OP_SHA(1));
    }
    else
    {
        _process_op(FORM_OP_SHA(0));
    }
    _ctx.task_kind = SCB_TASK_NONE;
}

void scb_sha256_read(u8 hash[SCB_HASH_SIZE])
{
    _ctx.task_kind = SCB_TASK_SHA;
    _process_op(FORM_OP_MOV(SCB_MODE_SRC_RC, SCB_MODE_DST_COMP_DATA_OUT));
    _copy_regs_to_mem(hash, SCB_COMP_DATA_OUT_0_ADDR, SCB_HASH_SIZE);
    _ctx.task_kind = SCB_TASK_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test wrappers to access internal functions in tests
////////////////////////////////////////////////////////////////////////////////////////////////////
void scb_tstwrp_set_comp_data(u8 *data) 
{
    _set_comp_data(data);
}

bool scb_tstwrp_process_op(u32 op) 
{
    _ctx.task_kind = SCB_TASK_ENC_DEC;
    bool ret = _process_op(op) == TS_TRUE ? true : false;
    _ctx.task_kind = SCB_TASK_NONE;
    return ret;
}

bool scb_tstwrp_mov_data_in(u8 dest) 
{
    _ctx.task_kind = SCB_TASK_ENC_DEC;
    bool ret = _process_op(FORM_OP_MOV(SCB_MODE_SRC_COMP_DATA_IN, dest)) == TS_TRUE ? true : false;
    _ctx.task_kind = SCB_TASK_NONE;
    return ret;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt Handler
////////////////////////////////////////////////////////////////////////////////////////////////////

__ISR void irq_scb_handler(void)
{
    _SCB_REG_WRITE(SCB_STATUS_ADDR, SCB_STATUS_OPD_MASK); // clear the IRQ request
    switch (_ctx.task_kind)
    {
    case SCB_TASK_HANDSHAKE:
    case SCB_TASK_ENC_DEC:
    case SCB_TASK_SHA:
        break;

    case SCB_TASK_NONE:
    default:
        LOG_ERROR_NUM(_SCB_ERR_ISR);
        os_alarm_isr();
        return;
    }
    _ctx.op_done = TS_TRUE;
}

