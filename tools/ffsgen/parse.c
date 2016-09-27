/*
 * parse.c
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

#include "parse.h"
#include "desc-parse.h"
#include "strs-parse.h"
#include "common.h"
#include <libconfig.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static int parse_strs(config_setting_t *root, const char *strs_file)
{
	void *data = NULL;
	int ret = SUCCESS;
	FILE *fp;
	struct usb_functionfs_strings_head *header;

	if (config_setting_get_member(root, "strings") == NULL) {
		ret = ERROR_NOT_FOUND;
		goto out;
	}

	ret = ffs_parse_str_config(root, &data);
	if (ret < 0)
		goto out;

	fp = fopen(strs_file, "w");
	if (fp == NULL) {
		fprintf(stderr, "%s: %s", strs_file, strerror(errno));
		ret = ERROR_FILE_OPEN_FAILED;
		goto out;
	}

	fwrite(data, 1, ret, fp);
	printf("Strings written to %s\n", strs_file);

	fclose(fp);

	header = (struct usb_functionfs_strings_head *)data;
	ret = header->str_count;

out:
	free(data);
	return ret;
}

static int parse_desc(config_setting_t *root, const char *desc_file, int str_count)
{
	void *data = NULL;
	int ret = SUCCESS;
	FILE *fp;

	if (config_setting_get_member(root, "descriptors") == NULL) {
		ret = ERROR_NOT_FOUND;
		goto out;
	}

	ret = ffs_parse_desc_config(root, &data, str_count);
	if (ret < 0)
		goto out;

	fp = fopen(desc_file, "w");
	if (fp == NULL) {
		fprintf(stderr, "%s: %s\n", desc_file, strerror(errno));
		ret = ERROR_FILE_OPEN_FAILED;
		goto out;
	}

	fwrite(data, 1, ret, fp);
	printf("Descriptors written to %s\n", desc_file);

	fclose(fp);

out:
	free(data);
	return ret;
}

int parse_ffs_config(const char *input_file, const char *desc_file,
		     const char *strs_file)
{
	config_t cfg;
	config_setting_t *root;
	int ret = SUCCESS;
	void *data;

	config_init(&cfg);

	if (config_read_file(&cfg, input_file) == CONFIG_FALSE) {
		fprintf(stderr, "%s:%d - %s\n",
			input_file,
			config_error_line(&cfg),
			config_error_text(&cfg));
		ret = ERROR_OTHER_ERROR;
		goto out;
	}

	root = config_root_setting(&cfg);

	ret = parse_strs(root, strs_file);
	if (ret < 0)
		goto out;

	ret = parse_desc(root, desc_file, ret);

out:
	config_destroy(&cfg);
	return ret;
}
