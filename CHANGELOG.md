# Changelog

All notable changes to this project will be documented in this file.

## [2026-09-23]

### Added
- `memzero_safe()` function for overwriting a memory region with zeros.
- `flash_clear_sector_cache()` function for erasing the driver sector cache in CPU memory.
- `sec_cntr_get_active_sensors()` returning the set of enabled alarm channels, needed for the freq mon disable during sleep in App.
- In `type.h`:
  - macro `TS_CHECK_RETVAL` wrapping the `warn_unused_result` function attribute,
  - macro `TS_IGNORE_RESULT` to explicitly discard a function result marked warn_unused_result.
- Unit tests for the MBIST driver (`test_mbist.c`), running the driver against a behavioral model of the TSMBIST engine.
- `flash.h`: `FLASH_MAX_ENCRYPTED_PAYLOAD` macro with value 475 - the maximum usable encrypted payload.

### Changed
- PRNG: `prng_get_value_insecure()` renamed to `prng_read()`, returns `ts_bool` and delivers the value through an output parameter. If the PRNG is not seeded yet, the function does not enter alarm anymore and returns `TS_FALSE`.
- `memerase_safe()`: while the PRNG is not seeded yet, the region is overwritten with zeros, so the erase itself never fails.
- `ui_callback_t`: change return value type to `void` - the handler answers the host itself, so `ui_task()` has no result to act on.
- `cpb_read_data()`: change return value type to `void` - the function cannot fail.
- `sys_init()`: change return value type to `void` - the function cannot fail.
- `_fss_command_exec()`, `_flash_cfg_basic()`: change return value type to `void` - the functions cannot fail.
- SCB: removed return values of `scb_tstwrp_process_op()` and `scb_tstwrp_mov_data_in()`.
- `mbist.h` API returns `ts_bool` instead of `u32` and is marked `TS_CHECK_RETVAL`:
  - `mbist_exec_test()` and `mbist_erase()` return `TS_TRUE` on success,
  - `mbist_exec_test()` no longer returns the mask of failed channels and returns `TS_FALSE` if `test_type == MBIST_TEST_CRC`,
  - defines `MBIST_RES_OK` and `MBIST_RES_FAIL` removed.
- `mbist_erase()` in `mbist.c` restores the `CONFIG` register of the caller instead of clearing it.
- `mbist_prepare_test()` removed and merged into `mbist_exec_test()` to make API more foolproof.
- `_mbist_reset()`: keep the test setup (`CONFIG[MODE]`, `INT_EN`, `PATTERN`, `RETENTION_1`, `RETENTION_2`) after reset (`CONFIG[MBIST_EN]`, `CONFIG[TEST_TYPE]`, `CONFIG[DATA_INDEX]` and `MEM_SEL` are left in reset values on purpose).
- SPI driver: removed `spi_ll_init(TS_CHIP_ST_STARTUP)` call from `spi_init()` (should be called manually before calling `spi_init()` with appropriate CHIP_STATUS value).
- FSS driver: handle ECC_SEC_F via interrupt.
- `irq_flash_handler()`: handle all flags of one STATUS snapshot in a single pass (fatal ones first) instead of an `else if` chain that serviced only the first match.
- `flash_read_sector_enc()`: takes a new `dest_size` parameter, so the driver bounds the copy by the caller's buffer instead of by the sector capacity.
- Build flags: `-fno-strict-aliasing` (`toolchain.cmake`, for the packed L2/L3 protocol structs overlaying byte buffers) and `-Wcast-align=strict` (`sdk.cmake`). Both are inherited by everything built against the SDK.

### Removed
- `memclear32()` function - superseded by `memzero_safe()`.
- `flash.h`: the `flash_read_buf()` declaration, which had no definition.

### Fixed
- `_msg_rx_byte()`: the receive buffer bound used `OS_ASSERT`, which called `os_alarm()` from the SPI IRQ and left the interrupts masked for the whole alarm cleanup. Now the frame is dropped and `os_alarm_isr()` hands the alarm to the main loop.
- `spect_init()`:
  - wait for STATUS[IDLE] with 1ms timeout and check only the IDLE bit (before, all other bits in STATUS had to be 0),
  - enable interrupts and clear RAM only after it is checked that STATUS[IDLE]=1.
