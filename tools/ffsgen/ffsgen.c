/*
 * ffsgen.c
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

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "parse.h"
#include "common.h"

static void usage()
{
	printf("Usage: ffsgen [options] file ...\n"
		"\n"
		"Options:\n"
		"  -d --descriptors-file <file>\tWrite descriptors into <file>\n"
		"  -s --strings-file <file>\tWrite strings into <file>\n"
		"  -f --descriptors-format <version> Select format of descriptors"
		"  (allow legacy format). Default is the newest one avaible.\n"
		"  --list-desc-formats\tShow list of available descriptor formats\n"
		"  -h --help\tPrint this help\n");
}

static void list_desc_formats()
{
	printf("v1\n");

#ifdef HAS_FFS_DESC_V2
	printf("v2\n");
#endif
}

int main(int argc, char **argv)
{
	int c;
	char *input_file = NULL, *desc_file = NULL, *strs_file = NULL;
	int ret = 1;

	while (1) {
		int option_index = 0;
		static struct option opts[] = {
			{"help", no_argument, 0, 'h'},
			{"descriptors-file", required_argument, 0, 'd'},
			{"strings-file", required_argument, 0, 's'},
			{"descriptors-format", required_argument, 0, 'f'},
			{"list-desc-formats", no_argument, 0, 1},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "hd:s:f:", opts, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			desc_file = optarg;
			break;
		case 's':
			strs_file = optarg;
			break;
		case 'f':
			desc_format = ffs_desc_format_from_str(optarg);
			if (desc_format >= FFS_DESC_FORMAT_END) {
				fprintf(stderr, "Format: %s not supported\n",
						optarg);
				goto out;
			}

			break;
		case 1:
			ret = 0;
			list_desc_formats();
			goto out;
			break;
		default:
			usage();
			ret = c == 'h' ? 0 : -EINVAL;
			goto out;
		}
	};

	if (optind >= argc) {
		fprintf(stderr, "Error: no input files\n");
		ret = 1;
		goto out;
	}

	input_file = argv[optind];

	if (desc_file == NULL)
		desc_file = "out.desc";

	if (strs_file == NULL)
		strs_file = "out.strs";

	ret = parse_ffs_config(input_file, desc_file, strs_file);

out:
	return ret;
}
