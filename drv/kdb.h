
/**
 * @file kdb.h
 * @author Tropic Square
 * @brief KDB (Key Distribution Block) driver header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef KDB_H
#define KDB_H

#define KDB_NUM_CONFIG_MEM_ENTRIES 7

/**
 * @name KDB config masks
 * @note These defines are generated but not yet part of the exported CSP!
 */
///@{
#define KDB_CONFIG_MEM_0_KEY_TYPE_MASK GENMASK(7, 0)
#define KDB_CONFIG_MEM_0_PUF_OFFSET_MASK GENMASK(15, 8)
#define KDB_CONFIG_MEM_0_SLOT_SIZE_MASK GENMASK(19, 16)
#define KDB_CONFIG_MEM_0_PUFLP_MASK BIT(24)

#define KDB_CONFIG_MEM_1_PUF_CHALLENGE_MASK GENMASK(20, 0)
///@}


typedef enum {
    KDB_KEY_TYPE_STPRIV = 0, /**< TROPIC01 Private Key for Secure Channel Handshake. */
    KDB_KEY_TYPE_STPUB  = 1, /**< TROPIC01 Public Key for Secure Channel Handshake. */
    KDB_KEY_TYPE_SHIPUB = 2, /**< Host MCU Public Key for Secure Channel Handshake from Pairing Key Slot i. */
    KDB_KEY_TYPE_KFXA   = 3, /**< A Key for MAC-and-Destroy sequence (F1 KMAC). */
    KDB_KEY_TYPE_ECC    = 4, /**< A key for ECC operations in SPECT. ECDSA/EDDSA key generation and signing. */
    KDB_KEY_TYPE_KFENC  = 5, /**< A key for encryption of Flash memory content. */
    KDB_KEY_TYPE_KFXB   = 6, /**< A Key for MAC-and-Destroy sequence (F2 KMAC). */
} kdb_key_type_e;

typedef struct {
    /** @brief Key type identifier on KBUS representing this key type! */
    u8      key_type;

    /** @brief Byte offset within the key to request the key from PUF, and not from OTP / Flash. */
    u8      puf_offset;

    /** @brief Size of the slot in the destination memory in exponential notation:
     *  - 0 - 1 byte
     *  - 1 - 2 bytes
     *  - 2 - 4 bytes
     *  - ...
     *  - 16 - 65536 bytes
     *  shall be between 0 and 16 (included).
     */
    u8      slot_size_exp:4;
    u8      unused3:4;

    /**
     * @brief puf_low_part = 0 - If (Byte offset within the key >= puf_offset) -> Read word of a key from PUF
     *                    else -> Read word of a key from OTP / Flash Memory.
     * @brief puf_low_part = 1 - If (Byte offset within the key >= puf_offset) -> Read word of a key from OTP / Flash Memory
     *                    else -> Read word of a key from PUF.
     */
    u8      puf_low_part:1;
    u8      unused4:7;

    /** @brief Challenge for PUF if part of the key is read from PUF.
     * Only LSB 20 bits are taken. Remaining 12 bits of challegen are constructed from
     * Key slot and Key offset within slot!
     */
    u32     puf_challenge;

    /** @brief Base address of the first slot in OTP ? Flash memory where this key_type shall be read from. */
    u32     key_base_address;
} __PACK kdb_cfg_mem_entry_t;

typedef enum {
    KDB_DEBUG_MODE   = 0x1,
    KDB_NORMAL_MODE  = 0x2,

    /** @brief Keep the default mode set in the HW. */
    KDB_DONT_SET_MODE = 0xFF
} kdb_mode_e;

typedef struct {
    /** @brief KDB mode. */
    kdb_mode_e      mode;

    /** @brief If set to Debug mode, shall wait on incoming KBUS transfer in Debug mode, and
    fire an interrupt. Ignored if Debug mode is not set. */
    u8              debug_wait;

    /** @brief Call-back to be called from Interrupt context to return the next data to be
    provided on the Key Bus. Set to NULL if Debug mode is not used. */
    void            (*debug_cb)(u32 *debug_data, u32 *debug_error_flag);
} kdb_config_t;

/**
 * @brief Enable clock for Key Distribution Block.
 */
void kdb_wakeup(void);

/**
 * @brief Disable clock for Key Distribution Block.
 */
void kdb_suspend(void);

/**
 * @brief Initialize Key distribution Block.
 */
void kdb_init(const kdb_config_t *cfg, const kdb_cfg_mem_entry_t *cfg_mem, int n_cfg_mem_entries);

#endif // ! KDB_H
