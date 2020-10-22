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

/* Standard includes. */
#include <stdlib.h>
#include <stdbool.h>

/* CBOR and OTA includes. */
#include "aws_ota_agent_config.h"
#include "aws_iot_ota_agent.h"
#include "aws_iot_ota_agent_private.h"
#include "aws_iot_ota_cbor_private.h"
#include "cbor.h"

/* Unity framework includes. */
#include "unity_fixture.h"
#include "unity.h"

#define CBOR_TEST_MESSAGE_BUFFER_SIZE                     ( OTA_FILE_BLOCK_SIZE * 2 )
#define CBOR_TEST_SERVER_CHUNK_COUNT                      16
#define CBOR_TEST_BITMAP_VALUE                            0xAAAAAAAA
#define CBOR_TEST_GETSTREAMRESPONSE_MESSAGE_ITEM_COUNT    4
#define CBOR_TEST_CLIENTTOKEN_VALUE                       "ThisIsAClientToken"
#define CBOR_TEST_STREAMVERSION_VALUE                     2
#define CBOR_TEST_STREAMDESCRIPTION_VALUE                 "ThisIsAStream"
#define CBOR_TEST_FILEIDENTITY_VALUE                      2
#define CBOR_TEST_BLOCKIDENTITY_VALUE                     0
#define CBOR_TEST_STREAMFILES_COUNT                       3
#define CBOR_TEST_STREAMFILE_FIELD_COUNT                  2

/* ========================================================================== */

static CborError createSampleGetStreamResponseMessage( uint8_t * pMessageBuffer,
                                                       size_t messageBufferSize,
                                                       int blockIndex,
                                                       uint8_t * pBlockPayload,
                                                       size_t blockPayloadSize,
                                                       size_t * pEncodedSize )
{
    CborError cborResult = CborNoError;
    CborEncoder cborEncoder, cborMapEncoder;

    /* Initialize the CBOR encoder. */
    cbor_encoder_init(
        &cborEncoder,
        pMessageBuffer,
        messageBufferSize,
        0 );
    cborResult = cbor_encoder_create_map(
        &cborEncoder,
        &cborMapEncoder,
        CBOR_TEST_GETSTREAMRESPONSE_MESSAGE_ITEM_COUNT );

    /* Encode the file identity. */
    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_text_stringz(
            &cborMapEncoder,
            OTA_CBOR_FILEID_KEY );
    }

    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_int(
            &cborMapEncoder,
            CBOR_TEST_FILEIDENTITY_VALUE );
    }

    /* Encode the block identity. */
    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_text_stringz(
            &cborMapEncoder,
            OTA_CBOR_BLOCKID_KEY );
    }

    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_int(
            &cborMapEncoder,
            blockIndex );
    }

    /* Encode the block size. */
    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_text_stringz(
            &cborMapEncoder,
            OTA_CBOR_BLOCKSIZE_KEY );
    }

    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_int(
            &cborMapEncoder,
            blockPayloadSize );
    }

    /* Encode the block payload. */
    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_text_stringz(
            &cborMapEncoder,
            OTA_CBOR_BLOCKPAYLOAD_KEY );
    }

    if( CborNoError == cborResult )
    {
        cborResult = cbor_encode_byte_string(
            &cborMapEncoder,
            pBlockPayload,
            blockPayloadSize );
    }

    /* Done with the encoder. */
    if( CborNoError == cborResult )
    {
        cborResult = cbor_encoder_close_container_checked(
            &cborEncoder,
            &cborMapEncoder );
    }

    /* Get the encoded size. */
    if( CborNoError == cborResult )
    {
        *pEncodedSize = cbor_encoder_get_buffer_size(
            &cborEncoder,
            pMessageBuffer );
    }

    return cborResult;
}

