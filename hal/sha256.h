/**
 * @file sha256.h
 * @author Tropic Square
 * @brief SHA-256 library header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SHA256_H
#define SHA256_H

#include "type.h"


/// @brief Size (in bytes) of a single SHA-256 data chunk.
#define SHA256_CHUNK_SIZE 64

/**
 * @brief SHA-256 context structure.
 * 
 * This structure holds the state of a SHA-256 computation, including
 * the input buffer, the length of data in the buffer, and the total
 * bit length processed so far.
 */
typedef struct
{
    u8 buf[SHA256_CHUNK_SIZE];  ///< Input buffer to hold data blocks.
    u32 buf_length;             ///< Current length of data in the buffer.
    u32 bit_length;             ///< Total number of bits processed.
} sha256_t;

/**
 * @brief Initializes the SHA-256 context.
 * 
 * Sets up the context to begin a new SHA-256 hashing operation.
 * 
 * @param ctx Pointer to a sha256_t structure to initialize.
 */
void sha256_init(sha256_t *ctx);

/**
 * @brief Suspends the SHA-256 engine (if hardware-based).
 * 
 * Used to power down or pause SHA-256 operations. No-op for software implementations.
 */
void sha256_suspend(void);

/**
 * @brief Wakes up the SHA-256 engine (if hardware-based).
 * 
 * Used to resume SHA-256 operations after suspension. No-op for software implementations.
 */
void sha256_wakeup(void);

/**
 * @brief Updates the SHA-256 context with new input data.
 * 
 * This function processes the given data and updates the internal state.
 * 
 * @param ctx Pointer to an initialized sha256_t context.
 * @param data Pointer to the input data to hash.
 * @param length Length of the input data in bytes.
 */
void sha256_update(sha256_t *ctx, const u8 *data, size_t length);

/**
 * @brief Finalizes the SHA-256 hash computation.
 * 
 * This function processes any remaining data and produces the final hash value.
 * 
 * @param ctx Pointer to the sha256_t context.
 * @param hash Pointer to a buffer where the resulting hash (32 bytes) will be stored.
 */
void sha256_final(sha256_t *ctx, u8 *hash);

#endif // ! SHA256_H
