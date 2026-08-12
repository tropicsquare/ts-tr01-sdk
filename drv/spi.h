/**
 * @file spi.h
 * @author Tropic Square
 * @brief SPI HW driver header file.
 * 
  * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SPI_H
#define SPI_H

#include "type.h"

/** @brief received data (called from IRQ). */
typedef void (*spi_rx_callback_t)(u8 data);
/** @brief fetching data for TX (called from IRQ). */
typedef ts_bool (*spi_tx_callback_t)(u32 *dest);

/**
 * @brief Low level minimal SPI init to get chip status working.
 *
 * @param chip_status is the current status to be transmitter.
 */
void spi_ll_init(u8 chip_status);

/**
 * @brief Switch chip status flag to READY.
 */
void spi_set_ready(void);


/**
 * @brief Full SPI init for regular operation.
 *
 * @param rx_callback RX byte callback
 * @param tx_callback TX word fetching calback
 */
void spi_init(spi_rx_callback_t rx_callback, spi_tx_callback_t tx_callback);

/**
 * @brief Get state of SPI transaction.
 *
 * @return TS_TRUE if chip select (CSN) not active (HIGH).
 */
ts_bool spi_idle(void);

/**
 * @brief Get state or response queue.
 *
 * @return TS_TRUE if empty, TS_FALSE if not empty.
 */
ts_bool spi_response_queue_empty(void);

/**
 * @brief Clear the response queue instantly.
 */
void spi_clear_response_queue(void);

/**
 * @brief Discard any request bytes the SS latched in the request FIFO.
 *
 * Drains the request queue without resetting the serial subsystem, so requests
 * received before the interface is READY are dropped (not processed once READY).
 */
void spi_flush_request_queue(void);

/**
 * @brief Enable TX fifo fetching and clear busy flag.
 */
void spi_tx_enable(void);

/**
 * @brief Disable RX fifo fetching. Usually in alarm mode.
 */
void spi_rx_disable(void);

/**
 * @brief Direct write to TX FIFO.
 *
 * @param w 32bit word to write
 */
void spi_tx_push(u32 w);

/**
 * @brief Modify the chip status value.
 *
 * @param value is the current status to be transmitter as first byte of SPI communication
 */
void spi_set_chip_status(u8 value);

#endif // ! SPI_H

