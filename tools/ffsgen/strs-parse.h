/*
 * strs-parse.h
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

#ifndef STRS_PARSE_H
#define STRS_PARSE_H

#include <libconfig.h>
#include <linux/usb/functionfs.h>

struct ffs_str_per_lang {
	__le16 code;
	char **str;
} __attribute__ ((__packed__));

/**
 * @brief Convert list of strings to binary data
 * @param[out] dst Pointer to destination pointer
 * @param[in] str List of strings
 * @param[in] lang_nmb Number of used languages
 * @param[in] str_nmb Number of strings in each language
 * @return Number of bytes in output if succeed, negative number otherwise.
 */
int strs_to_binary(void **dst, struct ffs_str_per_lang *str, int lang_nmb,
		int str_nmb);
/**
 * @brief Parse strings from configuration to binary data
 * @param[in] root Pointer to root of the configuration
 * @param[out] data Binary data containing strings present in configuration
 */
int ffs_parse_str_config(config_setting_t *root, void **data);

#endif /* STRS_PARSE_H */
