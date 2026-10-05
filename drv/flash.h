/**
 * @file flash.h
 * @author Tropic Square
 * @brief Flash controller HW driver header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef FLASH_H
#define FLASH_H

#include "type.h"

#define FLASH_SIZE (512*1024)

#define FLASH_EMPTY_VALUE (0xFFFFFFFF)

// Main address space layout, see api/layout_flash.h.

/**
 * @name NVR address space
 * 
 */
///@{
#define FLASH_NVR0_ADDR                     (0 * FLASH_SECTOR_SIZE)
#define FLASH_NVR1_ADDR                     (1 * FLASH_SECTOR_SIZE)
#define FLASH_NVR2_ADDR                     (2 * FLASH_SECTOR_SIZE)
#define FLASH_NVR3_ADDR                     (3 * FLASH_SECTOR_SIZE)
///@}

/**
 * @name NVR page layout
 * @brief According to "TROPIC01 Functional specification".
 * 
 */
///@{
#define FLASH_NVR0_OFFSET_MAGIC_WORD         (0x100)
#define FLASH_NVR0_OFFSET_TRIM_DATA0         (0x1e0)
#define FLASH_NVR0_OFFSET_TRIM_DATA1         (0x1e4)
#define FLASH_NVR0_OFFSET_TRIM_DATA2         (0x1e5)
#define FLASH_NVR0_OFFSET_TRIM_DATA3         (0x1ec)
#define FLASH_NVR0_OFFSET_TRIM_DATA4         (0x1f0)
#define FLASH_NVR0_OFFSET_TRIM_DATA5         (0x1f4)
#define FLASH_NVR0_OFFSET_TRIM_DATA6         (0x1f8)
#define FLASH_NVR0_OFFSET_TRIM_DATA7         (0x1fc)

#define FLASH_NVR1_OFFSET_PUF_MASK_0         (0x000)
#define FLASH_NVR1_OFFSET_PUF_MASK_SYNDROM   (0x080)
#define FLASH_NVR1_OFFSET_TRIM_CTRL          (0x084)
#define FLASH_NVR1_OFFSET_TRIM_CLK           (0x088)
#define FLASH_NVR1_OFFSET_OTP_CHALL          (0x08C)
#define FLASH_NVR1_OFFSET_FLASH_CHALL        (0x090)
#define FLASH_NVR1_OFFSET_PTRNG0_PP          (0x094)
#define FLASH_NVR1_OFFSET_PTRNG1_PP          (0x098)
#define FLASH_NVR1_OFFSET_PUF_CTRL_READ_RATE (0x09C)
#define FLASH_NVR1_OFFSET_VAL_CHIP_ID        (0x0A0)
#define FLASH_NVR1_OFFSET_PUF_INTEGRITY_TEST_CHALL (0x0A4)
#define FLASH_NVR1_OFFSET_PUF_INTEGRITY_RESP0 (0x0A8)
#define FLASH_NVR1_OFFSET_PUF_INTEGRITY_RESP1 (0x0AC)

#define FLASH_NVR1_OFFSET_TRIM_CRC           (0x1FC)
///@}



/**
 * @name Memory element sizes in address space
 * 
 */
///@{
#define FLASH_SECTOR_SIZE                   0x200
#define FLASH_SECTOR_MASK                   0x1ff
#define FLASH_BLOCK_MASK                    0xfff
///@}

/**
 * @name Encryption related defines
 * 
 */
///@{
#define FLASH_MAX_ENCRYPTED_SIZE     476  /**< Width of the SECT_CTEXT field, for struct layout only.
                                               Not a payload limit, see ::FLASH_MAX_ENCRYPTED_PAYLOAD. */

/**
 * @brief Maximum usable encrypted payload in bytes.
 *
 * One less than ::FLASH_MAX_ENCRYPTED_SIZE because of a known, accepted off-by-one in the FSS
 * SECT_DLENGTH comparator: it compares >= 476 where it should compare > 476, so a 476 B payload
 * is encrypted and decrypted correctly but still raises STATUS[ENCRYPT_ERR] on write and
 * STATUS[DECRYPT_ERR] on read. The decision is to honour the flag rather than ignore it, so
 * payloads are capped one byte below the field width.
 *
 * @warning The FSS design specification states 476 B. Do not "restore" this constant to 476
 *          from the specification - the RTL is what deviates from it.
 */
