/*
 * desc-parse.c
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

#include "desc-parse.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <ctype.h>
#include <linux/usb/functionfs.h>
#include <errno.h>

int desc_to_binary(void **dst, struct ffs_desc_per_speed *desc, int mask)
{
	union {
		struct usb_functionfs_descs_head v1;
#ifdef HAS_FFS_DESC_V2
		struct usb_functionfs_descs_head_v2 v2;
#endif
	} *header;

	int ret = 0, i = 0, j = 0, size;
	char *pos;

	switch (desc_format) {
	case FFS_DESC_FORMAT_V1:
		size = sizeof(header->v1);
		break;
#ifdef HAS_FFS_DESC_V2
	case FFS_DESC_FORMAT_V2:
		size = sizeof(header->v2);
		break;
#endif
	default:
		ret = -ENOTSUP;
		goto out;
	}

	for (i = FFS_USB_FULL_SPEED; i < FFS_USB_TERMINATOR; i = i << 1)
		if (i & mask)
			size += desc[j++].desc_size;

	/* For *_count fields in descriptor */
	if (desc_format != FFS_DESC_FORMAT_V1)
		size += j*sizeof(__le32);

	pos = malloc(size);
	if (!pos) {
		ret = -ENOMEM;
		goto out;
	}
	*dst = pos;
	ret = size;

	/* Fill header of functionfs descriptors */
	header = *dst;
	j = 0;

	switch (desc_format) {
	case FFS_DESC_FORMAT_V1:
		header->v1.length = htole32(size);
		header->v1.magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC);
		header->v1.fs_count = mask & FFS_USB_FULL_SPEED ?
			htole32(desc[j++].desc_count) : 0;
		header->v1.hs_count = mask & FFS_USB_HIGH_SPEED ?
			htole32(desc[j++].desc_count) : 0;
		pos += sizeof(header->v1);
		break;
#ifdef HAS_FFS_DESC_V2
	case FFS_DESC_FORMAT_V2:
		header->v2.magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC_V2);
		header->v2.length = htole32(size);
		header->v2.flags = 0;
		for (i = FFS_USB_FULL_SPEED; i != FFS_USB_TERMINATOR; i = i << 1) {
			if (!(i & mask))
				continue;
			header->v2.flags |= i;
			*(__le32*)(pos + sizeof(header->v2) + sizeof(__le32)*j) =
				htole32(desc[j].desc_count);
			++j;
		}
		pos += sizeof(header->v2) + sizeof(__le32)*j;
		break;
#endif
	default:
		/* We already checked for usupported format */
		break;
	}

	/* Fill endpoint descriptors */
	j = 0;
	for (i = FFS_USB_FULL_SPEED; i < FFS_USB_TERMINATOR; i = i << 1)
		if (i & mask) {
			memcpy(pos, desc[j].desc, desc[j].desc_size);
			pos += desc[j].desc_size;
			++j;
		}

out:
	return ret;
}

static const char *string_skip_spaces(const char *ptr)
{
	while(isspace(*ptr) && *ptr != '\0')
		ptr++;
	return ptr;
}

static const char *string_skip_const_name(const char *ptr)
{
	while(!isspace(*ptr) && *ptr != '|' && *ptr != '\0')
		ptr++;
	return ptr;
}

static int ffs_parse_flags(const char *str, int *res,
		int (*translate)(const char *, int, int *))
{
	int tmp;
	int flag;
	const char *start;
	const char *pos;

	*res = 0;
	pos = str;
	start = pos;
	while (1) {
		pos = string_skip_spaces(pos);
		if (*pos == '\0')
			break;

		start = pos;
		pos = string_skip_const_name(pos);
		tmp = translate(start, pos - start, &flag);
		if (tmp < 0)
			return tmp;
		*res |= flag;

		pos = string_skip_spaces(pos);
		if (*pos == '\0')
			break;

		if (*pos != '|')
			return ERROR_BAD_VALUE;
		pos++;
	}

	return SUCCESS;
}

static int ffs_desc_translate_attribute(const char *name, int len, int *att)
{
	int res = -1;
	int tmp;
	static struct gd_named_const keys[] = {
		DECLARE_ELEMENT(USB_ENDPOINT_XFER_ISOC),
		DECLARE_ELEMENT(USB_ENDPOINT_XFER_BULK),
		DECLARE_ELEMENT(USB_ENDPOINT_XFER_INT),
		DECLARE_ELEMENT(USB_ENDPOINT_MAX_ADJUSTABLE),
		DECLARE_END()
	};

	if (name == NULL)
		return ERROR_INVALID_PARAM;

	tmp = get_const_value(name, len, keys, &res);
	if (tmp < 0) {
		fprintf(stderr, "Unknown attribute flag: %.*s\n", len, name);
		return ERROR_BAD_VALUE;
	}

	*att = res;

	return SUCCESS;
}

