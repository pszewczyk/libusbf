/*
 * common.h
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

#ifndef COMMON_H
#define COMMON_H

#include <libconfig.h>

typedef enum  {
	SUCCESS = 0,
	ERROR_NOT_FOUND = -1,
	ERROR_NOT_SUPPORTED = -2,
	ERROR_FILE_OPEN_FAILED = -4,
	ERROR_BAD_VALUE = -5,
	ERROR_NO_MEM = -6,
	ERROR_INVALID_PARAM = -7,
	ERROR_NOT_DEFINED = -8,
	ERROR_OTHER_ERROR = -99
} error;

enum {
	OPT_FORCE = 0x01,
};

/**
 * @brief structure storing name of variable and it's value
 **/
struct gd_named_const {
	const char *name;
	int len;
	int value;
};

#define DECLARE_ELEMENT(name) {#name, sizeof(#name)-1, name}
#define DECLARE_END() {NULL, 0, 0}

/**
 * @brief Find value with given name on list
 * @param[in] name Name of value to be find
 * @param[in] len Lenth of name
 * @param[in] keys List of values to be searched
 * @param[out] res Pointer to integer to be filled with found value
 */
int get_const_value(const char *name, int len, struct gd_named_const *keys, int *res);

/**
 * @brief Get string from given libconfig setting
 * @details Assume that given setting is string and report errors
 * when failed.
 * @param[in] root Setting which should be string
 * @param[out] str Pointer to value of given setting
 * @return Error code if failed
 */
int ffs_setting_get_string(config_setting_t *root, const char **str);

/**
 * @brief Get string from given libconfig setting
 * @details Assume that given setting is integer and report errors when failed.
 * @param[in] root Setting which should be string
 * @param[out] str Pointer to value of given setting
 * @return Error code if failed
 */
int ffs_setting_get_int(config_setting_t *root, int *dst);

#define CONFIG_ERROR(node, msg, ...)\
	fprintf(stderr, "%s:%d: %s: "msg" \n",\
			config_setting_source_file(node),\
			config_setting_source_line(node),\
			(node->name ? node->name : ""), ##__VA_ARGS__);

typedef enum {
	FFS_DESC_FORMAT_V1,
	FFS_DESC_FORMAT_V2,
	FFS_DESC_FORMAT_END
} FFS_DESC_FORMAT;

extern FFS_DESC_FORMAT desc_format;

/**
 * Flags set by user in command line
 */
extern int cmd_flags;

/**
 * @brief Get descriptor format from string
 */
int ffs_desc_format_from_str(const char *str);

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

#endif /* COMMON_H */
