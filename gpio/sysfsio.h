#ifndef _SYSFSIO_H_
#define _SYSFSIO_H_

#include <stdbool.h>
#include <stdint.h>

/*
A simple module for setting GPIO lines.
We use the Linux kernel's sysfs gpio interface located at
/sys/class/gpio to export a line and set its output.
Input is not supported.
*/

#define GPIO_LINES_PER_CHIP				32
#define GPIO_CHIPS						4
#define MAX_GPIO_NUM					(GPIO_LINES_PER_CHIP * GPIO_CHIPS - 1)
#define SYSFS_GPIO_DIR					"/sys/class/gpio"
#define FILENAME_BUF_LEN				(sizeof(SYSFS_GPIO_DIR) + 50)
#define ERRMSG_BUF_LEN					256
#define STAT_POLL_DELAY_MS				5
#define SYSFS_EXPORT_POLL_TIMEOUT_MS	1000
#define SYSFS_EXPORT_POST_WAIT_MS		1

/**
  Check if given GPIO line is exported
  @param gpio_num Number of GPIO to check
  @return true if exported, false otherwise
 */
bool sysfs_is_gpio_exported(uint32_t gpio_num);

struct timespec ts_diff(const struct timespec ts1, const struct timespec ts2);

uint32_t ts_to_ms(const struct timespec ts);

bool poll_file_rw(const char* filename, uint32_t timeout_ms);

bool sysfs_gpio_export(uint32_t gpio_num);

bool sysfs_gpio_unexport(uint32_t gpio_num);

bool sysfs_gpio_output_value(uint32_t gpio_num, uint8_t value);

/**
  Set value to given GPIO line (export GPIO line if needed)
  @param gpio_num Number of GPIO to set
  @param value Output value (0 or 1)
  @return true if operation succeeded, false otherwise
*/
bool sysfs_gpio_set(uint32_t gpio_num, uint8_t value);

#endif // _SYSFSIO_H_
