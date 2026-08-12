/**
 * @file ptrng.c
 * @brief PTRNG driver source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 *
 * @author: Tropic Square
 */

#include "common.h"
#include "ptrng.h"
#include "timer.h"

#include "tassic_defs.h"
#include "io_ops.h"
#include "soc_ctrl.h"

#include "ptrng_regs.h"
#include "sec_cntr_regs.h"

#include <string.h>

#include "log.h"
LOG_DEF("TRNG");

#define _PTRNG_ERR_STATUS  1

#define PTRNG_ENABLE                                      1
#define PTRNG_DISABLE                                     0

#define PTRNG_KNOWN_ANSWER                                0x838D2578

#define PTRNG_STARTUP_OK                                  0
#define PTRNG_STARTUP_TOT_FAIL                            1
#define PTRNG_STARTUP_AP_FAIL                             2
#define PTRNG_STARTUP_MU_FAIL                             4
#define PTRNG_STARTUP_RC_FAIL                             8
#define PTRNG_STARTUP_TESTS_NOT_FINISHED                  16

#define PTRNG_CONFIG_T2D_VER_V0                           0x0          //Value: 0x0          version 0 - T2D
#define PTRNG_TEST_DNS_CONFIG_RC_ENA_DIS                  0x0          //Value: 0x0          Disable
#define PTRNG_TEST_DNS_CONFIG_AP_ENA_DIS                  0x0          //Value: 0x0          Disable
#define PTRNG_CONFIG_DFT_SRC_SEL_REG                      0x1          //Value: 0x10000000   the 16 bit test register (PTRNG_DFT_DATA) is selected for T2D converter
#define PTRNG_TEST_DNS_CONFIG_RC_ENA_ENA                  0x1          //Value: 0x1000       Enable
#define PTRNG_TEST_DNS_CONFIG_AP_ENA_ENA                  0x1          //Value: 0x1000       Enable
#define PTRNG_TEST_ARB_CONFIG_MU_WIN_2048                 0x1          //Value: 0x4000       N=2048 crossing events
#define PTRNG_TEST_ARB_CONFIG_MU_ENA_DIS                  0x0          //Value: 0x0          Disable
#define PTRNG_TEST_DNS_CONFIG_AP_SIZE_1BIT                0x0          //Value: 0x0          1 bit
#define PTRNG_TEST_DNS_CONFIG_AP_SIZE_2BITS               0x1          //Value: 0x4000       2 bits
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_1024                 0x4          //Value: 0x20000      N=1024

#define PTRNG_TEST_ARB_CONFIG_TOT_ENA_DIS                 0x0          //Value: 0x0          Disable
#define PTRNG_TEST_ARB_CONFIG_TOT_ENA_ENA                 0x1          //Value: 0x1          Enable

#define PTRNG_TEST_DNS_CONFIG_AP_SOURCE_RRN               0x0          //Value: 0x0          RRN bits
#define PTRNG_TEST_DNS_CONFIG_AP_SOURCE_ARB               0x1          //Value: 0x2000       Arbiter_out

#define PTRNG_TEST_DNS_CONFIG_AP_WIN_64                   0x0          //Value: 0x0          N=64
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_128                  0x1          //Value: 0x8000       N=128
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_256                  0x2          //Value: 0x10000      N=256
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_512                  0x3          //Value: 0x18000      N=512
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_1024                 0x4          //Value: 0x20000      N=1024
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_2048                 0x5          //Value: 0x28000      N=2048
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_4096                 0x6          //Value: 0x30000      N=4096
#define PTRNG_TEST_DNS_CONFIG_AP_WIN_8192                 0x7          //Value: 0x38000      N=8192

#define PTRNG_DFT_PP_DFT_PP_DIS                           0x0          //Value: 0x0          No action
#define PTRNG_DFT_PP_DFT_PP_ENA                           0x1          //Value: 0x1          Sets the value

#define PTRNG_CONFIG_T2D_VER_V0                           0x0          //Value: 0x0          version 0 - T2D
#define PTRNG_CONFIG_T2D_VER_V1                           0x1          //Value: 0x2          version 1 - EM
#define PTRNG_CONFIG_T2D_VER_V2                           0x2          //Value: 0x4          version 2 - G
#define PTRNG_CONFIG_T2D_VER_V3                           0x3          //Value: 0x6          version 3 - UD

#define PTRNG_CONFIG_T2D_EDGE_RISE                        0x0          //Value: 0x0          rising: 0-to-1 edges
#define PTRNG_CONFIG_T2D_EDGE_FALL                        0x1          //Value: 0x8          falling: 1-to-0 edges

#define PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_DIS                0x0          //Value: 0x0          No counter clear
#define PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_ENA                0x1          //Value: 0x10         Enable the clear

#define PTRNG_CONFIG_T2D_CNT                              0x3

#define PTRNG_CONFIG_PP_N_PARAM                           0x36

#define PTRNG_CONFIG_T2D_DATA_ENA_DIS                     0x0          //Value: 0x0          Disable
#define PTRNG_CONFIG_T2D_DATA_ENA_ENA                     0x1          //Value: 0x20         Enable

#define PTRNG_CONFIG_T2D_ENA_DIS                          0x0          //Value: 0x0          Disable
#define PTRNG_CONFIG_T2D_ENA_ENA                          0x1          //Value: 0x1          Enable

#define PTRNG_TEST_ARB_CONFIG_MU_WIN_1024                 0x0          //Value: 0x0          N=1024 crossing events
#define PTRNG_TEST_ARB_CONFIG_MU_WIN_2048                 0x1          //Value: 0x4000       N=2048 crossing events
#define PTRNG_TEST_ARB_CONFIG_MU_WIN_4096                 0x2          //Value: 0x8000       N=4096 crossing events
#define PTRNG_TEST_ARB_CONFIG_MU_WIN_8192                 0x3          //Value: 0xc000       N=8192 crossing events

#define PTRNG_TEST_ARB_CONFIG_MU_ONCE_CONTINUE            0x0          //Value: 0x0          the test runs continuously
#define PTRNG_TEST_ARB_CONFIG_MU_ONCE_ONCE                0x1          //Value: 0x2000       the test runs once

#define PTRNG_TEST_ARB_CONFIG_MU_ENA_DIS                  0x0          //Value: 0x0          Disable
#define PTRNG_TEST_ARB_CONFIG_MU_ENA_ENA                  0x1          //Value: 0x1000       Enable

#define PTRNG_TOTAL_FAILURE_CUTOFF                        126

#define PTRNG_RC_CUTOFF                                   0x35
#define PTRNG_AP_STARTUP_CUTOFF                           63
#define PTRNG_AP_MONOBIT_CUTOFF                           597
#define PTRNG_MU_MIN1_CUTOFF                              0x1E    // startup test
#define PTRNG_MU_MAX1_CUTOFF                              0x53
#define PTRNG_MU_MIN2_CUTOFF                              0x1E    // online test
#define PTRNG_MU_MAX2_CUTOFF                              0x53

