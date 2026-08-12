/**
 * @file layout_flash.h
 * @author Tropic Square
 * @brief FLASH Layout defines for common bootloader and APP fields and HW related content.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef LAYOUT_FLASH_H
#define LAYOUT_FLASH_H

#define LAYOUT_FLASH_ADDR_R_CONFIG         (0x0000000)
#define LAYOUT_FLASH_SIZE_R_CONFIG           (0x00200)

/** @brief == 2*25kiB + 2*13kiB for firmwares */
#define LAYOUT_FLASH_ADDR_FW_STORAGE       (0x0044200)
#define LAYOUT_FLASH_SIZE_FW_STORAGE         (0x13000)

#define LAYOUT_FLASH_ADDR_R_CONFIG_SHADOW  (0x0057200)
#define LAYOUT_FLASH_SIZE_R_CONFIG_SHADOW      (0x200)

#define LAYOUT_FLASH_ADDR_BOOT_HOLD        (0x0057400)
#define LAYOUT_FLASH_SIZE_BOOT_HOLD            (0x200)

// spare space 0x57600 to 0x62FFF

#define LAYOUT_FLASH_ADDR_X509_CERT        (0x0063000)
#define LAYOUT_FLASH_SIZE_X509_CERT           (0x1000)

#define LAYOUT_FLASH_ADDR_AES_KEYS         (0x0064000)
#define LAYOUT_FLASH_SIZE_AES_KEYS           (0x04000)

#define LAYOUT_FLASH_ADDR_ECC_KEYS         (0x0068000)
#define LAYOUT_FLASH_SIZE_ECC_KEYS           (0x08000)

#define LAYOUT_FLASH_ADDR_MAC_AND_DESTROY  (0x0070000)
#define LAYOUT_FLASH_SIZE_MAC_AND_DESTROY    (0x10000)

#endif // ! LAYOUT_FLASH_H