static int ffs_lookup_desc_attributes(config_setting_t *root, __u8 *att)
{
	config_setting_t *node;
	int res;
	const char *buff;
	int tmp;

	node = config_setting_get_member(root, "bmAttributes");
	if (node == NULL) {
		CONFIG_ERROR(root, "bmAttributes not defined");
		return ERROR_NOT_FOUND;
	}

	if (config_setting_type(node) == CONFIG_TYPE_INT) {
		res = config_setting_get_int(node);
	} else if (config_setting_type(node) == CONFIG_TYPE_STRING) {
		buff = config_setting_get_string(node);
		tmp = ffs_parse_flags(buff, &res, ffs_desc_translate_attribute);
		if (tmp < 0) {
			CONFIG_ERROR(node, "error parsing attributes");
			return tmp;
		}

		if (res < 0 || res > UCHAR_MAX) {
			CONFIG_ERROR(node, "out of range");
			return ERROR_INVALID_PARAM;
		}

		*att = (__u8) res;
	} else {
		CONFIG_ERROR(node, "must be string or number");
	}

	return SUCCESS;
}

static int ffs_parse_ep_desc_no_audio(config_setting_t *root,
	struct usb_endpoint_descriptor_no_audio *desc)
{
	int res;
	int tmp;
	const char *buff;
	char dir_in[] = "in";
	char dir_out[] = "out";
	config_setting_t *node;

	desc->bLength = sizeof(*desc);
	desc->bDescriptorType = USB_DT_ENDPOINT;
	node = config_setting_get_member(root, "address");
	if (node == NULL) {
		CONFIG_ERROR(root, "address not defined");
		return ERROR_NOT_FOUND;
	}

	tmp = ffs_setting_get_int(node, &res);
	if (tmp < 0)
		return tmp;

	if (res > UCHAR_MAX || res < 0) {
		CONFIG_ERROR(node, "adress out of range");
		return ERROR_BAD_VALUE;
	}

	/* only part of address, direction bit is set later */
	desc->bEndpointAddress = res;
	tmp = ffs_lookup_desc_attributes(root, &desc->bmAttributes);
	if (tmp < 0)
		return tmp;

	node = config_setting_get_member(root, "direction");
	if (node == NULL) {
		CONFIG_ERROR(root, "direction not defined");
		return ERROR_NOT_FOUND;
	}

	tmp = ffs_setting_get_string(node, &buff);
	if (tmp < 0)
		return tmp;

	if (strcmp(buff, dir_in) == 0) {
		desc->bEndpointAddress |= USB_DIR_IN;
	} else if (strcmp(buff, dir_out) == 0) {
		desc->bEndpointAddress |= USB_DIR_OUT;
	} else {
		CONFIG_ERROR(node, "Invalid direction value");
		return ERROR_BAD_VALUE;
	}

	return SUCCESS;
}