#define _TRNG_REG_PTR(ptrng,offset) PTR32_T((ptrng) + (offset))
#define _TRNG_REG_MODIFY(ptrng,offset,mask,value) \
    _TRNG_REG_PTR(ptrng,offset) = ((_TRNG_REG_PTR(ptrng,offset) & ~(mask)) | (value))

#define _get_ptrng_config(ptrng) _TRNG_REG_PTR(ptrng, PTRNG_CONFIG_ADDR)
#define _get_ptrng_status(ptrng) _TRNG_REG_PTR(ptrng, PTRNG_STATUS_ADDR)

#define _PTRNG_DEFAULT_TIMEOUT (1000) // [us]
#define _PTRNG_HEALT_TEST_TIMEOUT (2000) // [us] tests finish at about 1100
#define _PTRNG_ONLINE_TEST_TIMEOUT (10000) // [us] tests finish at about 3100

#define _PTRNG_SANITY_ADDR(addr) OS_SANITY((addr == PTRNG1) || (addr == PTRNG0))

// ----------------------------------------------------------------------------------------
// ------------------------PTRNG-REGISTER-MANIPULATION-FUNCTIONS---------------------------
// ----------------------------------------------------------------------------------------

static inline void _set_ptrng_config(ptrng_address_e ptrng, u32 data) {
    _TRNG_REG_PTR(ptrng, PTRNG_CONFIG_ADDR) = data;
}

static inline void _set_ptrng_status(ptrng_address_e ptrng, u32 data) {
    _TRNG_REG_PTR(ptrng, PTRNG_STATUS_ADDR) = data;
}

static inline void _set_ptrng_pi(ptrng_address_e ptrng, u32 data) {
    _TRNG_REG_PTR(ptrng, PTRNG_PP_PI_ADDR) = data;
}

static inline void _set_ptrng_dft_data(ptrng_address_e ptrng, u32 data) {
    _TRNG_REG_PTR(ptrng, PTRNG_DFT_DATA_ADDR) = data;
}

static inline void _set_ptrng_test_arb_config(ptrng_address_e ptrng, u32 data) {
    _TRNG_REG_PTR(ptrng, PTRNG_TEST_ARB_CONFIG_ADDR) = data;
}

static inline void _set_ptrng_test_dns_config(ptrng_address_e ptrng, u32 data) {
   _TRNG_REG_PTR(ptrng, PTRNG_TEST_DNS_CONFIG_ADDR) = data;
}

static inline void _set_ptrng_dft_pp(ptrng_address_e ptrng, u32 data) {
   _TRNG_REG_PTR(ptrng, PTRNG_DFT_PP_ADDR) = data;
}

static inline void _set_ptrng_config_t2d_cnt(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_config(ptrng, (_get_ptrng_config(ptrng) & ~PTRNG_CONFIG_T2D_CNT_MASK) | ((data << PTRNG_CONFIG_T2D_CNT_POS) & PTRNG_CONFIG_T2D_CNT_MASK));
}

static inline void _set_ptrng_config_t2d_cnt_clear_ena(ptrng_address_e ptrng, u32 data){
    _set_ptrng_config(ptrng, (_get_ptrng_config(ptrng) & ~PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_MASK)
            | ((data << PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_POS) & PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_MASK));
}

static inline void _code_ptrng_config_t2d_cnt_clear_ena(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_MASK)
            | ((field_data << PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_POS) & PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_MASK));
}

static inline u32 _get_ptrng_test_arb_config_tot_ena(ptrng_address_e ptrng) {
    return ((_TRNG_REG_PTR(ptrng, PTRNG_TEST_ARB_CONFIG_ADDR) & PTRNG_TEST_ARB_CONFIG_TOT_ENA_MASK) >> PTRNG_TEST_ARB_CONFIG_TOT_ENA_POS);
}

static inline u32 _get_ptrng_test_arb_config(ptrng_address_e ptrng) {
    return(_TRNG_REG_PTR(ptrng, PTRNG_TEST_ARB_CONFIG_ADDR));
}

static inline u32 _get_ptrng_test_dns_config(ptrng_address_e ptrng) {
  return (_TRNG_REG_PTR(ptrng, PTRNG_TEST_DNS_CONFIG_ADDR));
}

static inline void _set_ptrng_config_dft_src_sel(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_config(ptrng, (_get_ptrng_config(ptrng) & ~PTRNG_CONFIG_DFT_SRC_SEL_MASK)
            | ((data << PTRNG_CONFIG_DFT_SRC_SEL_POS) & PTRNG_CONFIG_DFT_SRC_SEL_MASK));
}

static inline void _set_ptrng_config_t2d_ver(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_config(ptrng, (_get_ptrng_config(ptrng) & ~PTRNG_CONFIG_T2D_VER_MASK) | ((data << PTRNG_CONFIG_T2D_VER_POS) & PTRNG_CONFIG_T2D_VER_MASK));
}

static inline void _code_ptrng_config_t2d_ver(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_T2D_VER_MASK)
          | ((field_data << PTRNG_CONFIG_T2D_VER_POS) & PTRNG_CONFIG_T2D_VER_MASK));
}

static inline void _code_ptrng_config_t2d_ena(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_T2D_ENA_MASK)
            | ((field_data << PTRNG_CONFIG_T2D_ENA_POS) & PTRNG_CONFIG_T2D_ENA_MASK));
}

static inline u32 _get_ptrng_config_t2d_ver(ptrng_address_e ptrng){
  return ((_get_ptrng_config(ptrng) & PTRNG_CONFIG_T2D_VER_MASK) >> PTRNG_CONFIG_T2D_VER_POS);
}

static inline u32 _decode_ptrng_config_t2d_ver(u32 register_data){
  return ((register_data & PTRNG_CONFIG_T2D_VER_MASK) >> PTRNG_CONFIG_T2D_VER_POS);
}

static inline u32 _get_ptrng_status_tot_fail(ptrng_address_e ptrng) {
    return ((_get_ptrng_status(ptrng) & PTRNG_STATUS_TOT_FAIL_MASK) >> PTRNG_STATUS_TOT_FAIL_POS);
}

static inline u32 _get_ptrng_status_rc_fail(ptrng_address_e ptrng) {
    return ((_get_ptrng_status(ptrng) & PTRNG_STATUS_RC_FAIL_MASK) >> PTRNG_STATUS_RC_FAIL_POS);
}

static inline u32 _get_ptrng_status_ap_fail(ptrng_address_e ptrng) {
    return ((_get_ptrng_status(ptrng) & PTRNG_STATUS_AP_FAIL_MASK) >> PTRNG_STATUS_AP_FAIL_POS);
}

static inline u32 _get_ptrng_status_arb_ready(ptrng_address_e ptrng) {
    return ((_get_ptrng_status(ptrng) & PTRNG_STATUS_ARB_READY_MASK) >> PTRNG_STATUS_ARB_READY_POS);
}