#define FLASH_MAX_ENCRYPTED_PAYLOAD  475
#define FLASH_NONCE_SIZE             16
#define FLASH_ATAG_SIZE              16
///@}

/**
 * @brief Initialize Flash Subsystem.
 *
 * Performs:
 *  - Enables clock for Flash Subsystem
 *  - Configures Flash macro timing
 *  - Reads out and applies Flash macro trim
 *
 * @note You need to call also flash_init_scrambling() during initialization phase
 */
void flash_init(void);

/**
 * @brief Check if the flash subsystem has been initialized.
 * @return TS_TRUE if flash_init() has been successfully completed.
 */
ts_bool flash_init_done(void) TS_CHECK_RETVAL;

/**
 * @brief Disables clock for Flash Subsystem
 */
void flash_suspend(void);

/**
 * @brief Enables clock for Flash Subsystem
 */
void flash_wakeup(void);

/**
 * @brief Disables ECC checking for Flash Subsystem
 */
void flash_ecc_disable(void);

/**
 * @brief Enables ECC checking for Flash Subsystem
 * @note ECC is enabled by default, may be temporary disabled by flash_ecc_disable()
 */
void flash_ecc_enable(void);


/**
 * @brief Configures scrambling of Flash Memory macro.
 *
 * The words order in sector and sectors in pages will be reordered.
 *
 * @param[in] seed constant value of 17 pseudo-random bytes
 * @note In TROPIC01 seed is given by PUF, therefore it is diversified for each chip instance.
 * @note Number of seed bytes depends on FSS_SECTOR_SCRAM and FSS_PAGE_SCRAM_* registers.
 */
void flash_init_scrambling(u8 *seed);

/**
 * @brief Reads single word from Flash Memory
 * @warning The function returns 0x0 as data if flash_read_data() fails. All code
 * paths that exist at this commit are verified to not be affected by this warning.
 * @param[in] address Byte address to read from, relative to start of Flash Subsystem address space
 * @returns Value read from Flash Memory
 */
u32 flash_read_word(u32 address) TS_CHECK_RETVAL;

/**
 * @brief Read multiple words from Flash Memory
 * @param[in] address Byte address to read from, relative to start of Flash Subsystem address space
 * @param[in] size Number of bytes to read, must be aligned to u32
 * @param[out] dest Destination where to put data.
 * @returns TS_TRUE if read performed ok
 */
ts_bool flash_read_data(u32 *dest, u32 address, size_t size) TS_CHECK_RETVAL;

/**
 * @brief Writes (Programs) single word of Flash Memory
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 */
void flash_write_word(u32 address, u32 data);

/**
 * @brief Writes (Programs) single word of Flash Memory and checks the word was written correctly.
 * @warning If `data` is 0x0 and flash_read_word() fails, the function returns TS_TRUE. All code
 * paths that exist at this commit are verified to not be affected by this warning.
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 * @returns TS_TRUE if data were programmed correctly, TS_FALSE otherwise
 */
ts_bool flash_write_word_verify(u32 address, u32 data) TS_CHECK_RETVAL;


/**
 * @brief Writes (Programs) single word of Flash Memory from ISR.
 * Special version for usage from ISR with limited functionality, does not have timeout.
 * Use with caution only when really necessary.
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 */
void flash_write_word_isr(u32 address, u32 data);

/**
 * @brief Check if flash has empty space then writes single word of Flash Memory and checks the word was written correctly.
 * @warning If `data` is 0x0 and flash_read_word() fails, the function returns TS_TRUE. All code
 * paths that exist at this commit are verified to not be affected by this warning.
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 * @returns TS_TRUE if data were programmed correctly, TS_FALSE otherwise
 */
ts_bool flash_safe_write_word(u32 address, u32 data) TS_CHECK_RETVAL;

/**
 * @brief Reads single sector of Flash Memory to RAM Buffer and copy to CPU Memory.
 * @param[out] dest Target memory where to store the read sector content
 * @param[in] address Address of an unencrypted sector to read, shall be start of sector.
 * @returns TS_FALSE if address is invalid, TS_TRUE otherwise
 */
ts_bool flash_read_sector(u32 *dest, u32 address) TS_CHECK_RETVAL;

