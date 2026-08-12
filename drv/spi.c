/**
 * @file spi.c
 * @author Tropic Square
 * @brief SPI HW driver source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "os.h"
#include "spi.h"
#include "ts_l1_defs.h"
#include "ts_l2_defs.h"

#include "tassic_defs.h"
#include "gpo.h"
#include "cpu.h"
#include "irq_ctrl.h"
#include "io_ops.h"
#include "serial_subsystem_regs.h"
#include "soc_ctrl.h"

/** @brief x32 bit (G_REQ_QUEUE_DEPTH) */
#define _SS_FIFO_SIZE (64)
#define _SS_WATERMARK_SIZE (_SS_FIFO_SIZE/2)

#define _SS_REG_WRITE(offset, value) IO_WRITE_32(SS_REG_MAP_BASE_ADDR+(offset), value)
#define _SS_REG_READ(offset) IO_READ_32(SS_REG_MAP_BASE_ADDR+(offset))
#define _SS_REG_PTR32(offset) PTR32_T(SS_REG_MAP_BASE_ADDR+(offset))

#define _GPO_IRQ_SET() gpo_int_set()
#define _GPO_IRQ_CLR() gpo_int_clr()


static spi_rx_callback_t _rx_byte = NULL;
static spi_tx_callback_t _tx_fetch_word = NULL;

// Shared between the SPI IRQ (irq_ss_handler) and the main loop (_tx_reset via
// spi_tx_enable/spi_clear_response_queue); volatile so LTO cannot cache it. (ETR01FW-278)
static volatile bool _tx_fetch_done = false;

static void _watermark_reset(void)
{   // set default watermark 
    // watermark is number of 32bit words
    _SS_REG_WRITE(SS_WATERMARK_ADDR, (_SS_WATERMARK_SIZE << SS_WATERMARK_REQQLVL_POS) | (_SS_WATERMARK_SIZE << SS_WATERMARK_RSPQLVL_POS));
}

static void _tx_reset(void)
{
    _SS_REG_PTR32(SS_INT_EN_ADDR) &= ~SS_INT_EN_RSPQWML_MASK;
    _GPO_IRQ_CLR();
    _watermark_reset();
    _tx_fetch_done = false;
}

void spi_ll_init(u8 chip_status)
{
    // enable periphery clock
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_SSCLKEN_MASK);
    // enable minimal SPI functionality
    _SS_REG_WRITE(SS_CONFIG_ADDR, SS_CONFIG_SPIENA_MASK | SS_CONFIG_FBSTAT_MASK | (chip_status << SS_CONFIG_FBVAL_POS));
    // set codes for proper "NO_RESP" result
    _SS_REG_WRITE(SS_CODES_ADDR, (TS_L2_GET_RESP << SS_CODES_GET_RESP_POS) | (TS_L2_RESP_NO_RESP << SS_CODES_NO_RESP_POS));
}

void spi_set_ready(void)
{
    _SS_REG_WRITE(SS_STATUS_ADDR, SS_STATUS_NREQN_MASK);
}

void spi_init(spi_rx_callback_t rx_callback, spi_tx_callback_t tx_callback)
{
    OS_SANITY_NULL(rx_callback);
    OS_SANITY_NULL(tx_callback);

    spi_ll_init(TS_CHIP_ST_STARTUP);

    _watermark_reset();

    _rx_byte = rx_callback;
    _tx_fetch_word = tx_callback;

    // Discard any L2 request bytes the SS latched during the boot/START window,
    // before the interface is READY. This replaces the SSRST formerly done in
    // spi_ll_init() (removed in ETR01FW-230 to avoid the chip-status START-flag
    // glitch) without resetting the subsystem. Must run before enabling the
    // request IRQ so a stale request is dropped instead of delivered to _rx_byte.
    spi_flush_request_queue();

    // enable IRQ
    _SS_REG_WRITE(SS_INT_EN_ADDR, 0 \
        | SS_INT_EN_REQQNETY_MASK    // request queue empty (SS_INT_STATUS_REQQNETYS_MASK)
        );

    // By default is READY flag inactive, make it active now, when interface is ready
    spi_set_ready();
}

ts_bool spi_idle(void)
{
    return ((_SS_REG_READ(SS_STATUS_ADDR) & SS_STATUS_CSN_MASK) ? TS_TRUE : TS_FALSE); 
}