static inline u32 _get_ptrng_rn_data(ptrng_address_e ptrng) {
    return (_TRNG_REG_PTR(ptrng, PTRNG_RN_DATA_ADDR));
}

static inline u32 _get_ptrng_arb_data(ptrng_address_e ptrng) {
    return (_TRNG_REG_PTR(ptrng, PTRNG_ARB_DATA_ADDR));
}

static inline void _set_ptrng_test_arb_config_tot_cutoff(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_arb_config(ptrng, (_get_ptrng_test_arb_config(ptrng) & ~PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_MASK) | ((data << PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_POS) & PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_MASK));
}

static inline void _code_ptrng_test_arb_config_tot_cutoff(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_POS) & PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_MASK));
}

static inline u32 _get_ptrng_test_arb_config_tot_cutoff(ptrng_address_e ptrng) {
    return ((_get_ptrng_test_arb_config(ptrng) & PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_MASK) >> PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_POS);
}

static inline u32 _decode_ptrng_test_arb_config_tot_cutoff(u32 register_data) {
    return ((register_data & PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_MASK) >> PTRNG_TEST_ARB_CONFIG_TOT_CUTOFF_POS);
}

static inline void _set_ptrng_test_arb_config_mu_cutoff_min(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_arb_config(ptrng, (_get_ptrng_test_arb_config(ptrng) & ~PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_MASK) | ((data << PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_POS) & PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_MASK));
}

static inline void _code_ptrng_test_arb_config_mu_cutoff_min(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_POS) & PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_MASK));
}

static inline u32 _get_ptrng_test_arb_config_mu_cutoff_min(ptrng_address_e ptrng) {
    return ((_get_ptrng_test_arb_config(ptrng) & PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_MASK) >> PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_POS);
}

static inline u32 _decode_ptrng_test_arb_config_mu_cutoff_min(u32 register_data) {
    return ((register_data & PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_MASK) >> PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MIN_POS);
}

static inline void _set_ptrng_test_dns_config_rc_ena(ptrng_address_e ptrng, u32 data){
    _set_ptrng_test_dns_config(ptrng, (_get_ptrng_test_dns_config(ptrng) & ~PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK) | ((data << PTRNG_TEST_DNS_CONFIG_RC_ENA_POS) & PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK));
}

static inline void _code_ptrng_test_dns_config_rc_ena(u32 *p_register_data, u32 field_data){
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_RC_ENA_POS) & PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK));
}

static inline u32 _get_ptrng_test_dns_config_rc_ena(ptrng_address_e ptrng){
    return ((_get_ptrng_test_dns_config(ptrng) & PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK) >> PTRNG_TEST_DNS_CONFIG_RC_ENA_POS);
}

static inline u32 _decode_ptrng_test_dns_config_rc_ena(u32 register_data){
    return ((register_data & PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK) >> PTRNG_TEST_DNS_CONFIG_RC_ENA_POS);
}

static inline void _set_ptrng_test_dns_config_rc_cutoff(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_dns_config(ptrng, (_get_ptrng_test_dns_config(ptrng) & ~PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_MASK) | ((data << PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_POS) & PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_MASK));
}

static inline void _code_ptrng_test_dns_config_rc_cutoff(u32 *p_register_data, u32 field_data){
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_POS) & PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_MASK));
}

static inline u32 _get_ptrng_test_dns_config_rc_cutoff(ptrng_address_e ptrng){
    return ((_get_ptrng_test_dns_config(ptrng) & PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_MASK) >> PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_POS);
}

static inline u32 _decode_ptrng_test_dns_config_rc_cutoff(u32 register_data){
    return ((register_data & PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_MASK) >> PTRNG_TEST_DNS_CONFIG_RC_CUTOFF_POS);
}

static inline void _set_ptrng_test_dns_config_ap_win(ptrng_address_e ptrng, u32 data){
    _set_ptrng_test_dns_config(ptrng, (_get_ptrng_test_dns_config(ptrng) & ~PTRNG_TEST_DNS_CONFIG_AP_WIN_MASK) | ((data << PTRNG_TEST_DNS_CONFIG_AP_WIN_POS) & PTRNG_TEST_DNS_CONFIG_AP_WIN_MASK));
}

static inline void _code_ptrng_test_dns_config_ap_win(u32 *p_register_data, u32 field_data){
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_AP_WIN_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_AP_WIN_POS) & PTRNG_TEST_DNS_CONFIG_AP_WIN_MASK));
}

static inline u32 _get_ptrng_test_dns_config_ap_win(ptrng_address_e ptrng){
    return ((_get_ptrng_test_dns_config(ptrng) & PTRNG_TEST_DNS_CONFIG_AP_WIN_MASK) >> PTRNG_TEST_DNS_CONFIG_AP_WIN_POS);
}

static inline u32 _decode_ptrng_test_dns_config_ap_win(u32 register_data){
    return ((register_data & PTRNG_TEST_DNS_CONFIG_AP_WIN_MASK) >> PTRNG_TEST_DNS_CONFIG_AP_WIN_POS);
}

static inline void _set_ptrng_test_dns_config_ap_ena(ptrng_address_e ptrng, u32 data){
    _set_ptrng_test_dns_config(ptrng, (_get_ptrng_test_dns_config(ptrng) & ~PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK) | ((data << PTRNG_TEST_DNS_CONFIG_AP_ENA_POS) & PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK));
}

static inline void _code_ptrng_test_dns_config_ap_ena(u32 *p_register_data, u32 field_data){
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_AP_ENA_POS) & PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK));
}

static inline u32 _get_ptrng_test_dns_config_ap_ena(ptrng_address_e ptrng){
    return ((_get_ptrng_test_dns_config(ptrng) & PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK) >> PTRNG_TEST_DNS_CONFIG_AP_ENA_POS);
}

static inline u32 _decode_ptrng_test_dns_config_ap_ena(u32 register_data){
    return ((register_data & PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK) >> PTRNG_TEST_DNS_CONFIG_AP_ENA_POS);
}

static inline void _set_ptrng_status_mu_fail(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_status(ptrng, (_get_ptrng_status(ptrng) & ~PTRNG_STATUS_MU_FAIL_MASK) | ((data << PTRNG_STATUS_MU_FAIL_POS) & PTRNG_STATUS_MU_FAIL_MASK));
}

static inline void _code_ptrng_status_mu_fail(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_STATUS_MU_FAIL_MASK) | ((field_data << PTRNG_STATUS_MU_FAIL_POS) & PTRNG_STATUS_MU_FAIL_MASK));
}

static inline u32 _get_ptrng_status_mu_fail(ptrng_address_e ptrng) {
    return ((_get_ptrng_status(ptrng) & PTRNG_STATUS_MU_FAIL_MASK) >> PTRNG_STATUS_MU_FAIL_POS);
}

static inline u32 _decode_ptrng_status_mu_fail(u32 register_data) {
    return ((register_data & PTRNG_STATUS_MU_FAIL_MASK) >> PTRNG_STATUS_MU_FAIL_POS);
}

