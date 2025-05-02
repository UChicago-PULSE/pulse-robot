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
 * Define I2C App  Messages and info
 */

#ifndef I2C_APP_MSG_H
#define I2C_APP_MSG_H

/*
** I2C App command codes
*/
#define I2C_APP_NOOP_CC           0
#define I2C_APP_RESET_COUNTERS_CC 1
#define I2C_APP_PROCESS_CC        2

/*************************************************************************/

/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader; /**< \brief Command header */
} I2C_APP_NoArgsCmd_t;

/*
** The following commands all share the "NoArgs" format
**
** They are each given their own type name matching the command name, which
** allows them to change independently in the future without changing the prototype
** of the handler function
*/
typedef I2C_APP_NoArgsCmd_t I2C_APP_NoopCmd_t;
typedef I2C_APP_NoArgsCmd_t I2C_APP_ResetCountersCmd_t;
typedef I2C_APP_NoArgsCmd_t I2C_APP_ProcessCmd_t;

/*************************************************************************/
/*
** Type definition (I2C App housekeeping)
*/

typedef struct
{
    uint8 CommandErrorCounter;
    uint8 CommandCounter;
    uint8 spare[2];
} I2C_APP_HkTlm_Payload_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    I2C_APP_HkTlm_Payload_t Payload;         /**< \brief Telemetry payload */
} I2C_APP_HkTlm_t;

#endif /* I2C_APP_MSG_H */