static int ffs_lookup_desc_interface_class(config_setting_t *root, __u8 *cl)
{
	const char *buff;
	int buflen;
	config_setting_t *node;
	int res;
	int tmp;

	node = config_setting_get_member(root, "bInterfaceClass");
	if (node == NULL) {
		CONFIG_ERROR(root, "Interface class not defined");
		return ERROR_NOT_FOUND;
	}

	if (config_setting_type(node) == CONFIG_TYPE_INT) {
		res = config_setting_get_int(node);
	} else if (config_setting_type(node) == CONFIG_TYPE_STRING) {
		buff = config_setting_get_string(node);
		buflen = strlen(buff);
		res = -1;

		static struct gd_named_const keys[] = {
			DECLARE_ELEMENT(USB_CLASS_PER_INTERFACE),
			DECLARE_ELEMENT(USB_CLASS_AUDIO),
			DECLARE_ELEMENT(USB_CLASS_COMM),
			DECLARE_ELEMENT(USB_CLASS_HID),
			DECLARE_ELEMENT(USB_CLASS_PHYSICAL),
			DECLARE_ELEMENT(USB_CLASS_STILL_IMAGE),
			DECLARE_ELEMENT(USB_CLASS_PRINTER),
			DECLARE_ELEMENT(USB_CLASS_MASS_STORAGE),
			DECLARE_ELEMENT(USB_CLASS_HUB),
			DECLARE_ELEMENT(USB_CLASS_CDC_DATA),
			DECLARE_ELEMENT(USB_CLASS_CSCID),
			DECLARE_ELEMENT(USB_CLASS_CONTENT_SEC),
			DECLARE_ELEMENT(USB_CLASS_VIDEO),
			DECLARE_ELEMENT(USB_CLASS_WIRELESS_CONTROLLER),
			DECLARE_ELEMENT(USB_CLASS_MISC),
			DECLARE_ELEMENT(USB_CLASS_APP_SPEC),
			DECLARE_ELEMENT(USB_CLASS_VENDOR_SPEC),
			DECLARE_END()
		};

		tmp = get_const_value(buff, buflen, keys, &res);
		if (tmp < 0) {
			CONFIG_ERROR(node, "Unknown interface class %s", buff);
			return ERROR_BAD_VALUE;
		}
	} else {
		CONFIG_ERROR(node, "expected string or number");
		return ERROR_BAD_VALUE;
	}

	if (res > UCHAR_MAX || res < 0) {
		CONFIG_ERROR(node, "out of range");
		return ERROR_BAD_VALUE;
	}
	*cl = res;

	return SUCCESS;
}

static int ffs_lookup_desc_interface_subclass(config_setting_t *root, __u8 *cl)
{
	int res;
	config_setting_t *node;

	node = config_setting_get_member(root, "bInterfaceSubClass");
	if (node == NULL) {
		CONFIG_ERROR(root, "Interface subclass not defined");
		return ERROR_NOT_FOUND;
	}

	if (config_setting_type(node) == CONFIG_TYPE_INT) {
		res = config_setting_get_int(node);
	} else if (config_setting_type(node) == CONFIG_TYPE_STRING) {
		/* TODO string-defined subclasses */
		CONFIG_ERROR(node, "strings not supported in subclass");
		return ERROR_NOT_SUPPORTED;
	} else {
		CONFIG_ERROR(node, "Interface subclass must be string or number");
		return ERROR_BAD_VALUE;
	}

	if (res > UCHAR_MAX || res < 0) {
		CONFIG_ERROR(node, "out of range");
		return ERROR_BAD_VALUE;
	}
	*cl = res;

	return SUCCESS;
}

static int ffs_parse_interface_desc(config_setting_t *root,
	struct usb_interface_descriptor *desc, int str_count)
{
	int tmp;
	int res;
	config_setting_t *node;

	desc->bLength = sizeof(*desc);
	desc->bDescriptorType = USB_DT_INTERFACE;
	tmp = ffs_lookup_desc_interface_class(root, &desc->bInterfaceClass);
	if (tmp < 0)
		return tmp;

	tmp = ffs_lookup_desc_interface_subclass(root, &desc->bInterfaceSubClass);
	if (tmp < 0 && tmp != ERROR_NOT_FOUND)
		return tmp;

	else if (tmp == ERROR_NOT_FOUND)
		desc->bInterfaceSubClass = 0;

	node = config_setting_get_member(root, "iInterface");
	if (node == NULL) {
		CONFIG_ERROR(root, "iInterface not defined");
		return ERROR_NOT_FOUND;
	}

	tmp = ffs_setting_get_int(node, &res);
	if (tmp < 0)
		return tmp;

	if (res > UCHAR_MAX || res < 0) {
		CONFIG_ERROR(node, "out of range");
		return ERROR_BAD_VALUE;
	}

	if (res > str_count) {
		CONFIG_ERROR(node, "String of this index does not exist");
		return ERROR_BAD_VALUE;
	}

	desc->iInterface = res;

	return SUCCESS;
}

/**
 * @brief Read list of ffs descriptors into proper structure
 * @param[in] list Libconfig list containing definitions of descriptors
 * @param[out] desc Structure containing descriptors
 */
