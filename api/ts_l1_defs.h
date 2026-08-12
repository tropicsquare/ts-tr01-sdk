
#ifndef TS_L1_DEFS_H
#define TS_L1_DEFS_H

// chip status values (first SPI byte content)
//  TS_CHIP_ST_READY is handled by HW NREQ
#define TS_CHIP_ST_IDLE            (0)
#define TS_CHIP_ST_ALARM      (1 << 0)
#define TS_CHIP_ST_STARTUP    (1 << 1)
#define TS_CHIP_ST_BOOT_HOLD  (1 << 2)
// final value of chip-status will be: (TS_CHIP_ST_* << 1) | SS_STATUS[NREQ]

#endif // ! TS_L1_DEFS_H