static inline void _set_ptrng_test_arb_config_mu_ena(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_arb_config(ptrng, (_get_ptrng_test_arb_config(ptrng) & ~PTRNG_TEST_ARB_CONFIG_MU_ENA_MASK) | ((data << PTRNG_TEST_ARB_CONFIG_MU_ENA_POS) & PTRNG_TEST_ARB_CONFIG_MU_ENA_MASK));
}

static inline void _code_ptrng_test_arb_config_mu_ena(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_MU_ENA_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_MU_ENA_POS) & PTRNG_TEST_ARB_CONFIG_MU_ENA_MASK));
}

static inline u32 _get_ptrng_test_arb_config_mu_ena(ptrng_address_e ptrng) {
    return ((_get_ptrng_test_arb_config(ptrng) & PTRNG_TEST_ARB_CONFIG_MU_ENA_MASK) >> PTRNG_TEST_ARB_CONFIG_MU_ENA_POS);
}

static inline u32 _decode_ptrng_test_arb_config_mu_ena(u32 register_data) {
    return ((register_data & PTRNG_TEST_ARB_CONFIG_MU_ENA_MASK) >> PTRNG_TEST_ARB_CONFIG_MU_ENA_POS);
}

static inline void _set_ptrng_test_dns_config_ap_source(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_dns_config(ptrng, (_get_ptrng_test_dns_config(ptrng) & ~PTRNG_TEST_DNS_CONFIG_AP_SOURCE_MASK) | ((data << PTRNG_TEST_DNS_CONFIG_AP_SOURCE_POS) & PTRNG_TEST_DNS_CONFIG_AP_SOURCE_MASK));
}

static inline void _code_ptrng_test_dns_config_ap_source(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_AP_SOURCE_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_AP_SOURCE_POS) & PTRNG_TEST_DNS_CONFIG_AP_SOURCE_MASK));
}

static inline u32 _get_ptrng_test_dns_config_ap_source(ptrng_address_e ptrng) {
    return ((_get_ptrng_test_dns_config(ptrng) & PTRNG_TEST_DNS_CONFIG_AP_SOURCE_MASK) >> PTRNG_TEST_DNS_CONFIG_AP_SOURCE_POS);
}

static inline u32 _decode_ptrng_test_dns_config_ap_source(u32 register_data) {
    return ((register_data & PTRNG_TEST_DNS_CONFIG_AP_SOURCE_MASK) >> PTRNG_TEST_DNS_CONFIG_AP_SOURCE_POS);
}

static inline void _set_ptrng_test_arb_config_mu_once(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_arb_config(ptrng, (_get_ptrng_test_arb_config(ptrng) & ~PTRNG_TEST_ARB_CONFIG_MU_ONCE_MASK) | ((data << PTRNG_TEST_ARB_CONFIG_MU_ONCE_POS) & PTRNG_TEST_ARB_CONFIG_MU_ONCE_MASK));
}

static inline void _code_ptrng_test_arb_config_mu_once(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_MU_ONCE_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_MU_ONCE_POS) & PTRNG_TEST_ARB_CONFIG_MU_ONCE_MASK));
}

static inline u32 _get_ptrng_test_arb_config_mu_once(ptrng_address_e ptrng) {
    return ((_get_ptrng_test_arb_config(ptrng) & PTRNG_TEST_ARB_CONFIG_MU_ONCE_MASK) >> PTRNG_TEST_ARB_CONFIG_MU_ONCE_POS);
}

static inline u32 _decode_ptrng_test_arb_config_mu_once(u32 register_data) {
    return ((register_data & PTRNG_TEST_ARB_CONFIG_MU_ONCE_MASK) >> PTRNG_TEST_ARB_CONFIG_MU_ONCE_POS);
}

static inline void _set_ptrng_config_pp_n_param(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_config(ptrng, (_get_ptrng_config(ptrng) & ~PTRNG_CONFIG_PP_N_PARAM_MASK) | ((data << PTRNG_CONFIG_PP_N_PARAM_POS) & PTRNG_CONFIG_PP_N_PARAM_MASK));
}

static inline void _set_ptrng_test_arb_config_tot_ena(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_arb_config(ptrng, (_get_ptrng_test_arb_config(ptrng) & ~PTRNG_TEST_ARB_CONFIG_TOT_ENA_MASK) | ((data << PTRNG_TEST_ARB_CONFIG_TOT_ENA_POS) & PTRNG_TEST_ARB_CONFIG_TOT_ENA_MASK));
}

static inline void _code_ptrng_test_dns_config_ap_cutoff(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_AP_CUTOFF_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_AP_CUTOFF_POS) & PTRNG_TEST_DNS_CONFIG_AP_CUTOFF_MASK));
}

static inline void _code_ptrng_test_dns_config_ap_size(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_DNS_CONFIG_AP_SIZE_MASK) | ((field_data << PTRNG_TEST_DNS_CONFIG_AP_SIZE_POS) & PTRNG_TEST_DNS_CONFIG_AP_SIZE_MASK));
}

static inline void _set_ptrng_test_dns_config_ap_cutoff(ptrng_address_e ptrng, u32 data) {
    _set_ptrng_test_dns_config(ptrng, (_get_ptrng_test_dns_config(ptrng) & ~PTRNG_TEST_DNS_CONFIG_AP_CUTOFF_MASK) | ((data << PTRNG_TEST_DNS_CONFIG_AP_CUTOFF_POS) & PTRNG_TEST_DNS_CONFIG_AP_CUTOFF_MASK));
}

static inline void _code_ptrng_config_t2d_edge(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_T2D_EDGE_MASK) | ((field_data << PTRNG_CONFIG_T2D_EDGE_POS) & PTRNG_CONFIG_T2D_EDGE_MASK));
}

static inline void _code_ptrng_test_arb_config_tot_ena(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_TOT_ENA_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_TOT_ENA_POS) & PTRNG_TEST_ARB_CONFIG_TOT_ENA_MASK));
}

static inline void _code_ptrng_test_arb_config_mu_cutoff_max(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MAX_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MAX_POS) & PTRNG_TEST_ARB_CONFIG_MU_CUTOFF_MAX_MASK));
}

static inline void _code_ptrng_test_arb_config_mu_win(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_TEST_ARB_CONFIG_MU_WIN_MASK) | ((field_data << PTRNG_TEST_ARB_CONFIG_MU_WIN_POS) & PTRNG_TEST_ARB_CONFIG_MU_WIN_MASK));
}

static inline void _code_ptrng_config_t2d_cnt(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_T2D_CNT_MASK) | ((field_data << PTRNG_CONFIG_T2D_CNT_POS) & PTRNG_CONFIG_T2D_CNT_MASK));
}

static inline void _code_ptrng_config_t2d_data_ena(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_T2D_DATA_ENA_MASK) | ((field_data << PTRNG_CONFIG_T2D_DATA_ENA_POS) & PTRNG_CONFIG_T2D_DATA_ENA_MASK));
}

