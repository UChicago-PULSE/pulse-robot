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
 * Main header file for the I2C application
 */

#ifndef I2C_APP_H
#define I2C_APP_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_config.h"
#include "cfe_es.h"
/*
#include "sample_app_mission_cfg.h"
#include "sample_app_platform_cfg.h"
*/
#include "i2c_app_perfids.h"
#include "i2c_app_msgids.h"
#include "i2c_app_msg.h"


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>


#define I2C_ADDRESS 0x14
#define I2C_APP_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define I2C_APP_NUMBER_OF_TABLES 1 /* Number of Table(s) */

/* Define filenames of default data images for tables */
#define I2C_APP_TABLE_FILE "/cf/sample_app_tbl.tbl"

#define I2C_APP_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define I2C_APP_TBL_ELEMENT_1_MAX 10
/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    I2C_APP_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    int i2c_fd;

    CFE_TBL_Handle_t TblHandles[I2C_APP_NUMBER_OF_TABLES];
} I2C_APP_Data_t;

typedef struct {
    int16_t left_speed, right_speed;
    int16_t left_dist, right_dist;
    bool r_led, g_led, y_led;
} I2C_Command_Packet;


typedef struct {
    int fd;
    int bus_num;
} Open_I2C;


typedef struct {
  int16_t l_enc, r_enc;
  int16_t rem_left;
  int16_t rem_right;
  int16_t cmd_left_dist;
  int16_t cmd_right_dist;
  int16_t cmd_left_speed;
  int16_t cmd_right_speed;
  int16_t set_left_speed;
  int16_t set_right_speed;
  uint16_t batteryMillivolts;
  bool button_A;
  bool button_B;
  bool button_C;
} I2C_Telem_Packet;




/*
** Global data structure
*/
/*
extern I2C_APP_Data_t I2C_APP_Data;
*/
/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (I2C_APP_Main), these
**       functions are not called from any other source module.
*/
void         I2C_APP_Main(void);
CFE_Status_t I2C_APP_Init(void);
CFE_Status_t I2C_OPEN_BUS(int bus_num, int* fd);
CFE_Status_t I2C_APP_Send(int fd, I2C_Command_Packet* packet);


void  I2C_APP_Main(void);
int32 I2C_APP_Init(void);
void  I2C_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void  I2C_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);
int32 I2C_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 I2C_APP_ResetCounters(const I2C_APP_ResetCountersCmd_t *Msg);
int32 I2C_APP_Process(const I2C_APP_ProcessCmd_t *Msg);
int32 I2C_APP_Noop(const I2C_APP_NoopCmd_t *Msg);
void  I2C_APP_GetCrc(const char *TableName);

int32 I2C_APP_TblValidationFunc(void *TblData);

bool I2C_APP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);
#endif /* I2C_APP_CMDS_H */
