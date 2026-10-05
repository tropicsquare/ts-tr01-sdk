/**
 * @file soc_ctrl.c
 * @author Tropic Square
 * @brief SoC control functions source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "soc_ctrl.h"
#include "io_ops.h"

#include <serial_subsystem_regs.h>

#include "os.h"
#include "spi.h"

#define _SOC_CTRL_REG_WRITE(offset, value) IO_WRITE_32(SOCCTRL_REG_MAP_BASE_ADDR+(offset), value)
#define _SOC_CTRL_REG_PTR(offset) PTR32_T(SOCCTRL_REG_MAP_BASE_ADDR + (offset))

static u32 _ss_int_en_bak;

void soc_ctrl_init(void)
{
    // enable Schmitt triggers for inputs, they are disabled by default (reset value = 0)
    _SOC_CTRL_REG_WRITE(SOC_CTRL_PIN_SMT_ADDR, SOC_CTRL_PIN_SMT_SPI_SMT_MASK | SOC_CTRL_PIN_SMT_TPDI_SMT_MASK);

    // set drive strength for outputs, the default is lowest strength (reset value = 0)
    _SOC_CTRL_REG_WRITE(SOC_CTRL_PIN_DS_ADDR, SOC_CTRL_PIN_DS_SPI_DS_MASK | SOC_CTRL_PIN_DS_GPO_DS_MASK);
    // NOTE: we dont set higher DS for TPDI because there is some HW limitation, which cause clock misbehavior
}

void soc_ctrl_clk_en(soc_ctrl_periph_clk_en_t peripherals)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_EN_ADDR) |= peripherals;
}

void soc_ctrl_clk_dis(soc_ctrl_periph_clk_en_t peripherals)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_EN_ADDR) &= ~peripherals;
}

void soc_ctrl_sclk_en(soc_ctrl_periph_sclk_t peripherals)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_SRC_ADDR) |= peripherals;
}

void soc_ctrl_sclk_dis(soc_ctrl_periph_sclk_t peripherals)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_SRC_ADDR) &= ~peripherals;
}

void soc_ctrl_clk_div(void)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_CFG_ADDR) |= SOC_CTRL_CLK_CFG_CLKDIV_MASK;
}

void soc_ctrl_sclk_mac_and_d_clk_en(void)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_SRC_ADDR) |= SOC_CTRL_SCLK_MAC_AND_D;
}

void soc_ctrl_sclk_spect_clk_en(void)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_SRC_ADDR) |= SOC_CTRL_SCLK_SPECT;
}

void soc_ctrl_sclk_cpb_clk_en(void)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_SRC_ADDR) |= SOC_CTRL_SCLK_CPB;
}

void soc_ctrl_pwr_on(u32 bits)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_PWR_ENA_ADDR) |= bits;
}

void soc_ctrl_pwr_off(u32 bits)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_PWR_ENA_ADDR) &= ~bits;
}

void soc_ctrl_reset(u32 bits)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_UTRESET_ADDR) = bits;
}

void soc_ctrl_sleep_prepare(void)
{
    // Enable OSC wakeup from SPI interface (Write any data)
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_CFG_ADDR) |= SOC_CTRL_CLK_CFG_SCK_NREQ_EN_MASK;

    // BACK up current state of interrupt enable of Serial subsystem
    _ss_int_en_bak = PTR32_T(SS_REG_MAP_BASE_ADDR + SS_INT_EN_ADDR);
    
    // Disable Serial Subsystem interrupts to avoid premature Interrupt execution when system wakes up.
    PTR32_T(SS_REG_MAP_BASE_ADDR + SS_INT_EN_ADDR) = 0;
}

void soc_ctrl_sleep_enter(void)
{
    // Never stop the oscillator while a SPI transfer is in progress (STATUS[CSN]=0).
    //
    // spi_set_ready() sets SS STATUS[NREQN] and arms the SS to clear its wake-up line
    // sck_reqq_nreq, but the SS applies that clear only while CSN is high. Stopping the
    // oscillator with the clear still armed is fatal: the next request restarts the
    // oscillator for a few cycles, the armed clear is applied in them and switches it off
    // again, and the register that then holds sck_reqq_nreq in reset is clocked by clk_sys,
    // so it stays asserted and no later request can raise the wake-up line. The chip is
    // dead until an external reset - the CPU cannot issue GRST, it has no clock.
    //
    // The trap loop below is no protection: STATUS[NREQN] was just set by us and reads 1.
    // Reading STATUS[CSN]=1 does prove the clear has already been applied.
    // (ETR01FW-265, see sck_reqq_nreq in ss_req_queue.sv)
    if (spi_idle() != TS_TRUE)
    {
        return; // transfer in progress - do not sleep, main loop retries next pass
    }

    // Going to sleep by disabling OSC 
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_CFG_ADDR) &= ~SOC_CTRL_CLK_CFG_OSCEN_MASK;
    while (PTR32_T(SS_REG_MAP_BASE_ADDR + SS_STATUS_ADDR) & SS_STATUS_NREQN_MASK)
    {
        // Trap loop (System will have a few more clock cycles to live)
    }
}

void soc_ctrl_sleep_leave(void)
{
    // Set back the clock enable (OSC en is currently driven by SS NREQ signal)
    // If not set back to one system would loose the OSC while another SPI transfer would be in progress
    _SOC_CTRL_REG_PTR(SOC_CTRL_CLK_CFG_ADDR) |= SOC_CTRL_CLK_CFG_OSCEN_MASK;

    // Recover the Serial Subsystem Int enable
    PTR32_T(SS_REG_MAP_BASE_ADDR + SS_INT_EN_ADDR) = _ss_int_en_bak;
}

void soc_ctrl_sleep_mode(void)
{
    soc_ctrl_sleep_prepare();
    soc_ctrl_sleep_enter();
    // here it continue after interrupt from serial subsystem
    soc_ctrl_sleep_leave();
}

void soc_ctrl_deep_sleep(void)
{
    _SOC_CTRL_REG_PTR(SOC_CTRL_PWR_ENA_ADDR) |= SOC_CTRL_PWR_ENA_PM_ON_CLR_MASK ;
    // code should never continue here
}