static inline void _code_ptrng_config_pp_n_param(u32 *p_register_data, u32 field_data) {
    *p_register_data = ((*p_register_data & ~PTRNG_CONFIG_PP_N_PARAM_MASK) | ((field_data << PTRNG_CONFIG_PP_N_PARAM_POS) & PTRNG_CONFIG_PP_N_PARAM_MASK));
}

// ----------------------------------------------------------------------------------------
// --------------------END-PTRNG-REGISTER-MANIPULATION-FUNCTIONS---------------------------
// ----------------------------------------------------------------------------------------

/**
 * Enables Online Test Alarm
 *
 * @param[in] alEna Enable value for Online Failure Test Alarm. Shall
 * be chosen among PTRNG_ENABLE and PTRNG_DISABLE to respectively
 * enable or disable the Total Failure Alarm.
 */
static inline void _ptrng_wakeupline_alarm_set (ptrng_address_e ptrng, uint32_t alarm_enable)
{
    // set_ptrng_al_ena_online_al_ena(alEna);
    _TRNG_REG_MODIFY(ptrng, PTRNG_AL_ENA_ADDR, PTRNG_AL_ENA_ONLINE_AL_ENA_MASK,(alarm_enable << PTRNG_AL_ENA_ONLINE_AL_ENA_POS));
}

/**
 * Enables Total Failure Test Alarm
 *
 * @param[in] alEna Enable value for Total Failure Test Alarm. Shall
 * be chosen among PTRNG_ENABLE and PTRNG_DISABLE.
 */
static inline void _ptrng_total_alarm_set (ptrng_address_e ptrng, uint32_t alarm_enable)
{
    // set_ptrng_al_ena_tot_al_ena(alEna);
    _TRNG_REG_MODIFY(ptrng, PTRNG_AL_ENA_ADDR, PTRNG_AL_ENA_TOT_AL_ENA_MASK,(alarm_enable << PTRNG_AL_ENA_TOT_AL_ENA_POS));
}

/**
 * Gate RNG Clock before any PTRNG config update
 * A countdown is added to be RNG clock is stopped before
 * any configuration update.
 */
static inline void _ptrng_clk_gate(ptrng_address_e ptrng) {
    _TRNG_REG_PTR(ptrng, PTRNG_ANALOG_CONFIG_ADDR) &= ~(PTRNG_ANALOG_CONFIG_RNG_CLK_ENA_MASK);
    os_delay_us(15);
}

/**
 * Ungate RNG Clock after PTRNG config update
 * A call to _ptrng_wait_conf_ready might be required after
 * clock enabled to wait for config to be ready.
 */
static inline void _ptrng_clk_ungate(ptrng_address_e ptrng) {
    _TRNG_REG_PTR(ptrng, PTRNG_ANALOG_CONFIG_ADDR) |= (1 << PTRNG_ANALOG_CONFIG_RNG_CLK_ENA_POS);
}

/**
 * Check if an alarm is raised of the PTRNG peripheral.
 * @return 0 if PTRNG is not under Alarm, a non-nul value otherwise.
 */
static inline uint32_t _ptrng_is_under_alarm (ptrng_address_e trng)
{
    return (_TRNG_REG_PTR(trng, PTRNG_STATUS_ADDR) & (PTRNG_STATUS_ONLINE_FAIL_MASK | PTRNG_STATUS_TOT_FAIL_MASK));
}

/**
 * Disables the Test mode for Arbiter out.
 */
static inline void _ptrng_disable_arb_out_test (ptrng_address_e trng)
{
  _TRNG_REG_PTR(trng, PTRNG_CONFIG_ADDR) &= ~PTRNG_CONFIG_DFT_SRC_SEL_MASK;
}

// structure to support os_wait_for() functionality
typedef struct _condition {
    ptrng_address_e ptrng;
    u32 status_value;
    u32 status_bits;
} condition_t;

static condition_t _condition;

static ts_bool _condition_status(void)
{
    if ((_TRNG_REG_PTR(_condition.ptrng, PTRNG_STATUS_ADDR) & _condition.status_bits) != _condition.status_value)
        return TS_FALSE;

    return TS_TRUE;
}

/**
 * Busy waits on the PTRNG_STATUS register's bits until they are set.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant
 * @param bits mask of bits to wait on
 * @param timeout_us max amount of time to wait in us
 */
static ts_bool _ptrng_status_wait(ptrng_address_e ptrng, u32 bits, u32 timeout_us)
{
    _condition.ptrng = ptrng;
    _condition.status_value = bits;
    _condition.status_bits = bits;

    return os_wait_for(_condition_status, timeout_us);
}

/**
 * Busy waits on the PTRNG_STATUS register's bits until they are unset.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant
 * @param bits mask of bits to wait on
 * @param timeout_us max amount of time to wait in us
 */
static ts_bool _ptrng_status_wait_unset(ptrng_address_e ptrng, u32 bits, u32 timeout_us)
{
    _condition.ptrng = ptrng;
    _condition.status_value = 0;
    _condition.status_bits = bits;

    return os_wait_for(_condition_status, timeout_us);
}

/**
 * Waits for the PTRNG BUS ACCESS to be ready (after a flush)
 */
static inline void _ptrng_wait_conf_ready(ptrng_address_e ptrng)
{
    _ptrng_status_wait_unset(ptrng, PTRNG_STATUS_BUSY_MASK, _PTRNG_DEFAULT_TIMEOUT);
}

/**
 * Waits for the PTRNG BUS ACCESS to be ready (after a flush), ungated version
 */
static void _ptrng_wait_conf_ready_ungated(ptrng_address_e ptrng)
{
    _ptrng_clk_ungate(ptrng);
    _ptrng_wait_conf_ready(ptrng);
    _ptrng_clk_gate(ptrng);
}


/**
 * Perform an IP flush and wait for ready status.
*/
static inline void _ptrng_ip_flush (ptrng_address_e ptrng)
{
    _set_ptrng_status (ptrng, 0x0);
    _ptrng_wait_conf_ready(ptrng);
}

static u32 _ptrng_is_tot_finished(ptrng_address_e ptrng)
{
    u32 status = _get_ptrng_status(ptrng);
    return ((status & PTRNG_STATUS_TOT_END_MASK) || (status & PTRNG_STATUS_TOT_FAIL_MASK)) ? 1 : 0;
}

static u32 _ptrng_is_rep_finished(ptrng_address_e ptrng)
{
    u32 status = _get_ptrng_status(ptrng);
    return ((status & PTRNG_STATUS_RC_END_MASK) || (status & PTRNG_STATUS_RC_FAIL_MASK)) ? 1 : 0;
}

static u32 _ptrng_is_adapt_finished(ptrng_address_e ptrng)
{
    u32 status = _get_ptrng_status(ptrng);
    return ((status & PTRNG_STATUS_AP_END_MASK) || (status & PTRNG_STATUS_AP_FAIL_MASK)) ? 1 : 0;
}