static int ffs_fill_desc_list(config_setting_t *list, struct ffs_desc_per_speed *desc, int str_count)
{
	int len;
	char *pos;
	int i, j;
	int tmp;
	const char *buff;
	config_setting_t *group;
	struct usb_interface_descriptor *inter = NULL;
	struct usb_endpoint_descriptor_no_audio *ep;

	len = config_setting_length(list);

	desc->desc_count = len;
	desc->desc_size = 0;
	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);

		if (config_setting_is_group(group) == CONFIG_FALSE) {
			CONFIG_ERROR(group, "expected group")
			return ERROR_BAD_VALUE;
		}

		tmp = config_setting_lookup_string(group, "type", &buff);
		if (tmp == CONFIG_FALSE) {
			CONFIG_ERROR(group, "descriptor type not defined");
			return ERROR_NOT_FOUND;
		}

		if (strcmp(buff, "INTERFACE_DESC") == 0)
			desc->desc_size += sizeof(*inter);
		else if (strcmp(buff, "EP_NO_AUDIO_DESC") == 0)
			desc->desc_size += sizeof(*ep);
		else {
			CONFIG_ERROR(group, "%s descriptor type unsupported", buff);
			return ERROR_NOT_SUPPORTED;
		}
	}

	desc->desc = calloc(desc->desc_size, sizeof(char));
	if (desc->desc == NULL) {
		fprintf(stderr, "Error allocating memory\n");
		return ERROR_NO_MEM;
	}

	pos = desc->desc;
	j = 0;
	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);
		tmp = config_setting_lookup_string(group, "type", &buff);
		if (tmp == CONFIG_FALSE) {
			CONFIG_ERROR(group, "type not defined");
			tmp = ERROR_NOT_FOUND;
			goto out;
		}

		if (strcmp(buff, "INTERFACE_DESC") == 0) {
			inter = (struct usb_interface_descriptor *)pos;
			tmp = ffs_parse_interface_desc(group, inter, str_count);
			if (tmp < 0)
				goto out;
			inter->bNumEndpoints = 0;
			inter->bInterfaceNumber = j++;
			pos += sizeof(*inter);
		} else if (strcmp(buff, "EP_NO_AUDIO_DESC") == 0) {
			if (inter == NULL) {
				CONFIG_ERROR(group,
				"endpoint descriptor defined before any interface descriptor");
				tmp = ERROR_OTHER_ERROR;
				goto out;
			}
			ep = (struct usb_endpoint_descriptor_no_audio *)pos;
			tmp = ffs_parse_ep_desc_no_audio(group, ep);
			if (tmp < 0)
				goto out;

			inter->bNumEndpoints++;
			pos += sizeof(*ep);
		}
	}

	return SUCCESS;
out:
	free(desc->desc);
	return tmp;
}

static const char *desc_speed_strs[] = {
	[FFS_USB_FULL_SPEED] = "fs_desc",
	[FFS_USB_HIGH_SPEED] = "hs_desc",
#ifdef HAS_FFS_DESC_V2
	[FFS_USB_SUPER_SPEED] = "ss_desc"
#endif
};

int ffs_parse_desc_config(config_setting_t *root, void **data, int str_count)
{
	config_setting_t *node;
	config_setting_t *group;
	struct ffs_desc_per_speed *desc;
	int speeds = 0;
	int tmp;
	int mask = 0;
	int ret;
	int i, j = 0;

	for (i = 1; i < FFS_USB_TERMINATOR; i = i << 1)
		++speeds;

	desc = calloc(speeds, sizeof(*desc));
	if (desc == NULL) {
		CONFIG_ERROR(root, "error allocating memory");
		return ERROR_NO_MEM;
	}

	group = config_setting_get_member(root, "descriptors");
	if (group == NULL) {
		CONFIG_ERROR(root, "descriptors not defined");
		return ERROR_NOT_FOUND;
	}

	if (config_setting_is_group(group) == CONFIG_FALSE) {
		CONFIG_ERROR(group, "descriptors must be group");
		return ERROR_BAD_VALUE;
	}

	for (i = 1; i < FFS_USB_TERMINATOR; i = i << 1) {
		node = config_setting_get_member(group, desc_speed_strs[i]);
		if (node != NULL) {
			if (config_setting_is_list(node) == CONFIG_FALSE) {
				CONFIG_ERROR(node, "expected list");
				return ERROR_BAD_VALUE;
			}

			tmp = ffs_fill_desc_list(node, &desc[j++], str_count);
			if (tmp < 0)
				return tmp;

			mask |= i;
		}
	}

	if (!mask) {
		CONFIG_ERROR(group, "no descriptors defined");
		return ERROR_OTHER_ERROR;
	}

	ret = desc_to_binary(data, desc, mask);
	if (ret < 0)
		CONFIG_ERROR(group, "error in descriptors serialization");

	j = 0;
	for (i = 1; i < FFS_USB_TERMINATOR; i = i << 1)
		if (mask & i)
			free(desc[j++].desc);

	free(desc);

	return ret;
}
