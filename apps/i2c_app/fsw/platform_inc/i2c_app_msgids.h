/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * Define I2c App Message IDs
 *
 * \note The I2c App assumes default configuration which uses V1 of message id implementation
 */

#ifndef I2C_APP_MSGIDS_H
#define I2C_APP_MSGIDS_H

/* V1 Command Message IDs must be 0x18xx */
#define I2C_APP_CMD_MID     0x1889
#define I2C_APP_SEND_HK_MID 0x1886
/* V1 Telemetry Message IDs must be 0x08xx */
#define I2C_APP_HK_TLM_MID 0x0887

#endif /* I2C_APP_MSGIDS_H */
