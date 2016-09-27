/*
 * parse.h
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

#ifndef PARSE_H
#define PARSE_H

/**
 * @brief Generate binary filles from ffs function definition
 * @details Parse configuration file containing definition of function
 * 	descriptors and strings, and write them in binary format to given files.
 * @param[in] input_file Path to ffs configuration file
 * @param[out] desc_file Path to output file where descriptors will be written
 * @param[out] strs_file Path to output file where strings will be written
 * @return 0 on success, error code otherwise
 */
int parse_ffs_config(const char *input_file, const char *desc_file,
		     const char *strs_file);

#endif /* PARSE_H */
