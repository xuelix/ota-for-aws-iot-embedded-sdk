/*
 * FreeRTOS OTA V1.2.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file aws_iot_ota_interface.c
 * @brief .
 */

/* Standard library includes. */
#include <string.h>

/* OTA inteface includes. */
#include "aws_iot_ota_interface.h"

/* OTA transport inteface includes. */

#if ( configENABLED_DATA_PROTOCOLS & OTA_DATA_OVER_MQTT ) || ( configENABLED_CONTROL_PROTOCOL & OTA_CONTROL_OVER_MQTT )
    #include "mqtt/aws_iot_ota_mqtt.h"
#endif

#if ( configENABLED_DATA_PROTOCOLS & OTA_DATA_OVER_HTTP )
    #include "aws_iot_ota_http.h"
#endif

/* Check if primary protocol is enabled in aws_iot_ota_agent_config.h. */

#if !( configENABLED_DATA_PROTOCOLS & configOTA_PRIMARY_DATA_PROTOCOL )
    #error "Primary data protocol must be enabled in aws_iot_ota_agent_config.h"
#endif

/*
 * Primary data protocol will be the protocol used for file download if more
 * than one protocol is selected while creating OTA job.
 */
#if ( configOTA_PRIMARY_DATA_PROTOCOL == OTA_DATA_OVER_MQTT )
    static const char * pcProtocolPriority[ OTA_DATA_NUM_PROTOCOLS ] =
    {
        "MQTT",
        "HTTP"
    };
#elif ( configOTA_PRIMARY_DATA_PROTOCOL == OTA_DATA_OVER_HTTP )
    static const char * pcProtocolPriority[ OTA_DATA_NUM_PROTOCOLS ] =
    {
        "HTTP",
        "MQTT"
    };
#endif /* if ( configOTA_PRIMARY_DATA_PROTOCOL == OTA_DATA_OVER_MQTT ) */


void setControlInterface( OtaControlInterface_t * pxControlInterface )
{
    #if ( configENABLED_CONTROL_PROTOCOL == OTA_CONTROL_OVER_MQTT )
        pxControlInterface->requestJob = requestJob_Mqtt;
        pxControlInterface->updateJobStatus = prvUpdatJobStatusMqtt;
    #else
    #error "Enable MQTT control as control operations are only supported over MQTT."
    #endif
}

OtaErr_t setDataInterface( OtaDataInterface_t * pxDataInterface,
                           const uint8_t * pucProtocol )
{
    DEFINE_OTA_METHOD_NAME( "setDataInterface" );

    OtaErr_t xErr = OTA_ERR_INVALID_DATA_PROTOCOL;
    uint32_t i;

    for( i = 0; i < OTA_DATA_NUM_PROTOCOLS; i++ )
    {
        if( NULL != strstr( ( const char * ) pucProtocol, pcProtocolPriority[ i ] ) )
        {
            #if ( configENABLED_DATA_PROTOCOLS & OTA_DATA_OVER_MQTT )
                if( strcmp( pcProtocolPriority[ i ], "MQTT" ) == 0 )
                {
                    pxDataInterface->initFileTransfer = initFileTransfer_Mqtt;
                    pxDataInterface->requestFileBlock = requestFileBlock_Mqtt;
                    pxDataInterface->decodeFileBlock = decodeFileBlock_Mqtt;
                    pxDataInterface->cleanup = cleanup_Mqtt;

                    OTA_LOG_L1( "[%s] Data interface is set to MQTT.\r\n", OTA_METHOD_NAME );

                    xErr = OTA_ERR_NONE;
                    break;
                }
            #endif /* if ( configENABLED_DATA_PROTOCOLS & OTA_DATA_OVER_MQTT ) */

            #if ( configENABLED_DATA_PROTOCOLS & OTA_DATA_OVER_HTTP )
                if( strcmp( pcProtocolPriority[ i ], "HTTP" ) == 0 )
                {
                    pxDataInterface->initFileTransfer = _AwsIotOTA_InitFileTransfer_HTTP;
                    pxDataInterface->requestFileBlock = _AwsIotOTA_RequestDataBlock_HTTP;
                    pxDataInterface->decodeFileBlock = _AwsIotOTA_DecodeFileBlock_HTTP;
                    pxDataInterface->cleanup = _AwsIotOTA_Cleanup_HTTP;

                    OTA_LOG_L1( "[%s] Data interface is set to HTTP.\r\n", OTA_METHOD_NAME );

                    xErr = OTA_ERR_NONE;
                    break;
                }
            #endif /* if ( configENABLED_DATA_PROTOCOLS & OTA_DATA_OVER_HTTP ) */
        }
    }

    return xErr;
}