ts_bool spi_response_queue_empty(void)
{
    u32 tmp = _SS_REG_READ(SS_RSPQPTR_ADDR);

    if (((tmp & SS_RSPQPTR_RQWP_MASK) >> SS_RSPQPTR_RQWP_POS) != ((tmp & SS_RSPQPTR_RQRP_MASK) >> SS_RSPQPTR_RQRP_POS))
    {
        return TS_FALSE; // RQWP != RQRP
    }
    // RQWP == RQRP, but it may be fifo empty or full
    if (_SS_REG_READ(SS_STATUS_ADDR) & SS_STATUS_RSPQFULLS_MASK)
    {
        return TS_FALSE;
    }
    // response queue is empty
    return TS_TRUE;
}

static inline ts_bool _response_queue_full(void)
{
    return ((_SS_REG_READ(SS_STATUS_ADDR) & SS_STATUS_RSPQFULLS_MASK) ? TS_TRUE : TS_FALSE);
}

ts_bool spi_request_queue_empty(void)
{
    return ((_SS_REG_READ(SS_STATUS_ADDR) & SS_STATUS_REQQNETYS_MASK) ? TS_FALSE : TS_TRUE);
}

void spi_clear_response_queue(void)
{
    _tx_reset();
    _SS_REG_PTR32(SS_COMMAND_ADDR) |= SS_COMMAND_RSPQCLR_MASK;

    while (_SS_REG_READ(SS_COMMAND_ADDR) & SS_COMMAND_RSPQCLR_MASK)
        ;
}

void spi_flush_request_queue(void)
{   // there is no HW request-queue-clear command (SS_COMMAND has RSPQCLR only),
    // so drain the request FIFO by popping it empty; mirrors the pop in irq_ss_handler()
    while (spi_request_queue_empty() != TS_TRUE)
    {
        (void)_SS_REG_READ(SS_REQQ_POP_ADDR);
    }
}

void spi_tx_enable(void)
{   // enable IRQ for fetching data
    if (_tx_fetch_word == NULL)
    {
        return;
    }
    _tx_reset();
    _SS_REG_PTR32(SS_INT_EN_ADDR) |= SS_INT_EN_RSPQWML_MASK;
    spi_set_ready();
    _GPO_IRQ_SET();
}

void spi_rx_disable(void)
{
    // disable the receiving irq
    _SS_REG_PTR32(SS_INT_EN_ADDR) &= ~SS_INT_EN_REQQNETY_MASK;
}

void spi_set_chip_status(u8 value)
{
    u32 reg = _SS_REG_READ(SS_CONFIG_ADDR) & ~SS_CONFIG_FBVAL_MASK;

    reg |= ((value << SS_CONFIG_FBVAL_POS) & SS_CONFIG_FBVAL_MASK);
    _SS_REG_WRITE(SS_CONFIG_ADDR, reg);
}

void spi_tx_push(u32 w)
{
    _SS_REG_WRITE(SS_RSPQ_PUSH_ADDR, w);
}

__ISR void irq_ss_handler(void)
{
    if (_SS_REG_READ(SS_STATUS_ADDR) & SS_STATUS_REQQNETYS_MASK)
    {   // Request Queue not empty status
        u32 data;
        u32 i;

        data = _SS_REG_READ(SS_REQQ_POP_ADDR);
        for (i=0; i<sizeof(u32); i++)
        {
            _rx_byte(data & 0xFF);
            data >>= 8;
        }
        return;
    }
    // if it is not REQQNETYS, it must be RSPQWML
    if (_tx_fetch_done == true)
    {   // almost done, in RSP queue is less than 4B of data
        _tx_reset();
        return;
    }
    while (1)
    {   // fill whole queue
        u32 w;

        if (_response_queue_full() == TS_TRUE)
        {   // fifo full, postpone filling until bellow watermark 
            return;
        }

        if (_tx_fetch_word(&w) != TS_TRUE)
        {   // end of data
            _tx_fetch_done = true;
            // dont switch off IRQ, we have to deassert GPO_IRQ once transfer done
            // set RSPQ watermark to one word 
            _SS_REG_WRITE(SS_WATERMARK_ADDR, (_SS_WATERMARK_SIZE << SS_WATERMARK_REQQLVL_POS) | (1 << SS_WATERMARK_RSPQLVL_POS));
            // we will get IRQ when in respone queue will last less then 4B of data
            return;
        }
        // write u32 to fifo
        spi_tx_push(w);
    }
}

