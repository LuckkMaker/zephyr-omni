/**
 * @file    util.h
 * @author  LuckkMaker
 * @brief   Useful utilities for omni
 * @attention
 *
 * Copyright (c) 2024-2025 LuckkMaker
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef OMNI_INCLUDE_UTIL_H
#define OMNI_INCLUDE_UTIL_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNUSED
#define UNUSED(X) (void)X
#endif /* UNUSED */

#ifndef ARRAY_SIZE
/**
 * @brief Get the number of elements in an array
 *
 * @param array Array to get the number of elements
 * @return Number of elements in the array
 */
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif /* ARRAY_SIZE */

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef LO_BYTE
#define LO_BYTE(x) ((uint8_t)(x & 0x00FF))
#endif /* LO_BYTE */

#ifndef HI_BYTE
#define HI_BYTE(x) ((uint8_t)((x & 0xFF00) >> 8))
#endif /* HI_BYTE */

#ifndef MB
#define MB(size) ((size) * 1024 * 1024)
#endif /* MB */

#ifndef KB
#define KB(size) ((size) * 1024)
#endif /* KB */

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif /* BIT */

#ifndef WBVAL
#define WBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF)
#endif /* WBVAL */

#ifndef HEX2CHAR
/**
 * @brief Convert a hex character to ascii character
 *
 * @param n Hex number (0-15)
 * @return Ascii character ('0'-'9', 'a'-'f')
 * @note 0-9 -> '0'-'9'
 *       10-15 -> 'a'-'f'
 */
#define HEX2CHAR(n) ((n) > 9 ? ((n) + 87) : ((n) + 48))
#endif /* HEX2CHAR */

#ifndef BYTE_TO_WORD
/**
 * @brief Convert a byte to a word, rounding up to the nearest word size
 *
 * @param bytes Number of bytes
 * @return Number of words (4 bytes each)
 */
#define BYTE_TO_WORD(bytes) (((bytes) + 3U) / 4U)
#endif /* BYTE_TO_WORD */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMNI_INCLUDE_UTIL_H */