void test_OTA_CborEncodeStreamRequest()
{
    uint8_t cborWork[ CBOR_TEST_MESSAGE_BUFFER_SIZE ];
    size_t encodedSize = 0;
    uint32_t bitmap = CBOR_TEST_BITMAP_VALUE;

    /* CBOR representation of a json get stream request message,
     * {"c": "ThisIsAClientToken", "f": 1, "l": 4096, "o": 0, "b": b"\xaa\xaa\xaa\xaa", "n": 1} */
    uint8_t expectedData[] =
    {
        0xa6, 0x61, 0x63, 0x72, 0x54, 0x68, 0x69, 0x73, 0x49, 0x73, 0x41, 0x43, 0x6c, 0x69, 0x65,
        0x6e, 0x74, 0x54, 0x6f, 0x6b, 0x65, 0x6e, 0x61, 0x66, 0x1,  0x61, 0x6c, 0x19, 0x10, 0x0,
        0x61, 0x6f, 0x0,  0x61, 0x62, 0x44, 0xaa, 0xaa, 0xaa, 0xaa, 0x61, 0x6e, 0x1
    };

    bool result = OTA_CBOR_Encode_GetStreamRequestMessage(
        cborWork,                          /* output message buffer. */
        sizeof( cborWork ),                /* size of output message buffer. */
        &encodedSize,                      /* size of encoded message. */
        CBOR_TEST_CLIENTTOKEN_VALUE,       /* client token. */
        1,                                 /* file id. */
        OTA_FILE_BLOCK_SIZE,               /* block size. */
        0,                                 /* block offset. */
        ( uint8_t * ) &bitmap,             /* block bitmap. */
        sizeof( bitmap ),                  /* size of bitmap. */
        otaconfigMAX_NUM_BLOCKS_REQUEST ); /* number of block requested. */

    TEST_ASSERT_TRUE( result );
    TEST_ASSERT_EQUAL( sizeof( expectedData ), encodedSize );

    int i;

    for( i = 0; i < sizeof( expectedData ); ++i )
    {
        TEST_ASSERT_EQUAL( expectedData[ i ], cborWork[ i ] );
    }
}

void test_OTA_CborDecodeStreamResponse()
{
    uint8_t blockPayload[ OTA_FILE_BLOCK_SIZE ] = { 0 };
    uint8_t cborWork[ CBOR_TEST_MESSAGE_BUFFER_SIZE ] = { 0 };
    size_t encodedSize = 0;
    int fileId = 0;
    int fileSize = 0;
    int blockIndex = 0;
    int blockSize = 0;
    uint8_t * pPayload = NULL;
    size_t payloadSize = 0;
    bool result = false;
    int i = 0;

    /* Test OTA_CBOR_Decode_GetStreamResponseMessage( ). */
    for( i = 0; i < sizeof( blockPayload ); i++ )
    {
        blockPayload[ i ] = i;
    }

    result = createSampleGetStreamResponseMessage(
        cborWork,
        sizeof( cborWork ),
        CBOR_TEST_BLOCKIDENTITY_VALUE,
        blockPayload,
        sizeof( blockPayload ),
        &encodedSize );
    TEST_ASSERT_EQUAL( CborNoError, result );

    if( TEST_PROTECT() )
    {
        result = OTA_CBOR_Decode_GetStreamResponseMessage(
            cborWork,
            encodedSize,
            &fileId,
            &blockIndex,
            &blockSize,
            &pPayload,
            &payloadSize );
        TEST_ASSERT_TRUE( result );
        TEST_ASSERT_EQUAL( CBOR_TEST_FILEIDENTITY_VALUE, fileId );
        TEST_ASSERT_EQUAL( CBOR_TEST_BLOCKIDENTITY_VALUE, blockIndex );
        TEST_ASSERT_EQUAL( OTA_FILE_BLOCK_SIZE, blockSize );
        TEST_ASSERT_EQUAL( OTA_FILE_BLOCK_SIZE, payloadSize );

        for( i = 0; i < sizeof( blockPayload ); i++ )
        {
            TEST_ASSERT_EQUAL( blockPayload[ i ], pPayload[ i ] );
        }
    }

    if( NULL != pPayload )
    {
        free( pPayload );
        pPayload = NULL;
    }
}