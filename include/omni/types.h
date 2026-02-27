/**
  * @file    types.h
  * @author  LuckkMaker
  * @brief   Type definitions and structures
  * @attention
  *
  * Copyright (c) 2025 LuckkMaker
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
#ifndef OMNI_INCLUDE_TYPES_H
#define OMNI_INCLUDE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  OMNI status
 */
#define OMNI_OK                 0
#define OMNI_BUSY               1
#define OMNI_TIMEOUT            2
#define OMNI_FAIL               -1

/**
 * @brief Driver event callback function
 */
typedef void (*driver_event_callback_t)(uint32_t event);

/**
 * @brief Driver delay callback
 */
typedef void (*driver_delay_callback_t)(uint32_t delay);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMNI_INCLUDE_TYPES_H */