static u32 _ptrng_is_total_test_pass(ptrng_address_e ptrng)
{
    // If Active, then continue tests
    if (_get_ptrng_test_arb_config_tot_ena(ptrng))
    {
        // Test that Total Tests have been run
        if (_ptrng_is_tot_finished(ptrng))
        {
            // No Total Alarm raised
            if (_get_ptrng_status_tot_fail(ptrng) == 0)
            {
                return 0x11;
            }
            else
            {
                return 0x14;
            }
        }
        else
        {
            return 0x13;
        }
    }
    // else
    return 0x12;
}

static u32 _ptrng_is_online_test_pass(ptrng_address_e ptrng)
{
    u32 cfg_value = _get_ptrng_test_dns_config(ptrng);

    // If Active, then continue tests
    if ((cfg_value & PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK) && (cfg_value & PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK))
    {
        // Test that Repetitive Count and Adaptive Tests have been ran
        if (_ptrng_is_rep_finished(ptrng) && _ptrng_is_adapt_finished(ptrng))
        {
            u32 statusValue = _get_ptrng_status(ptrng);

            if ((statusValue & PTRNG_STATUS_RC_FAIL_MASK) == 0 && (statusValue & PTRNG_STATUS_AP_FAIL_MASK) == 0)
            {
                return 0x11;
            }
            else if ((statusValue & PTRNG_STATUS_RC_FAIL_MASK) && ((statusValue & PTRNG_STATUS_AP_FAIL_MASK) == 0))
            {
                return 0x18;
            }
            else if ((statusValue & PTRNG_STATUS_AP_FAIL_MASK) && ((statusValue & PTRNG_STATUS_RC_FAIL_MASK) == 0))
            {
                return 0x19;
            }
            else                // Both failed
            {
                return 0x1A;
            }
        }
        else if (!_ptrng_is_rep_finished(ptrng))
        {
            return 0x15;
        }
        else if (!_ptrng_is_adapt_finished(ptrng))
        {
            return 0x16;
        }
        else                    // Both not run
        {
            return 0x17;
        }
    }
    // Repetition Count test is disabled
    else if ((cfg_value & PTRNG_TEST_DNS_CONFIG_RC_ENA_MASK) == 0)
    {
        return 0x12;
    }
    else if ((cfg_value & PTRNG_TEST_DNS_CONFIG_AP_ENA_MASK) == 0)
    {
        return 0x13;
    }
    else                        // both disabled
    {
        return 0x14;
    }
}

static void _ptrng_known_answer_test_init(ptrng_address_e ptrng)
{
    u32 value;

    _set_ptrng_status(ptrng, 0); // clear pending error (PTRNG_STATUS_ERROR_MASK)

    // Disable alarms
    _ptrng_wakeupline_alarm_set(ptrng, PTRNG_DISABLE);
    _ptrng_total_alarm_set(ptrng, PTRNG_DISABLE);

    // Gate clock before config update
    _ptrng_clk_gate(ptrng);

    // Force version 0
    _set_ptrng_config_t2d_ver(ptrng, PTRNG_CONFIG_T2D_VER_V0);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Force Arbiter out into a known state '1'
    _set_ptrng_dft_data(ptrng, 0xFFFFFFFF);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Force DFT as source
    _set_ptrng_config_dft_src_sel(ptrng, PTRNG_CONFIG_DFT_SRC_SEL_REG);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Set T2D config
    value = _get_ptrng_config(ptrng);
    _code_ptrng_config_t2d_cnt(&value, 7);
    _code_ptrng_config_pp_n_param(&value, 32);
    _set_ptrng_config(ptrng, value);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Enable tests
    value = _get_ptrng_test_arb_config(ptrng);
    _code_ptrng_test_arb_config_tot_ena(&value, PTRNG_ENABLE);
    _code_ptrng_test_arb_config_tot_cutoff(&value, 1);
    _set_ptrng_test_arb_config(ptrng, value);

    _ptrng_wait_conf_ready_ungated(ptrng);

    value = _get_ptrng_test_dns_config(ptrng);
    _code_ptrng_test_dns_config_rc_ena(&value, PTRNG_ENABLE);
    _code_ptrng_test_dns_config_ap_ena(&value, PTRNG_ENABLE);
    _code_ptrng_test_dns_config_rc_cutoff(&value, 2);
    _code_ptrng_test_dns_config_ap_cutoff(&value, 1);
    _set_ptrng_test_dns_config(ptrng, value);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Set DFT mode in PTRNG so all registers go into a known state
    _set_ptrng_dft_pp(ptrng, PTRNG_DFT_PP_DFT_PP_ENA);

    // Ungate clock after config update
    _ptrng_clk_ungate(ptrng);
}


static u32 _ptrng_known_answer_test_finish(ptrng_address_e ptrng)
{
    u32 status = 0;

    // Wait for generation
    if (_ptrng_status_wait(ptrng, PTRNG_STATUS_RN_READY_MASK, 1000) != TS_TRUE)
    {
        return 0xFF;
    }

    // Check the known answer
    if (_get_ptrng_rn_data(ptrng) != PTRNG_KNOWN_ANSWER)
    {
        status |= 0x01;
    }

    // Check that all tests have run at least once
    if (!_ptrng_is_tot_finished(ptrng))
    {
        status |= 0x02;
    }

    if (!_ptrng_is_rep_finished(ptrng))
    {
        status |= 0x04;
    }

    if (!_ptrng_is_adapt_finished(ptrng))
    {
        status |= 0x08;
    }

    // Check that all tests are fail before reading data
    if (_ptrng_is_total_test_pass(ptrng) != 0x14)
    {
        status |= 0x10;
    }

    if (_ptrng_is_online_test_pass(ptrng) != 0x1A)
    {
        status |= 0x20;
    }

    // Check that Alarm is risen
    if (!_ptrng_is_under_alarm(ptrng))
    {
        status |= 0x40;
    }

    // Stop forcing Arbiter, let it be
    _ptrng_disable_arb_out_test(ptrng);

    // Return status
    return status;
}

static u32 _ptrng_known_answer_test(ptrng_address_e ptrng)
{
    _ptrng_known_answer_test_init(ptrng);
    return _ptrng_known_answer_test_finish(ptrng);
}

