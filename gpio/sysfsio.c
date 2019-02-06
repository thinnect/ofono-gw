#include "sysfsio.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>

bool sysfs_is_gpio_exported(uint32_t gpio_num) {
	char filename[FILENAME_BUF_LEN], errmsg[ERRMSG_BUF_LEN];
	struct stat stat_buf;

	snprintf(filename, FILENAME_BUF_LEN, SYSFS_GPIO_DIR "/gpio%u", gpio_num);
	if (stat(filename, &stat_buf) == 0) {
		if (S_ISDIR(stat_buf.st_mode)) {
			//printf("[%s] is directory\n", filename);
			return true;
		} else {
			printf("[%s] is not a directory\n", filename);
			return false;
		}
	} else {
		strerror_r(errno, errmsg, sizeof(errmsg));
		printf("[%s] not exported: %s\n", filename, errmsg);
		return false;
	}
}

struct timespec ts_diff(const struct timespec ts1, const struct timespec ts2) {
	struct timespec diff;
	assert(ts1.tv_sec > ts2.tv_sec || ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec >= ts2.tv_nsec);
	if (ts1.tv_nsec >= ts2.tv_nsec) {
		diff.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
		diff.tv_sec = ts1.tv_sec - ts2.tv_sec;
	} else {
		diff.tv_nsec = ts1.tv_nsec + 1000000000 - ts2.tv_nsec;
		diff.tv_sec = ts1.tv_sec - ts2.tv_sec - 1;
	}
	return diff;
}

uint32_t ts_to_ms(const struct timespec ts) {
	return (uint32_t) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

bool poll_file_rw(const char* filename, uint32_t timeout_ms) {
	struct timespec time_start, time_now;
	bool timeout = false;

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &time_start) < 0) {
		perror("poll_file_rw");
		return false;
	}

	do {
		// Poll a file until it's been created
		if (access(filename, F_OK) == 0) {
			// Now poll the file until its group is no longer 0 (root)
			// because otherwise users in gpio group don't have access
			if (access(filename, R_OK|W_OK) < 0) {
				printf("[%s] without RW access, waiting\n", filename);
			} else {
				printf("[%s] exists and has RW access (%u ms)\n", filename, ts_to_ms(ts_diff(time_now, time_start)) + STAT_POLL_DELAY_MS);
				return true;
			}
		} else {
			printf("[%s] not found, waiting\n", filename);
		}

		if (clock_gettime(CLOCK_MONOTONIC_RAW, &time_now) < 0) {
			perror("poll_file_rw");
			timeout = true;
		} else {
			if (ts_to_ms(ts_diff(time_now, time_start)) > timeout_ms) {
				printf("Timed out at %u ms\n", timeout_ms);
				timeout = true;
			} else {
				usleep(STAT_POLL_DELAY_MS * 1000);
			}
		}
	} while (!timeout);
	return false;
}

bool sysfs_gpio_export(uint32_t gpio_num) {
	FILE* fd = NULL;

	fd = fopen(SYSFS_GPIO_DIR "/export", "w");
	if (!fd) {
		perror("export/open file " SYSFS_GPIO_DIR "/export");
		return false;
	}

	if (fprintf(fd, "%u", gpio_num) < 0) {
		perror("export/write file " SYSFS_GPIO_DIR "/export");
		fclose(fd);
		return false;
	}

	if (fclose(fd) != 0) {
		perror("export/close file " SYSFS_GPIO_DIR "/export");
	}
	return true;
}

bool sysfs_gpio_unexport(uint32_t gpio_num) {
	FILE* fd = NULL;

	fd = fopen(SYSFS_GPIO_DIR "/unexport", "w");
	if (!fd) {
		perror("unexport/open file " SYSFS_GPIO_DIR "/unexport");
		return false;
	}

	if (fprintf(fd, "%u", gpio_num) < 0) {
		perror("unexport/write file " SYSFS_GPIO_DIR "/unexport");
		fclose(fd);
		return false;
	}

	if (fclose(fd) != 0) {
		perror("unexport/close file " SYSFS_GPIO_DIR "/unexport");
	}
	return true;
}

bool sysfs_gpio_output_value(uint32_t gpio_num, uint8_t value) {
	FILE* fd = NULL;
	char filename[FILENAME_BUF_LEN];

	snprintf(filename, FILENAME_BUF_LEN, SYSFS_GPIO_DIR "/gpio%u/direction", gpio_num);

	fd = fopen(filename, "w");
	if (!fd) {
		printf("Failed to open file\n");
		perror(filename);
		return false;
	}

	if (fprintf(fd, value ? "high" : "low") < 0) {
		printf("Failed to write to file\n");
		perror(filename);
		fclose(fd);
		return false;
	}

	if (fclose(fd) != 0) {
		perror(filename);
        }
	return true;
}

bool sysfs_gpio_set(uint32_t gpio_num, uint8_t value)
{
	bool export_state;
	char filename[FILENAME_BUF_LEN];

	snprintf(filename, FILENAME_BUF_LEN, SYSFS_GPIO_DIR "/gpio%u/direction", gpio_num);

	export_state = sysfs_is_gpio_exported(gpio_num);
	if (!export_state) {
		printf("Exporting gpio%u\n", gpio_num);
		if (!sysfs_gpio_export(gpio_num)) {
			return false;
		}
		// Wait for udev to set correct attributes to all files
		if (!poll_file_rw(filename, SYSFS_EXPORT_POLL_TIMEOUT_MS)) {
			printf("Timed out waiting for export to work!\n");
		} else {
			// We still need to sleep for a bit
			usleep(SYSFS_EXPORT_POST_WAIT_MS * 1000);
		}
	}
	if (!sysfs_gpio_output_value(gpio_num, value)) {
		return false;
	}

	return true;
}


// vim: autoindent tabstop=4 shiftwidth=4 softtabstop=4 noexpandtab