- SCB: ignored timeout while waiting for any SCB operation to finish. Now, alarm is entered if any SCB operation is not completed within `_SCB_DEFAULT_TIMEOUT` us.
- `ui_response()`/`ui_response_async()`: update the `RESEND_REQ` retry buffers only after `msg_tx_send()` really queued the message. A rejected message used to become the stored "last response" and break every later `RESEND_REQ`.
- `msg_tx_send()`: log the rejection of an oversized message, which would overflow `_tx_stream.data` into the IRQ-shared `ptr`/`len`.
- `msg_tx_send()`: `msg` NULL check uses `OS_ASSERT`, not `OS_SANITY_NULL` (can be compiled out with `OS_SANITY_DISABLE=1`).
- `_mbist_run()`: wait for the test to finish with timeout, so a corrupted or unresponsive TSMBIST engine can't lock the FW up.
- `mbist_exec_test()`, `mbist_erase()`: correctly verify if a test passed by checking registers `TEST_PROGRESS` and `TEST_ERROR` are cleared, and `TEST_RESULT` is set (given by the TSMBIST design specification).
- `mbist_erase()`: verify that test preparation was successful before running the `MEM_CLR` test.
- `_mbist_prepare()`: set `CONFIG[MBIST_EN]` after `MEM_SEL` and before `COMMAND[PREPARE]` so the preparation takes effect and can be verified.
- MBIST: reset on a timeout to stop any running test.
- `_mbist_reset()`: wait 5 cycles of the MBIST clock after reset (required by the TSMBIST design specification).
- `mbist_init()`, `mbist_erase()`: use `MBIST_INT_EN_DONE_EN_MASK` instead of `MBIST_STATUS_DONE_MASK` to enable the DONE interrupt.
- `BITS_PER_WORD` in `bits.h`: derived from the compiler instead of being hardcoded to 32 (needed for unit tests that run on on 64 bit hosts).
- `soc_ctrl_sleep_enter()`: do not stop the oscillator while `SS_STATUS[CSN]` is low. A `STATUS[NREQN]=1` write issued during an SPI transfer is applied by the SS only when CSN rises; stopping the oscillator before that strands the clear and leaves `sck_reqq_nreq` held in async reset, so no further request can restart the oscillator and the chip is dead until power-cycle. Note the function (and thus `soc_ctrl_sleep_mode()` / `os_sleep_mode()`) may now return without having slept - callers must tolerate that.
- `crt.s`: disable maskable interrupts at the very beginning of the startup code (prevents corrupting low memory and clobbering the registers used to initialize `MTVEC` and the stack).
- FSS driver: connect ECC_SEC_R and ECC_DED_R errors to interrupts. This way, if any FSS operation executed by any HW block raises these errors, the errors are handled with alarm as soon as possible.
- `flash_read_sector_enc()` and `flash_write_sector_enc()`: bound the payload by `FLASH_MAX_ENCRYPTED_PAYLOAD` (475) instead of the `SECT_CTEXT` field width (476), so the driver never issues an operation that trips the FSS `SECT_DLENGTH` comparator. Writing 476 B previously returned `TS_FALSE` while the sector was still programmed, leaving the slot occupied and unreadable.
- `flash_write_word_isr()`: reject `address == FLASH_SIZE`, (LLM SA finding SDK-BOOT-002).
- FSS encrypted sectors: erase the plaintext sector cache in `flash_read_sector_enc()` (including the early return on an invalid stored size) and flush the FSS RAM buffer after `flash_write_sector_enc()`, on both the success and the failure path (LLM SA finding SDK-APP-005).
- `_timer_acknowledge_irq()`: build the acknowledge mask with `|` instead of `+` (LLM SA finding SDK-APP-004). The two masks are disjoint bits, so the value is unchanged.

## [2026-07-29]

### Added
- `memerase_safe` function as secure alternative to `memset`
  - unit tests for `memerase_safe`
  - use `memerase_safe` in suitable places in SDK

### Fixed
- Make the following variables `volatile`, as they are set in ISRs:
  - `_otp_prog_done` (`otp.c`),
  - `_ctx.op_done` (`scb.c`),
  - `_alarm_memory` (`sec_cntr.c`),
  - `_spect_done`, `_spect_error` (`spect.c`).
- `scb_decrypt_data()` in `scb.c`:
  - change `chunk_size` to `size_t` to fix `bugprone-narrowing-conversions`,
  - rework loop to count down, preventing potential counter overflow and infinite loops,
  - advance data pointers by `chunk_size` (not fixed block size) to prevent out-of-bounds memory access on partial blocks.
- `_load_hmac_message_data()` in `scb.c`: the `data` variable is now incremented only in scopes where it is proven that `len` is bigger than `_SCB_REG_SIZE`.
- `FORM_OP` macro in `scb.c`: cast lhs to `u32` to prevent undefined behavior when shifting `int` by 31 bits.
- `t_scb_ctx.force_aes_mask` in `scb.c`: change type from `int` to `u32` to prevent undefined behavior when shifting by 31 bits.

## [2026-07-08]

### Added
- `ui_resend_served()` and `ui_set_stream_active()` so the L3 streaming task can defer the next-chunk advance by one pass across a `RESEND_REQ`.

### Fixed
- `RESEND_REQ` during an L3 result stream could advance to the next chunk; the async-retry block now also arms while `_stream_active` is set (covering the window after the last committed chunk was promoted but the stream is not done).

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
