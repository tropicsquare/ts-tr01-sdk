/**
 * @file scb.h
 * @author Tropic Square
 * @brief SCB (Secure Channel Block) driver header file.
 *
 * This file contains macros, structures, enumerations, and function
 * declarations for the Secure Channel Block (SCB) operations.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SCB_H
#define SCB_H

#include "common.h"

/** @defgroup SCB_Constants SCB Constants */
/**@{*/
#define SCB_TAG_SIZE          (16) /**< Size of authentication tag. */
#define SCB_AES_CHUNK_SIZE    (16) /**< AES encryption chunk size. */
#define SCB_KEY_SIZE          (32) /**< Size of cryptographic keys. */
#define SCB_HASH_SIZE         (32) /**< Hash size (SHA-256 output). */
#define SCB_HASH_ROUND_SIZE   (64) /**< SHA-256 round buffer size. */
/**@}*/

/**
 * @brief Handshake context structure.
 *
 * Stores key material and intermediate results for SCB handshake operations.
 */
typedef struct {
    u8 pkey_index;                 /**< Private key index. */
    u8 e_hpub[SCB_KEY_SIZE];       /**< Encrypted public key of host. */
    u8 e_tpub[SCB_KEY_SIZE];       /**< Encrypted public key of target. */
    u8 x25519[SCB_KEY_SIZE];       /**< Temporary result of X25519 operation. */
    u8 hash[SCB_HASH_SIZE];        /**< Handshake hash. */
} scb_handshake_context_t;

/**
 * @brief Enumeration for SCB handshake steps.
 */
typedef enum {
    SCB_HSK_STEP_SHA = 0,   /**< Handshake step 1, multiple SHA256 */
    SCB_HSK_STEP_HKDF_1,    /**< HKDF step 1. */
    SCB_HSK_STEP_HKDF_2,    /**< HKDF step 2. */
    SCB_HSK_STEP_HKDF_3,    /**< HKDF step 3. */
    SCB_HSK_STEP_TAG,     /**< Tag step 1. */
    SCB_HSK_STEP_HKDF_4,    /**< HKDF step 4. */

    SCB_HSK_NUM_STEPS   /**< Number of handshake steps. */
} scb_handshake_step_e;

/**
 * @brief Enumeration for encryption/decryption direction.
 */
typedef enum {
    SCB_DECRYPT = 0, /**< Decryption operation. */
    SCB_ENCRYPT = 1  /**< Encryption operation. */
} scb_ed_dir_e;

/**
 * @brief Enumeration for SCB task types.
 */
typedef enum {
    SCB_TASK_NONE,       /**< No active task. */
    SCB_TASK_HANDSHAKE,  /**< Handshake process task. */
    SCB_TASK_ENC_DEC,    /**< Encryption/Decryption task. */
    SCB_TASK_SHA         /**< SHA processing task. */
} scb_task_kind_e;


/**
 * @brief Initializes the SCB module.
 */
void scb_init(void);

/**
 * @brief Resets the SCB module.
 */
void scb_reset(void);

/**
 * @brief Suspends SCB operation.
 */
void scb_suspend(void);

/**
 * @brief Wakes up SCB from suspension.
 */
void scb_wakeup(void);

/**
 * @brief Executes a handshake step.
 * @param ctx Pointer to handshake context.
 * @param step Step number.
 * @return True if step completed successfully, false otherwise.
 */
ts_bool scb_handshake_step(scb_handshake_context_t *ctx, scb_handshake_step_e step);

/**
 * @brief Reads authentication tag.
 * @param tag Buffer to store the tag.
 */
void scb_tag_read(u8 tag[SCB_TAG_SIZE]);

/**
 * @brief Initializes encryption or decryption.
 * @param ed_dir Direction: encrypt or decrypt.
 */
void scb_enc_dec_init(scb_ed_dir_e ed_dir);

/**
 * @brief Decrypts data.
 * @param plaintext Buffer to store decrypted data.
 * @param ciphertext Input ciphertext.
 * @param size Data size in bytes.
 */
void scb_decrypt_data(u8 *plaintext, u8 *ciphertext, size_t size);

/**
 * @brief Encrypts data.
 * @param ciphertext Buffer to store encrypted data.
 * @param plaintext Input plaintext.
 * @param size Data size in bytes.
 */
void scb_encrypt_data(u8 *ciphertext, u8 *plaintext, size_t size);

/**
 * @brief Completes encryption or decryption and retrieves authentication tag.
 * @param result_tag Buffer to store the resulting tag.
 */
void scb_enc_dec_finish(u8 result_tag[SCB_TAG_SIZE]);

/**
 * @brief Sets the CPB mode.
 * @param state True to enable CPB mode, false to disable.
 */
void scb_set_cpb_mode(ts_bool state);

/**
 * @brief Increments the nonce.
 */
void scb_nonce_increment(void);

/**
 * @brief Clears the nonce.
 */
void scb_nonce_clear(void);

/**
 * @brief Retrieves the current nonce value.
 * @return The current nonce value.
 */
u32 scb_nonce_get(void);

/**
 * @brief Performs a SHA-256 round.
 * @param init True to initialize hashing, false to continue.
 * @param data Input data for hashing.
 */
void scb_sha256_round(ts_bool init, u8 data[SCB_HASH_ROUND_SIZE]);

/**
 * @brief Reads the computed SHA-256 hash.
 * @param hash Buffer to store the hash.
 */
void scb_sha256_read(u8 hash[SCB_HASH_SIZE]);

/**
 * @name Internal test functions (temporary)
 *
 * These functions are available temporarily for testing purposes.
 */
///@{
void scb_tstwrp_set_comp_data(u8 *data);
bool scb_tstwrp_process_op(u32 op);
bool scb_tstwrp_mov_data_in(u8 dest);
///@}


#endif // !SCB_H

