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
 * \file
 *   This file contains the source code for the Sample App.
 */

/*
** Include Files:
*/
#include "i2c_app_events.h"
#include "i2c_app_version.h"
#include "i2c_app.h"
#include "i2c_app_table.h"


/* The sample_lib module provides the SAMPLE_LIB_Function() prototype */
#include <string.h>
#include <linux/i2c-dev.h>

/*
** global data
*/
I2C_APP_Data_t I2C_APP_Data;


CFE_Status_t I2C_OPEN_BUS(int bus_num, int* fd) {
    char filename[20];
    snprintf(filename, sizeof(filename), "/dev/i2c-%d", bus_num);
    *fd = open(filename, O_RDWR);
    if (*fd < 0) {
        perror("Opening I2C bus");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    } 
    OS_TaskDelay(100);
    if (ioctl(*fd, I2C_SLAVE, 14) < 0){
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    CFE_EVS_SendEvent(I2C_APP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "I2C BUS: %d", *fd);


    return CFE_SUCCESS;
}

CFE_Status_t I2C_APP_Send(int fd, I2C_Command_Packet* packet) {
    OS_TaskDelay(100);
    ssize_t res = write(fd, (const void*) packet, sizeof(I2C_Command_Packet));
    if (res == -1) {
        perror("Error sending app");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    fsync(fd);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void I2C_APP_Main(void)
{
    int32            status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(I2C_APP_PERF_ID);

    /*
    ** Perform application specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = I2C_APP_Init();
    if (status != CFE_SUCCESS)
    {
        I2C_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** I2C Runloop
    */
    while (CFE_ES_RunLoop(&I2C_APP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(I2C_APP_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, I2C_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(I2C_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            I2C_APP_ProcessCommandPacket(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(I2C_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "I2C APP: SB Pipe Read Error, App Will Exit");

            I2C_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(I2C_APP_PERF_ID);

    CFE_ES_ExitApp(I2C_APP_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 I2C_APP_Init(void)
{
    int32 status;

    I2C_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app command execution counters
    */
    I2C_APP_Data.CmdCounter = 0;
    I2C_APP_Data.ErrCounter = 0;

    /*
    ** Initialize app configuration data
    */
    I2C_APP_Data.PipeDepth = I2C_APP_PIPE_DEPTH;

    strncpy(I2C_APP_Data.PipeName, "I2C_APP_CMD_PIPE", sizeof(I2C_APP_Data.PipeName));
    I2C_APP_Data.PipeName[sizeof(I2C_APP_Data.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Initialize housekeeping packet (clear user data area).
    */
    CFE_MSG_Init(CFE_MSG_PTR(I2C_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(I2C_APP_HK_TLM_MID),
                 sizeof(I2C_APP_Data.HkTlm));

    /*
    ** Create Software Bus message pipe.
    */
    status = CFE_SB_CreatePipe(&I2C_APP_Data.CommandPipe, I2C_APP_Data.PipeDepth, I2C_APP_Data.PipeName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Error creating pipe, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to Housekeeping request commands
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(I2C_APP_SEND_HK_MID), I2C_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Error Subscribing to HK request, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to ground command packets
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(I2C_APP_CMD_MID), I2C_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return status;
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Open the I2C and set the corresponding file descriptor
        */
        status = I2C_OPEN_BUS(1, &I2C_APP_Data.i2c_fd);
        if (status == -1) {
            CFE_EVS_SendEvent(I2C_APP_STARTUP_INF_EID, CFE_EVS_EventType_ERROR,
                "I2C App: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        CFE_EVS_SendEvent(I2C_APP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "I2C Connection Established");
    }


    /*
    ** Register Table(s)
    */
    status = CFE_TBL_Register(&I2C_APP_Data.TblHandles[0], "I2cAppTable", sizeof(I2C_APP_Table_t),
                              CFE_TBL_OPT_DEFAULT, I2C_APP_TblValidationFunc);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Error Registering Table, RC = 0x%08lX\n", (unsigned long)status);

        return status;
    }
    else
    {
        status = CFE_TBL_Load(I2C_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, I2C_APP_TABLE_FILE);
    }

    CFE_EVS_SendEvent(I2C_APP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "I2C App Initialized.%s",
                      I2C_APP_VERSION_STRING);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the I2C    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void I2C_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case I2C_APP_CMD_MID:
            I2C_APP_ProcessGroundCommand(SBBufPtr);
            break;

        case I2C_APP_SEND_HK_MID:
            I2C_APP_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(I2C_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "I2C: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* I2C ground commands                                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void I2C_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process "known" I2C app ground commands
    */
    switch (CommandCode)
    {
        case I2C_APP_NOOP_CC:
            if (I2C_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(I2C_APP_NoopCmd_t)))
            {
                I2C_APP_Noop((I2C_APP_NoopCmd_t *)SBBufPtr);
            }

            break;

        case I2C_APP_RESET_COUNTERS_CC:
            if (I2C_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(I2C_APP_ResetCountersCmd_t)))
            {
                I2C_APP_ResetCounters((I2C_APP_ResetCountersCmd_t *)SBBufPtr);
            }

            break;

        case I2C_APP_PROCESS_CC:
            if (I2C_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(I2C_APP_ProcessCmd_t)))
            {
                I2C_APP_Process((I2C_APP_ProcessCmd_t *)SBBufPtr);
            }

            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(I2C_APP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d", CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 I2C_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{
    int i;

    /*
    ** Get command execution counters...
    */
    I2C_APP_Data.HkTlm.Payload.CommandErrorCounter = I2C_APP_Data.ErrCounter;
    I2C_APP_Data.HkTlm.Payload.CommandCounter      = I2C_APP_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(I2C_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(I2C_APP_Data.HkTlm.TelemetryHeader), true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < I2C_APP_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(I2C_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* I2C NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 I2C_APP_Noop(const I2C_APP_NoopCmd_t *Msg)
{
    I2C_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(I2C_APP_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "I2C: NOOP command %s",
                      I2C_APP_VERSION);

        I2C_Command_Packet packet;
        packet.right_speed = 100;
        packet.left_speed = 100;

        int res = I2C_APP_Send(I2C_APP_Data.i2c_fd, &packet);
        if (res == -1) {
            printf("I2C Error");
            exit(1);
        }

        return CFE_SUCCESS;
                  
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 I2C_APP_ResetCounters(const I2C_APP_ResetCountersCmd_t *Msg)
{
    I2C_APP_Data.CmdCounter = 0;
    I2C_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(I2C_APP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "I2C: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function Process Ground Station Command                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 I2C_APP_Process(const I2C_APP_ProcessCmd_t *Msg)
{
    int32               status;
    I2C_APP_Table_t *TblPtr;
    const char *        TableName = "I2C_APP.I2cAppTable";

    /* I2c Use of Table */

    status = CFE_TBL_GetAddress((void *)&TblPtr, I2C_APP_Data.TblHandles[0]);

    if (status < CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Fail to get table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    CFE_ES_WriteToSysLog("I2C App: Table Value 1: %d  Value 2: %d", TblPtr->Int1, TblPtr->Int2);

    I2C_APP_GetCrc(TableName);

    status = CFE_TBL_ReleaseAddress(I2C_APP_Data.TblHandles[0]);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2C App: Fail to release table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    /* Invoke a function */
    //I2C_LIB_Function();

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool I2C_APP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(I2C_APP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        I2C_APP_Data.ErrCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Verify contents of First Table buffer contents                  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 I2C_APP_TblValidationFunc(void *TblData)
{
    int32               ReturnCode = CFE_SUCCESS;
    I2C_APP_Table_t *TblDataPtr = (I2C_APP_Table_t *)TblData;

    /*
    ** I2c Table Validation
    */
    if (TblDataPtr->Int1 > I2C_APP_TBL_ELEMENT_1_MAX)
    {
        /* First element is out of range, return an appropriate error code */
        ReturnCode = I2C_APP_TABLE_OUT_OF_RANGE_ERR_CODE;
    }

    return ReturnCode;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Output CRC                                                      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void I2C_APP_GetCrc(const char *TableName)
{
    int32          status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("I2c App: Error Getting Table Info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("I2c App: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }
}
