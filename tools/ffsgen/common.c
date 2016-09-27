/*
 * common.c
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

#include "common.h"
#include <string.h>

#ifdef HAS_FFS_DESC_V2
FFS_DESC_FORMAT desc_format = FFS_DESC_FORMAT_V2;
#else
FFS_DESC_FORMAT desc_format = FFS_DESC_FORMAT_V1;
#endif

int cmd_flags = 0;

static const char *ffs_desc_format_strs[] = {
	[FFS_DESC_FORMAT_V1] = "v1",
	[FFS_DESC_FORMAT_V2] = "v2",
};

int ffs_desc_format_from_str(const char *str)
{
	int ret;

	for (ret = FFS_DESC_FORMAT_V1; ret < ARRAY_SIZE(ffs_desc_format_strs); ++ret)
		if (strcmp(str, ffs_desc_format_strs[ret]) == 0)
			break;

	if (ret == ARRAY_SIZE(ffs_desc_format_strs))
		ret = -1;

#ifndef HAS_FFS_DESC_V2
	if (ret == FFS_DESC_FORMAT_V2)
		ret = -1;
#endif

	return ret;
}

int ffs_setting_get_string(config_setting_t *root, const char **str)
{
	if (config_setting_type(root) != CONFIG_TYPE_STRING) {
		CONFIG_ERROR(root, "must be string");
		return ERROR_BAD_VALUE;
	}

	*str = config_setting_get_string(root);

	return SUCCESS;
}

int ffs_setting_get_int(config_setting_t *root, int *dst)
{
	if (config_setting_type(root) != CONFIG_TYPE_INT) {
		CONFIG_ERROR(root, "must be integer");
		return ERROR_BAD_VALUE;
	}

	*dst = config_setting_get_int(root);

	return SUCCESS;
}

int get_const_value(const char *name, int len, struct gd_named_const *keys, int *res)
{
	while (keys->name != NULL) {
		if (keys->len == len
		    && strncmp(name, keys->name, len) == 0) {
			*res = keys->value;
			return SUCCESS;
		}
		keys++;
	}
	return ERROR_NOT_FOUND;
}
