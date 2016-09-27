/*
 * desc-parse.h
 * Copyright(c) 2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DESC_PARSE_H
#define DESC_PARSE_H

#include <libconfig.h>
#include <linux/usb/functionfs.h>

struct ffs_desc_per_speed {
	int desc_size;
	int desc_count;
	char *desc;
} __attribute__ ((__packed__));

enum ffs_usb_max_speed {
#ifdef HAS_FFS_DESC_V2
	FFS_USB_FULL_SPEED = FUNCTIONFS_HAS_FS_DESC,
	FFS_USB_HIGH_SPEED = FUNCTIONFS_HAS_HS_DESC,
	FFS_USB_SUPER_SPEED = FUNCTIONFS_HAS_SS_DESC,
	FFS_USB_TERMINATOR = FFS_USB_SUPER_SPEED << 1
#else
	FFS_USB_FULL_SPEED = 1,
	FFS_USB_HIGH_SPEED = FFS_USB_FULL_SPEED << 1,
	FFS_USB_TERMINATOR = FFS_USB_HIGH_SPEED << 1
#endif
};

/**
 * @brief Convert list of descriptors to binary data
 * @param[out] dst Pointer to destination pointer
 * @param[in] desc List of descriptors
 * @param[in] mask Bit mask containing flags set in given descriptor
 * @return Number of bytes in output if succeed or negative number if failed
 */
int desc_to_binary(void **dst, struct ffs_desc_per_speed *desc, int mask);

/**
 * @brief Parse descriptors from configuration to binary data
 * @param[in] root Pointer to root of the configuration
 * @param[out] data Binary data containing descriptors present in configuration
 */
int ffs_parse_desc_config(config_setting_t *root, void **data, int str_count);

#endif
