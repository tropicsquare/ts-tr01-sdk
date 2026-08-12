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
#define FLASH_MAX_ENCRYPTED_SIZE            476 // Encrypted chunk size in sector
#define FLASH_NONCE_SIZE                    16
#define FLASH_ATAG_SIZE                     16
///@}

/**
 * @brief Initialize Flash Subsystem.
 *
 * Performs:
 *  - Enables clock for Flash Subsystem
 *  - Configures Flash macro timing
 *  - Reads out and applies Flash macro trim
 *
 * @note: You need to call also flash_init_scrambling() during initialization phase
 */
void flash_init(void);

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
 * @param[in] address Byte address to read from, relative to start of Flash Subsystem address space
 * @returns Value read from Flash Memory
 */
u32 flash_read_word(u32 address);

/**
 * @brief Read multiple words from Flash Memory
 * @param[in] address Byte address to read from, relative to start of Flash Subsystem address space
 * @param[in] size Number of bytes to read, must be aligned to u32
 * @param[out] dest Destination where to put data.
 * @returns TS_TRUE if read performed ok
 */
ts_bool flash_read_data(u32 *dest, u32 address, size_t size);

/**
 * @brief Writes (Programs) single word of Flash Memory
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 */
void flash_write_word(u32 address, u32 data);

/**
 * @brief Writes (Programs) single word of Flash Memory and checks the word was written correctly.
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 * @returns TS_TRUE if data were programmed correctly, TS_FALSE otherwise
 */
ts_bool flash_write_word_verify(u32 address, u32 data);

/**
 * @brief Check if flash has empty space then writes single word of Flash Memory and checks the word was written correctly.
 * @param[in] address Byte address to write
 * @param[in] data Data to write(program)
 * @returns TS_TRUE if data were programmed correctly, TS_FALSE otherwise
 */
ts_bool flash_safe_write_word(u32 address, u32 data);

/**
 * @brief Reads single sector of Flash Memory to RAM Buffer and copy to CPU Memory.
 * @param[out] dest Target memory where to store the read sector content
 * @param[in] address Address of an unencrypted sector to read, shall be start of sector.
 * @returns TS_FALSE if address is invalid, TS_TRUE otherwise
 */
ts_bool flash_read_sector(u32 *dest, u32 address);

/**
 * @brief Read single encrypted sector of Flash Memory to RAM Buffer, decrypt and copy to CPU Memory.
 * @param[out] dest Target memory where to store the read data after decryption.
 * @param[in] address Address of an encrypted sector to read, shall be start of sector.
 * @returns Number of bytes read from the sector.
 */
size_t flash_read_sector_enc(u8 *dest, u32 address);

ts_bool flash_write_sector(u32 address, u32 *data);

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
ts_bool flash_write_sector_enc(u32 address, void *data, size_t size, u8 nonce[FLASH_NONCE_SIZE]);

/**
 * @brief Read NVR sector to RAM buffer (FSS_RAM_BUF_BASE_ADDR)
 *
 * @param address of NVR sector.
 */
ts_bool flash_read_nvr_to_buf(u32 address);

/**
 * @brief Read NVR sector and copy to destination
 *
 * @param address of NVR sector.
 */
ts_bool flash_read_nvr(u32 *dest, u32 address);

/**
 * @brief Read data from RAM buffer.
 *
 * @param[in] offset Address in buffer (0..511)
 * @param[in] size Number of bytes to copy
 * @param[out] dest Destination where to copy data.
 */
ts_bool flash_read_buf(u8 *dest, u32 offset, size_t size);

/**
 * @brief Verify whole sector is erased (512B)
 *
 * @param address Starting address of sector
 * @returns TS_TRUE if sector is erased
 */
ts_bool flash_verify_erased(u32 address);

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

// void flash_read_to_buffer(u32 address);
// void flash_write_from_buffer(u32 address, u32 mask_31_0, u32 mask_63_32, u32 mask_95_64, u32 mask_127_96);
// u32 flash_verif_sector_erase(u32 address);

#endif // ! FLASH_H
