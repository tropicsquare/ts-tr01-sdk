/**
 * @file layout_otp.h
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 * @brief OTP Layout defines for common bootloader and APP fields.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef LAYOUT_OTP_H
#define LAYOUT_OTP_H

#define LAYOUT_OTP_KEY_SIZE                                    (32)

#define LAYOUT_OTP_ADDR_LIFE_CYCLE_A                      (0x00000)

#define LAYOUT_OTP_ADDR_VENDOR_KEY                        (0x00004)
#define LAYOUT_OTP_ADDR_VENDOR_SIGNATURE                  (0x00024)

#define LAYOUT_OTP_ADDR_X509_CERTIFICATE                  (0x00100)
#define LAYOUT_OTP_SIZE_X509_CERTIFICATE                     (3840)

#define LAYOUT_OTP_ADDR_I_CONFIG                          (0x01000)
#define LAYOUT_OTP_SIZE_I_CONFIG                            (4*512)

#define LAYOUT_OTP_ADDR_PUF_CHALLENGES                    (0x01800)
#define LAYOUT_OTP_SIZE_PUF_CHALLENGES                         (12)

#define LAYOUT_OTP_ADDR_TIMING                            (0x0180C)
#define LAYOUT_OTP_SIZE_TIMING                                 (16)

#define LAYOUT_OTP_ADDR_CHIP_ID                           (0x01900)
#define LAYOUT_OTP_SIZE_CHIP_ID                               (128)

#define LAYOUT_OTP_ADDR_ICONFIG_FAIL                      (0x01980)

// NOTE: at 0x01BF0 starts application space (LAYOUT_OTP_ADDR_S_H_PUB_STATE)

#define LAYOUT_OTP_ADDR_LIFE_CYCLE_B                      (0x01FFC) // end of OTP

#endif // ! LAYOUT_OTP_H

