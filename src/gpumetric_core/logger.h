#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/* ANSI terminal color escape codes for styled console output */
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

/**
 * @brief Logs an error message to stdout in red color.
 */
#define LOG_ERROR(fmt, ...) \
    printf(ANSI_RED "[ERROR] " ANSI_RESET fmt "\n", ##__VA_ARGS__)

/**
 * @brief Logs a warning message to stdout in yellow color.
 */
#define LOG_WARN(fmt, ...) \
    printf(ANSI_YELLOW "[WARN] " ANSI_RESET fmt "\n", ##__VA_ARGS__)

/**
 * @brief Logs an informational message to stdout in green color.
 */
#define LOG_INFO(fmt, ...) \
    printf(ANSI_GREEN "[INFO] " ANSI_RESET fmt "\n", ##__VA_ARGS__)

/**
 * @brief Logs a debug message to stdout in cyan color.
 */
#define LOG_DEBUG(fmt, ...) \
    printf(ANSI_CYAN "[DEBUG] " ANSI_RESET fmt "\n", ##__VA_ARGS__)

#endif