/**
 * @brief Read single encrypted sector of Flash Memory to RAM Buffer, decrypt and copy to CPU Memory.
 *
 * @param[out] dest Target memory where to store the read data after decryption.
 * @param[in] dest_size Size of the target memory in bytes. The copy is bounded by this value,
 *                      see the warning below.
 * @param[in] address Address of an encrypted sector to read, shall be start of sector.
 *
 * @returns Number of bytes copied into @p dest, or 0 if nothing was copied - the sector is
 *          free, its decryption failed, or the arguments are out of bounds.
 *
 * @warning The copy is silently truncated when the sector holds more than @p dest_size bytes.
 *          The return value is the number of bytes copied, not the payload length stored in
 *          the sector, so a caller cannot distinguish a truncated read from a sector that
 *          happened to hold exactly @p dest_size bytes. Pass a buffer of at least
 *          ::FLASH_MAX_ENCRYPTED_PAYLOAD bytes to rule truncation out.
 */
size_t flash_read_sector_enc(u8 *dest, size_t dest_size, u32 address) TS_CHECK_RETVAL;

ts_bool flash_write_sector(u32 address, u32 *data) TS_CHECK_RETVAL;

/**
 * @brief Set NONCE used in next encrypted write.
 * This is useful i.e. before SPECT operation which write to Flash.
 * @param[in] nonce Current nonce to set.
 */
void flash_set_nonce(u8 nonce[FLASH_NONCE_SIZE]);


/**
 * @brief Write single encrypted sector of Flash Memory.
 * @param[in] address Address of an encrypted sector to read, shall be start of sector.
 * @param[in] data Plain text data to store encrypted.
 * @param[in] size Size of data.
 * @param[in] nonce Nonce for this sector.
 * @returns TS_TRUE if stored successfully or TS_FALSE if not.
 */
ts_bool flash_write_sector_enc(u32 address, void *data, size_t size, u8 nonce[FLASH_NONCE_SIZE]) TS_CHECK_RETVAL;

/**
 * @brief Read NVR sector to RAM buffer (FSS_RAM_BUF_BASE_ADDR)
 * @warning If the cases when this function can fail change, take it into account in
 * _flash_cfg_trim(), where the retval is ignored.
 *
 * @param address of NVR sector.
 */
ts_bool flash_read_nvr_to_buf(u32 address) TS_CHECK_RETVAL;

/**
 * @brief Read NVR sector and copy to destination
 *
 * @param address of NVR sector.
 */
ts_bool flash_read_nvr(u32 *dest, u32 address) TS_CHECK_RETVAL;

/**
 * @brief Verify whole sector is erased (512B)
 *
 * @param address Starting address of sector
 * @returns TS_TRUE if sector is erased
 */
ts_bool flash_verify_erased(u32 address) TS_CHECK_RETVAL;

/**
 * @brief Erase whole sector (512B)
 *
 * The entire memory consist of 1024 sectors each sector is 128 x 32 bit words .
 *
 * @param address Accepts any address, the address is internally padded to sector base address
 * @note In MPW1 it takes cca 3.3ms
 */
void flash_erase_sector(u32 address);

/**
 * @brief Erase whole block (4096B)
 *
 * The entire memory consist of 128 blocks each block is 1024 x 32 bit words .
 *
 * @param address Accepts any address, the address is internally padded to block base address
 */
void flash_erase_block(u32 address);

/**
 * @brief Erase entire FLASH memory.
 */
void flash_erase_chip(void);

/**
 * @brief Clears RAM buffer content.
 */
void flash_flush_rambuf(void);

/**
 * @brief Erase the driver sector cache in CPU memory.
 *
 * The cache holds the plaintext of the sector being read or written by
 * `flash_read_sector_enc()` / `flash_write_sector_enc()`, so it is erased at the end
 * of both. This entry point covers the case when neither reaches its end, i.e. when
 * an alarm is entered from inside the driver. It erases CPU memory only, the FSS RAM
 * buffer is cleared by `flash_flush_rambuf()`.
 */
void flash_clear_sector_cache(void);

// void flash_read_to_buffer(u32 address);
// void flash_write_from_buffer(u32 address, u32 mask_31_0, u32 mask_63_32, u32 mask_95_64, u32 mask_127_96);
// u32 flash_verif_sector_erase(u32 address);

#endif // ! FLASH_H
