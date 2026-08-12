#include "common.h"
#include "pwr.h"

#include "io_ops.h"
#include "soc_ctrl.h"
#include "soc_ctrl_regs.h"

#define _SOC_CTRL_REG_PTR(offset) PTR32_T(SOCCTRL_REG_MAP_BASE_ADDR + (offset))
#define _SOC_CTRL_REG_WRITE(offset, value) IO_WRITE_32(SOCCTRL_REG_MAP_BASE_ADDR+(offset), value)
#define _SOC_CTRL_REG_READ(offset) IO_READ_32(SOCCTRL_REG_MAP_BASE_ADDR+(offset))

// SOC_CTRL_PWR_CFG_VDD_1V2_0_CTRL_IMAX values
#define _VDD_1V2_I_0_25_TO_5 0 // 0.25 to 5mA
#define _VDD_1V2_I_1_TO_20   1 // 1 to 20 mA
#define _VDD_1V2_I_2_TO_40   2 // 2 to 40 mA
#define _VDD_1V2_I_3_TO_60   3 // 3 to 60 mA

// SOC_CTRL_PWR_CFG_VDD_2V5_0_STDBY values
#define _SUPPLY_NORMAL       0 //  Power supply operates normally
#define _SUPPLY_STANDBY      1 //  Power supply operates with reduced power consumption and limited current capabilites.


static u32 _pwr_cfg;

void pwr_suspend(void)
{
    u32 reg =  _SOC_CTRL_REG_READ(SOC_CTRL_PWR_CFG_ADDR);

    // backup current setup
    _pwr_cfg = reg & (SOC_CTRL_PWR_CFG_VDD_1V2_0_CTRL_IMAX_MASK | SOC_CTRL_PWR_CFG_VDD_2V5_0_STDBY_MASK);
    
    FIELD_SET(reg, SOC_CTRL_PWR_CFG_VDD_1V2_0_CTRL_IMAX_MASK, _VDD_1V2_I_0_25_TO_5);
    FIELD_SET(reg, SOC_CTRL_PWR_CFG_VDD_2V5_0_STDBY_MASK, _SUPPLY_STANDBY);
    
    _SOC_CTRL_REG_WRITE(SOC_CTRL_PWR_CFG_ADDR, reg);

    // switch off 2V5 LDO
    _SOC_CTRL_REG_PTR(SOC_CTRL_PWR_ENA_ADDR) &= ~SOC_CTRL_PWR_ENA_VDD_2V5_ENA_MASK;
}

void pwr_wakeup(void)
{
    u32 reg = _SOC_CTRL_REG_READ(SOC_CTRL_PWR_CFG_ADDR);
    
    // switch on 2V5 LDO
    _SOC_CTRL_REG_PTR(SOC_CTRL_PWR_ENA_ADDR) |= SOC_CTRL_PWR_ENA_VDD_2V5_ENA_MASK;
    os_delay_us(120);

    // restore previous setup
    reg &= ~(SOC_CTRL_PWR_CFG_VDD_1V2_0_CTRL_IMAX_MASK | SOC_CTRL_PWR_CFG_VDD_2V5_0_STDBY_MASK);
    reg |= _pwr_cfg;
    _SOC_CTRL_REG_WRITE(SOC_CTRL_PWR_CFG_ADDR, reg);
}



