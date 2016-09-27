/*
 * strs-parse.c
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

#include "strs-parse.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>

int strs_to_binary(void **dst, struct ffs_str_per_lang *str, int lang_nmb,
		int str_nmb)
{
	struct usb_functionfs_strings_head *header;
	int size = sizeof(*header);
	void *pos;
	int i = 0, j = 0, ret = 0;

	for (i = 0; i < lang_nmb; ++i) {
		size += sizeof(str[i].code);
		for (j = 0; j < str_nmb; ++j)
			size += strlen(str[i].str[j]) + 1;
	}

	pos = malloc(size);
	if (!pos) {
		ret = -1;
		goto out;
	}

	*dst = pos;
	ret = size;

	/* Fill header of functitonfs strings */
	header = pos;
	header->magic = htole32(FUNCTIONFS_STRINGS_MAGIC);
	header->length = htole32(size);
	header->str_count = htole32(str_nmb);
	header->lang_count = htole32(lang_nmb);
	pos += sizeof(*header);

	/* Fill strings */
	for (i = 0; i < lang_nmb; ++i) {
		*(typeof(str[i].code)*)pos = htole16(str[i].code);
		pos += sizeof(str[i].code);
		for (j = 0; j < str_nmb; ++j) {
			strcpy(pos, str[i].str[j]);
			pos += strlen(str[i].str[j]) + 1;
		}
	}

out:
	return ret;
}

static int get_str_list(config_setting_t *root, char ***strs)
{
	int len, i, tmp;
	config_setting_t *node;
	const char *str;

	if (config_setting_is_list(root) == CONFIG_FALSE) {
		CONFIG_ERROR(root, "Expected list");
		return ERROR_BAD_VALUE;
	}

	len = config_setting_length(root);
	*strs = calloc(len, sizeof(**strs));
	for (i = 0; i < len; ++i) {
		node = config_setting_get_elem(root, i);
		tmp = ffs_setting_get_string(node, &str);
		if (tmp < 0)
			return tmp;

		(*strs)[i] = strdup(str);
		if ((*strs)[i] == NULL) {
			CONFIG_ERROR(node, "error allocating memory");
			return ERROR_NO_MEM;
		}
	}

	return len;
}

int ffs_parse_str_config(config_setting_t *root, void **data)
{
	config_setting_t *list;
	config_setting_t *group;
	config_setting_t *node;
	int i, j;
	int len, str_nmb;
	int lang;
	int tmp;
	struct ffs_str_per_lang *res;

	list = config_setting_get_member(root, "strings");
	if (list == NULL) {
		CONFIG_ERROR(root, "strings not defined");
		return ERROR_NOT_FOUND;
	}

	if (config_setting_is_list(list) == CONFIG_FALSE) {
		CONFIG_ERROR(list, "Expected list");
		return ERROR_BAD_VALUE;
	}

	len = config_setting_length(list);
	res = calloc(len, sizeof(*res));
	if (res == NULL) {
		CONFIG_ERROR(list, "error allocating memory");
		return ERROR_NO_MEM;
	}

	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);
		if (group == NULL) {
			CONFIG_ERROR(list, "Unexpected error");
			tmp = ERROR_OTHER_ERROR;
			goto out;
		}

		node = config_setting_get_member(group, "lang");
		if (node == NULL) {
			CONFIG_ERROR(group, "lang not defined");
			tmp = ERROR_NOT_FOUND;
			goto out;
		}

		tmp = ffs_setting_get_int(node, &lang);
		for (j = 0; j < i; j++)
			if (lang == res[j].code) {
				CONFIG_ERROR(node, "lang %d defined more than once",
					res[i].code);
				tmp = ERROR_OTHER_ERROR;
				goto out;
			}

		if (tmp < 0)
			return tmp;

		node = config_setting_get_member(group, "strs");
		if (node == NULL) {
			CONFIG_ERROR(group, "strs not defined");
			tmp = ERROR_NOT_FOUND;
			goto out;
		}

		if (config_setting_is_list(list) == CONFIG_FALSE) {
			CONFIG_ERROR(list, "Expected list");
			return ERROR_BAD_VALUE;
		}

		tmp = config_setting_length(node);
		if (tmp < 0)
			goto out;

		if (i == 0)
		       str_nmb = tmp;

		if (str_nmb != tmp) {
			CONFIG_ERROR(node, "number of strings must be the same for each lang");
			tmp = ERROR_OTHER_ERROR;
			goto out;
		}

		res[i].code = lang;
	}

	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);
		node = config_setting_get_member(group, "strs");
		tmp = get_str_list(node, &res[i].str);
		if (tmp < 0)
			goto out;
	}

	tmp = strs_to_binary(data, res, len, str_nmb);

out:
	for (i = 0; i < len; i++) {
		if (res[i].str)
			for (j = 0; j < str_nmb; ++j)
				free(res[i].str[j]);
		free(res[i].str);
	}

	free(res);

	return tmp;
}