static u32 _ptrng_startup_procedure_init(ptrng_address_e ptrng, u32 public_parameter)
{
    u32 value;
    u32 status = PTRNG_STARTUP_OK;

    // Disable online and total failure test alarms
    _ptrng_wakeupline_alarm_set(ptrng, PTRNG_DISABLE);
    _ptrng_total_alarm_set(ptrng, PTRNG_DISABLE);

    // Clock gate the RNG clock to start update configuration
    _ptrng_clk_gate(ptrng);

    // Configure and enable the digitalizer (T2D stage) and N post-processing parameter
    value = 0;
    _code_ptrng_config_t2d_ena(&value, PTRNG_CONFIG_T2D_ENA_ENA);
    _code_ptrng_config_t2d_ver(&value, PTRNG_CONFIG_T2D_VER_V1);
    _code_ptrng_config_t2d_edge(&value, PTRNG_CONFIG_T2D_EDGE_RISE);
    _code_ptrng_config_t2d_cnt_clear_ena(&value, PTRNG_CONFIG_T2D_CNT_CLEAR_ENA_ENA);
    _code_ptrng_config_t2d_data_ena(&value, PTRNG_CONFIG_T2D_DATA_ENA_DIS);
    _code_ptrng_config_t2d_cnt(&value, PTRNG_CONFIG_T2D_CNT);
    _code_ptrng_config_pp_n_param(&value, PTRNG_CONFIG_PP_N_PARAM);
    _set_ptrng_config(ptrng, value);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Configure and enable STARTUP HEALTH TESTS

    // Configure and enable the total failure test for startup health tests
    value = 0;
    _code_ptrng_test_arb_config_tot_ena(&value, PTRNG_TEST_ARB_CONFIG_TOT_ENA_ENA);
    _code_ptrng_test_arb_config_tot_cutoff(&value, PTRNG_TOTAL_FAILURE_CUTOFF);
    // Configure and enable the Mu test for startup health tests
    _code_ptrng_test_arb_config_mu_ena(&value, PTRNG_TEST_ARB_CONFIG_MU_ENA_ENA);
    _code_ptrng_test_arb_config_mu_once(&value, PTRNG_TEST_ARB_CONFIG_MU_ONCE_ONCE);
    _code_ptrng_test_arb_config_mu_win(&value, PTRNG_TEST_ARB_CONFIG_MU_WIN_1024);
    _code_ptrng_test_arb_config_mu_cutoff_min(&value, PTRNG_MU_MIN1_CUTOFF);
    _code_ptrng_test_arb_config_mu_cutoff_max(&value, PTRNG_MU_MAX1_CUTOFF);
    _set_ptrng_test_arb_config(ptrng, value);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Configure and enable the RC test for startup health tests
    value = 0;
    _code_ptrng_test_dns_config_rc_ena(&value, PTRNG_TEST_DNS_CONFIG_RC_ENA_ENA);
    _code_ptrng_test_dns_config_rc_cutoff(&value, PTRNG_RC_CUTOFF);
    // Configure and enable the AP test for startup health tests
    _code_ptrng_test_dns_config_ap_ena(&value, PTRNG_TEST_DNS_CONFIG_AP_ENA_ENA);
    _code_ptrng_test_dns_config_ap_source(&value, PTRNG_TEST_DNS_CONFIG_AP_SOURCE_RRN);
    _code_ptrng_test_dns_config_ap_size(&value, PTRNG_TEST_DNS_CONFIG_AP_SIZE_2BITS);
    _code_ptrng_test_dns_config_ap_win(&value, PTRNG_TEST_DNS_CONFIG_AP_WIN_64);
    _code_ptrng_test_dns_config_ap_cutoff(&value, PTRNG_AP_STARTUP_CUTOFF);
    _set_ptrng_test_dns_config(ptrng,value);

    _ptrng_clk_ungate(ptrng);
    _ptrng_wait_conf_ready(ptrng);

    // Wait for STARTUP HEALTH TESTS to be finished.
    u32 ptrng_tests_end_mask = PTRNG_STATUS_MU_END_MASK | PTRNG_STATUS_AP_END_MASK |
                                PTRNG_STATUS_TOT_END_MASK | PTRNG_STATUS_RC_END_MASK;

    if (_ptrng_status_wait(ptrng, ptrng_tests_end_mask, _PTRNG_HEALT_TEST_TIMEOUT) != TS_TRUE)
    {
        status |= PTRNG_STARTUP_TESTS_NOT_FINISHED;
    }

    if (_get_ptrng_status_tot_fail(ptrng))
    {
        status |= PTRNG_STARTUP_TOT_FAIL;
    }

    if (_get_ptrng_status_ap_fail(ptrng))
    {
        status |= PTRNG_STARTUP_AP_FAIL;
    }

    if (_get_ptrng_status_mu_fail(ptrng))
    {
        status |= PTRNG_STARTUP_MU_FAIL;
    }

    if (_get_ptrng_status_rc_fail(ptrng))
    {
        status |= PTRNG_STARTUP_RC_FAIL;
    }

    _ptrng_clk_gate(ptrng);

    // Disable STARTUP HEALTH TESTS
    _set_ptrng_test_arb_config_tot_ena(ptrng, PTRNG_TEST_ARB_CONFIG_TOT_ENA_DIS);

    _ptrng_wait_conf_ready_ungated(ptrng);

    _set_ptrng_test_arb_config_mu_ena(ptrng, PTRNG_TEST_ARB_CONFIG_MU_ENA_DIS);

    _ptrng_wait_conf_ready_ungated(ptrng);

    _set_ptrng_test_dns_config_ap_ena(ptrng, PTRNG_TEST_DNS_CONFIG_AP_ENA_DIS);

    _ptrng_wait_conf_ready_ungated(ptrng);

    _set_ptrng_test_dns_config_rc_ena(ptrng, PTRNG_TEST_DNS_CONFIG_RC_ENA_DIS);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Write diversified PI for post-processing
    _set_ptrng_pi(ptrng, public_parameter);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Configure and enable the RC test for online tests
    value = 0;
    _code_ptrng_test_dns_config_rc_ena(&value, PTRNG_TEST_DNS_CONFIG_RC_ENA_ENA);
    _code_ptrng_test_dns_config_rc_cutoff(&value, PTRNG_RC_CUTOFF);
    // Configure and enable the AP monobit test for online tests
    _code_ptrng_test_dns_config_ap_source(&value, PTRNG_TEST_DNS_CONFIG_AP_SOURCE_RRN);
    _code_ptrng_test_dns_config_ap_size(&value, PTRNG_TEST_DNS_CONFIG_AP_SIZE_1BIT);
    _code_ptrng_test_dns_config_ap_win(&value, PTRNG_TEST_DNS_CONFIG_AP_WIN_1024);
    _code_ptrng_test_dns_config_ap_cutoff(&value, PTRNG_AP_MONOBIT_CUTOFF);
    _code_ptrng_test_dns_config_ap_ena(&value, PTRNG_TEST_DNS_CONFIG_AP_ENA_ENA);
    _set_ptrng_test_dns_config(ptrng, value);

    _ptrng_wait_conf_ready_ungated(ptrng);

    // Configure and enable the total failure test for online tests
    value = 0;
    _code_ptrng_test_arb_config_tot_ena(&value, PTRNG_TEST_ARB_CONFIG_TOT_ENA_ENA);
    _code_ptrng_test_arb_config_tot_cutoff(&value, PTRNG_TOTAL_FAILURE_CUTOFF);
    // Configure and enable the Mu test for online tests
    _code_ptrng_test_arb_config_mu_cutoff_min(&value, PTRNG_MU_MIN2_CUTOFF);
    _code_ptrng_test_arb_config_mu_cutoff_max(&value, PTRNG_MU_MAX2_CUTOFF);
    _code_ptrng_test_arb_config_mu_win(&value, PTRNG_TEST_ARB_CONFIG_MU_WIN_2048);
    _code_ptrng_test_arb_config_mu_once(&value, PTRNG_TEST_ARB_CONFIG_MU_ONCE_CONTINUE);
    _code_ptrng_test_arb_config_mu_ena(&value, PTRNG_TEST_ARB_CONFIG_MU_ENA_ENA);
    _set_ptrng_test_arb_config(ptrng, value);

    _ptrng_clk_ungate(ptrng);

    return status;
}

