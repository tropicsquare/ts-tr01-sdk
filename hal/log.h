/**
 * @file log.h
 * @author Tropic Square
 * @brief Logging support header file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef LOG_H
#define LOG_H

#include "type.h"

#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4

#ifndef TS_LOG_LEVEL
  /** @brief Set default log level if undefined. */
  #define TS_LOG_LEVEL LOG_LEVEL_NONE
#endif 

#if TS_LOG_SPI || TS_SIMULATION_BUILD
  /** @brief Enable log support. */
  #define LOG_LEVEL TS_LOG_LEVEL
#else
  /** @brief Disable log support, because we dont have any channel to see it. */
  #define LOG_LEVEL LOG_LEVEL_NONE
#endif 

/**
 * @def LOG_DEF 
 * @brief Define a log tag prefix for the current file/module.
 * @param prefix Tag string used to identify log messages from this module.
 */
#define LOG_DEF(prefix) static const char __attribute__((unused)) *LOG_TAG = prefix

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
  #define LOG_DEBUG( ... )   log_msg("D", LOG_TAG, __VA_ARGS__)
#else
  #define LOG_DEBUG( ... )  {}
#endif // LOG_LEVEL < LOG_LEVEL_DEBUG

#if LOG_LEVEL >= LOG_LEVEL_INFO
  #define LOG_INFO( ... )    log_msg("I", LOG_TAG, __VA_ARGS__)
#else
  #define LOG_INFO( ... )  {}
#endif // LOG_LEVEL < LOG_LEVEL_INFO

#if LOG_LEVEL >= LOG_LEVEL_WARNING
  #define LOG_WARNING( ... ) log_msg("W", LOG_TAG, __VA_ARGS__)
#else
  #define LOG_WARNING( ... ) {}
#endif // LOG_LEVEL < LOG_LEVEL_WARNING

#if LOG_LEVEL >= LOG_LEVEL_ERROR
  #define LOG_ERROR( ... )  log_msg("E", LOG_TAG, __VA_ARGS__)
  #define LOG_ERROR_NUM( ... )  log_err_num(LOG_TAG, __VA_ARGS__)
  #define LOG_DUMP( ... )   log_dump(LOG_TAG, __VA_ARGS__);
#else
  #define LOG_ERROR( ... ) {}
  #define LOG_ERROR_NUM( ... ) {}
  #define LOG_DUMP( ... ) {}
#endif // LOG_LEVEL < LOG_LEVEL_ERROR


/**
 * @brief Print a formatted log message.
 * 
 * @param type Log level identifier (e.g., "D" for debug).
 * @param id Tag or module name.
 * @param fmt printf-style format string.
 * @param ... Additional arguments corresponding to the format string.
 */
void log_msg(const ascii *type, const ascii *id, const ascii *fmt, ... );

/**
 * @brief Print an error message with a numeric error code.
 * 
 * @param id Tag or module name.
 * @param num Numeric error code.
 */
void log_err_num(const ascii *id, u32 num);

/**
 * @brief Print a hex dump of data to the log.
 * 
 * @param id Tag or module name.
 * @param text Description of the data.
 * @param data Pointer to the byte array.
 * @param data_len Length of the byte array.
 */
void log_dump(const ascii *id, const ascii *text, const u8 *data, size_t data_len);

#endif // ! LOG_H

