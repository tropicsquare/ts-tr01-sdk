# Changelog

All notable changes to this project will be documented in this file.

## [2026-06-23]

### Fixed
- Flush SS request queue right before CHIP_STATUS.READY is set for the first time.

## [2026-05-05]

### Added
* add `shm_get_crc()` to get last value of SHM CRC calculated


### Removed
* remove Serial Subsystem reset from `spi_ll_init()` the READY bit is fully defined by chip POR reset/

## [2026-04-26]

### Added

#472867a: Add memclear32() function into util.c for fast memory clear
#d08cb17: Module arch.h with assembly macros


## [2026-04-22]

until #173614e no recorded changes