static u32 _ptrng_startup_procedure_finish(ptrng_address_e ptrng)
{
    u32 status = PTRNG_STARTUP_OK;

    // Wait for PTRNG not busy
    _ptrng_wait_conf_ready(ptrng);

    // Wait for online tests to be finished at least once
    u32 ptrng_tests_end_mask = PTRNG_STATUS_MU_END_MASK | PTRNG_STATUS_AP_END_MASK |
                                PTRNG_STATUS_TOT_END_MASK | PTRNG_STATUS_RC_END_MASK;

    if (_ptrng_status_wait(ptrng, ptrng_tests_end_mask, _PTRNG_ONLINE_TEST_TIMEOUT) != TS_TRUE)
    {
        status |= PTRNG_STARTUP_TESTS_NOT_FINISHED;
    }

    if (_get_ptrng_status_tot_fail(ptrng))
    {
        status |= PTRNG_STARTUP_TOT_FAIL;
    }

    if (_get_ptrng_status_ap_fail(ptrng))
    {
        status |= PTRNG_STARTUP_AP_FAIL;
    }

    if (_get_ptrng_status_mu_fail(ptrng))
    {
        status |= PTRNG_STARTUP_MU_FAIL;
    }

    if (_get_ptrng_status_rc_fail(ptrng))
    {
        status |= PTRNG_STARTUP_RC_FAIL;
    }

    _ptrng_clk_gate(ptrng);
    os_delay_us(10);
    _ptrng_clk_ungate(ptrng);

    // Enable online and total failure test alarms
    _ptrng_wakeupline_alarm_set(ptrng, PTRNG_ENABLE);
    _ptrng_total_alarm_set(ptrng, PTRNG_ENABLE);

    return status;
}

static u32 _ptrng_startup_procedure(ptrng_address_e ptrng, u32 public_parameter)
{
    u32 status = _ptrng_startup_procedure_init(ptrng, public_parameter);

    if (status != PTRNG_STARTUP_OK)
    {
        LOG_ERROR_NUM(_PTRNG_ERR_STATUS);
        return status;
    }
    return _ptrng_startup_procedure_finish(ptrng);
}

void ptrng_init(void)
{
    // common configuration for both PTRNG
    // turn on PTRNG redundancy (otherwise test fail flags are always set)
    PTR32_T(TROPIC01_MEMORY_MAP_SCNTR_BASE_ADDR + SEC_CNTR_CONFIG_ADDR) |= (SEC_CNTR_CONFIG_PTRNG0_RED_EN_MASK | SEC_CNTR_CONFIG_PTRNG1_RED_EN_MASK);
}

ts_bool ptrng_startup_test(ptrng_address_e ptrng, u32 public_parameter)
{
    _PTRNG_SANITY_ADDR(ptrng);

    ptrng_wakeup(ptrng);

    // now TRNG is on, do power-up tests: known answer test and a startup procedure
    u32 status = _ptrng_known_answer_test(ptrng);
    if (status != PTRNG_STARTUP_OK)
    {
        return TS_FALSE;
    }

    status = _ptrng_startup_procedure(ptrng, public_parameter);
    ptrng_suspend(ptrng);
    return ((status == PTRNG_STARTUP_OK) ? TS_TRUE : TS_FALSE);
}


void ptrng_wakeup(ptrng_address_e ptrng)
{
    _PTRNG_SANITY_ADDR(ptrng);

    // turn on system clock
    soc_ctrl_clk_en((ptrng == PTRNG0) ? SOC_CTRL_CLK_TRNG1 : SOC_CTRL_CLK_TRNG2);

    // turn on analog and clocks
    _TRNG_REG_PTR(ptrng, PTRNG_ANALOG_CONFIG_ADDR) |= (PTRNG_ANALOG_CONFIG_PRNG_ENA_MASK | PTRNG_ANALOG_CONFIG_RNG_CLK_ENA_MASK);

    // needed to not trigger the ERROR flag
    _ptrng_ip_flush(ptrng);
}

void ptrng_suspend(ptrng_address_e ptrng)
{
    _PTRNG_SANITY_ADDR(ptrng);

    _TRNG_REG_PTR(ptrng, PTRNG_ANALOG_CONFIG_ADDR) &= ~(PTRNG_ANALOG_CONFIG_RNG_CLK_ENA_MASK);
    soc_ctrl_clk_dis((ptrng == PTRNG0) ? SOC_CTRL_CLK_TRNG1 : SOC_CTRL_CLK_TRNG2);
    // NOTE: we dont switch off PTRNG_ANALOG_CONFIG_PRNG_ENA because startup test must be performed always after analog power on
}

ts_bool ptrng_read(ptrng_address_e ptrng, u8 *dest, size_t len)
{
    u32 rnd;
    size_t n;

    OS_SANITY_NULL(dest);
    _PTRNG_SANITY_ADDR(ptrng);

    while (len)
    {
        if (_ptrng_status_wait(ptrng, PTRNG_STATUS_RN_READY_MASK, _PTRNG_DEFAULT_TIMEOUT) != TS_TRUE)
            return (TS_FALSE);

        rnd = _TRNG_REG_PTR(ptrng, PTRNG_RN_DATA_ADDR);
        n = (len >= sizeof(rnd)) ? sizeof(rnd) : len;
        memcpy(dest, &rnd, n);
        dest += n;
        len -= n;
    }

    return (_ptrng_is_under_alarm(ptrng) == 0) ? TS_TRUE : TS_FALSE;
}

void ptrng_t2d_data_mode(ptrng_address_e ptrng, ts_bool enable)
{
    _PTRNG_SANITY_ADDR(ptrng);
    _ptrng_clk_gate(ptrng);

    u32 value = _get_ptrng_config(ptrng);
    _code_ptrng_config_t2d_data_ena(&value, enable == TS_TRUE ? PTRNG_CONFIG_T2D_DATA_ENA_ENA : PTRNG_CONFIG_T2D_DATA_ENA_DIS);
    _set_ptrng_config(ptrng, value);

    _ptrng_clk_ungate(ptrng);
    _ptrng_wait_conf_ready(ptrng);
}
