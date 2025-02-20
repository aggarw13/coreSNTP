/*
 * coreSNTP v1.0.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 */

/**
 * @file core_sntp_client.h
 * @brief API of an SNTPv4 client library that can send time requests and receive time response to/from
 * SNTP/NTP servers. The library follows the Best Practices suggested in the the SNTPv4 specification,
 * [RFC 4330](https://tools.ietf.org/html/rfc4330).
 * The library can be used to run an SNTP client in a dedicated deamon task to periodically synchronize
 * time from the Internet.
 */

#ifndef CORE_SNTP_CLIENT_H_
#define CORE_SNTP_CLIENT_H_

/* Standard include. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Include coreSNTP Serializer header. */
#include "core_sntp_serializer.h"

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to resolve time server domain-name
 * to an IPv4 address.
 * The SNTP client library attempts to resolve the DNS of the time-server being
 * used every time the @ref Sntp_SendTimeRequest API is called.
 *
 * @param[in] pTimeServer The time-server whose IPv4 address is to be resolved.
 * @param[out] pIpV4Addr This should be filled with the resolved IPv4 address.
 * of @p pTimeServer.
 *
 * @return `true` if DNS resolution is successful; otherwise `false` to represent
 * failure.
 */
typedef bool ( * SntpResolveDns_t )( const char * pServerAddr,
                                     uint32_t * pIpV4Addr );

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to obtain the current system time
 * in SNTP timestamp format.
 *
 * @note If your platform follows UNIX representation of time, the
 * #SNTP_TIME_AT_UNIX_EPOCH_SECS and #SNTP_FRACTION_VALUE_PER_MICROSECOND macros
 * can be used to convert UNIX time to SNTP timestamp.
 *
 * @param[out] pCurrentTime This should be filled with the current system time
 * in SNTP timestamp format.
 *
 * @return `true` if obtaining system time is successful; otherwise `false` to
 * represent failure.
 */
typedef bool ( * SntpGetTime_t )( SntpTimestamp_t * pCurrentTime );

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to update the system clock time
 * so that it is synchronized the time server used for getting current time.
 *
 * @param[in] pTimeServer The time server used to request time.
 * @param[in] pServerTime The current time returned by the @p pTimeServer.
 * @param[in] clockOffSetSec The calculated clock offset of the system relative
 * to the server time.
 *
 * @note The user can use either a "step" or "slew" clock discipline methodology
 * depending on the application needs.
 * If the application requires a smooth time continuum of system time is required,
 * then the "slew" discipline methodology can be used with the clock offset value,
 * @p clockOffSetSec, to apply correction to the system clock with a "slew rate"
 * (that is higher than the SNTP polling rate).
 * If the application can accept sudden jump in time (forward or backward), then
 * the "step" discipline methodology can be used to directly update the system
 * clock with the current server time, @p pServerTime, every time the coreSNTP
 * library calls the interface.
 *
 * @return `true` if setting system time is successful; otherwise `false` to
 * represent failure.
 */
typedef bool ( * SntpSetTime_t )( const char * pTimeServer,
                                  const SntpTimestamp_t * pServerTime,
                                  int32_t clockOffsetSec );

/**
 * @brief The default UDP port supported by SNTP/NTP servers for client-server
 * communication.
 *
 * @note It is possible for a server to use a different port number than
 * the default port when using the Network Time Security protocol as the security
 * mechanism for SNTP communication. For more information, refer to Section 4.1.8
 * of [RFC 8915](https://tools.ietf.org/html/rfc8915).
 */
#define SNTP_DEFAULT_SERVER_PORT    ( 123U )

/**
 * @brief core_sntp_struct_types
 * @brief Structure representing information for a time server.
 */
typedef struct SntpServerInfo
{
    const char * pServerName; /**<@brief The time server endpoint. */
    uint16_t port;            /**<@brief The UDP port supported by the server
                               * for SNTP/NTP communication. */
} SntpServerInfo_t;

/**
 * @ingroup core_sntp_struct_types
 * @typedef NetworkContext_t
 * @brief A user-defined type for context that is passed to the transport interface functions.
 * It MUST be defined by the user to use the library.
 * It is of incomplete type to allow user to define to the needs of their transport
 * interface implementation.
 */
struct NetworkContext;
typedef struct NetworkContext NetworkContext_t;

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to send data to the network
 * over User Datagram Protocol (UDP).
 *
 * @param[in,out] pNetworkContext The user defined NetworkContext_t which
 * is opaque to the coreSNTP library.
 * @param[in] pTimeServer The time server to send the data to.
 * @param[in] pBuffer The buffer containing the data to send over the network.
 * @param[in] bytesToSend The size of data in @p pBuffer to send.
 *
 * @return The function SHOULD return one of the following integer codes:
 * - @p bytesToSend when all requested data is successfully transmitted over the
 * network.
 * - >0 value representing number of bytes sent when only partial data is sent
 * over the network.
 * - 0 when no data could be sent over the network (due to network buffer being
 * full, for example), and the send operation can be retried.
 * - -2 when the send operation failed to send any data due to an internal error,
 * and operation cannot be retried.
 */
typedef int32_t ( * UdpTransportSendTo_t )( NetworkContext_t * pNetworkContext,
                                            const SntpServerInfo_t * pTimeServer,
                                            const void * pBuffer,
                                            size_t bytesToSend );

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to receive data from the network
 * over User Datagram Protocol (UDP).
 *
 * @param[in,out] pNetworkContext The user defined NetworkContext_t which
 * is opaque to the coreSNTP library.
 * @param[in] pTimeServer The time server to receive data from.
 * @param[out] pBuffer This SHOULD be filled with data received from the network.
 * @param[in] bytesToRecv The expected number of bytes to receive from the
 * server.
 *
 * @return The function SHOULD return one of the following integer codes:
 * - @p bytesToRecv value if all the requested number of bytes are received
 * from the network.
 * - > 0 value representing number of bytes received when partial data is
 * received from the network.
 * - ZERO when no data is available on the network, and the operation can be
 * retried.
 * - -2 when the read operation failed due to internal error, and operation cannot
 * be retried.
 */
typedef int32_t ( * UdpTransportRecvFrom_t )( NetworkContext_t * pNetworkContext,
                                              SntpServerInfo_t * pTimeServer,
                                              void * pBuffer,
                                              size_t bytesToRecv );

/**
 * @ingroup core_sntp_struct_types
 * @brief Struct representing the UDP transport interface for user-defined functions
 * that coreSNTP library depends on for performing read/write network operations.
 */
typedef struct UdpTransportIntf
{
    NetworkContext_t * pUserContext; /**<@brief The user-defined context for storing
                                      * network socket information. */
    UdpTransportSendTo_t sendTo;     /**<@brief The user-defined UDP send function. */
    UdpTransportRecvFrom_t recvFrom; /**<@brief The user-defined UDP receive function. */
} UdpTransportInterface_t;

/**
 * @ingroup core_sntp_struct_types
 * @typedef SntpAuthContext_t
 * @brief A user-defined type for context that is passed to the authentication interface functions.
 * It MUST be defined by the user to use the library.
 * It is of incomplete type to allow user to defined to the the needs of their authentication
 * interface implementation.
 */
struct SntpAuthContext;
typedef struct SntpAuthContext SntpAuthContext_t;

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to generate and append
 * authentication code in an SNTP request buffer for the SNTP client to be
 * authenticated by the time server, if a security mechanism is used.
 *
 * The user can choose to implement with any security mechanism, symmetric
 * key-based (like AES-CMAC) or asymmetric key-based (like Network Time Security),
 * depending on the security mechanism supported by the time server being used
 * to synchronize time with.
 *
 * @note The function SHOULD generate the authentication data for the first
 * #SNTP_PACKET_BASE_SIZE bytes of SNTP request packet present in the passed buffer
 * @p pBuffer, and fill the generated authentication data after #SNTP_PACKET_BASE_SIZE
 * bytes in the buffer.
 *
 * @param[in,out] pContext The user defined NetworkContext_t which
 * is opaque to the coreSNTP library.
 * @param[in] pTimeServer The time server being used to request time from.
 * This parameter is useful to choose the security mechanism when multiple time
 * servers are configured in the library, and they require different security
 * mechanisms or authentication credentials to use.
 * @param[in, out] pBuffer This buffer SHOULD be filled with the authentication
 * code generated from the #SNTP_PACKET_BASE_SIZE bytes of SNTP request data
 * present in it.
 * @param[in] bufferSize The maximum amount of data that can be held by
 * the buffer, @p pBuffer.
 * @param[out] pAuthCodeSize The size of authentication data filled in the buffer.
 *
 * @return The function SHOULD return one of the following integer codes:
 * - #SntpSuccess when the server is successfully authenticated.
 * - #SntpErrorBufferTooSmall when the user-supplied buffer (to the SntpContext_t through
 * Sntp_Init) is not large enough to hold authentication data.
 * - #SntpErrorAuthFailure for failure to generate authentication data due to internal
 * error.
 */
typedef SntpStatus_t (* SntpGenerateAuthCode_t )( SntpAuthContext_t * pContext,
                                                  const char * pTimeServer,
                                                  void * pBuffer,
                                                  size_t bufferSize,
                                                  size_t * pAuthCodeSize );

/**
 * @ingroup core_sntp_callback_types
 * @brief Interface for user-defined function to authenticate server by validating
 * the authentication code present in its SNTP response to a time request, if
 * a security mechanism is supported by the server.
 *
 * The user can choose to implement with any security mechanism, symmetric
 * key-based (like AES-CMAC) or asymmetric key-based (like Network Time Security),
 * depending on the security mechanism supported by the time server being used
 * to synchronize time with.
 *
 * @note In an SNTP response, the authentication code is present only after the
 * first #SNTP_PACKET_BASE_SIZE bytes. Depending on the security mechanism used,
 * the first #SNTP_PACKET_BASE_SIZE bytes MAY be used in validating the
 * authentication data sent by the server.
 *
 * @param[in,out] pContext The user defined NetworkContext_t which
 * is opaque to the coreSNTP library.
 * @param[in] pTimeServer The time server that has to be authenticated from its
 * SNTP response.
 * This parameter is useful to choose the security mechanism when multiple time
 * servers are configured in the library, and they require different security
 * mechanisms or authentication credentials to use.
 * @param[in] pResponseData The SNTP response from the server that contains the
 * authentication code after the first #SNTP_PACKET_BASE_SIZE bytes.
 * @param[in] responseSize The total size of the response from the server.
 *
 * @return The function SHOULD return one of the following integer codes:
 * - #SntpSuccess when the server is successfully authenticated.
 * - #SntpServerNotAuthenticated when server could not be authenticated.
 * - #SntpErrorAuthFailure for failure to authenticate server due to internal
 * error.
 */
typedef SntpStatus_t (* SntpValidateAuthCode_t )( SntpAuthContext_t * pContext,
                                                  const char * pTimeServer,
                                                  const void * pResponseData,
                                                  size_t responseSize );

/**
 * @ingroup core_sntp_struct_types
 * @brief Struct representing the authentication interface for securely
 * communicating with time servers.
 *
 * @note Using a security mechanism is OPTIONAL for using the coreSNTP
 * library i.e. a user does not need to define the authentication interface
 * if they are not using a security mechanism for SNTP communication.
 */
typedef struct SntpAuthenticationIntf
{
    /**
     *@brief The user-defined context for storing information like
     * key credentials required for cryptographic operations in the
     * security mechanism used for communicating with server.
     */
    SntpAuthContext_t * pAuthContext;

    /**
     * @brief The user-defined function for appending client authentication data.
     * */
    SntpGenerateAuthCode_t generateClientAuth;

    /**
     * @brief The user-defined function to authenticating server from its SNTP
     * response.
     */
    SntpValidateAuthCode_t validateServer;
} SntpAuthenticationInterface_t;

/**
 * @ingroup core_sntp_struct_types
 * @brief Structure for a context that stores state for managing a long-running
 * SNTP client that periodically polls time and synchronizes system clock.
 */
typedef struct SntpContext
{
    /**
     * @brief List of time servers in decreasing priority order configured
     * for the SNTP client.
     * Only a single server is configured for use at a time across polling
     * attempts until the server rejects a time request or there is a response
     * timeout, after which, the next server in the list is used for subsequent
     * polling requests.
     */
    const SntpServerInfo_t * pTimeServers;

    /**
     * @brief Number of servers configured for use.
     */
    size_t numOfServers;

    /**
     * @brief The index for the currently configured time server for time querying
     * from the list of time servers in @ref pTimeServers.
     */
    size_t currentServerIndex;

    /**
     * @brief The user-supplied buffer for storing network data of both SNTP requests
     * and SNTP response.
     */
    uint8_t * pNetworkBuffer;

    /**
     * @brief The size of the network buffer.
     */
    size_t bufferSize;

    /**
     * @brief The user-supplied function for resolving DNS name of time servers.
     */
    SntpResolveDns_t resolveDnsFunc;

    /**
     * @brief The user-supplied function for obtaining the current system time.
     */
    SntpGetTime_t getTimeFunc;

    /**
     * @brief The user-supplied function for correcting system time after receiving
     * time from a server.
     */
    SntpSetTime_t setTimeFunc;

    /**
     * @brief The user-defined interface for performing User Datagram Protocol (UDP)
     * send and receive network operations.
     */
    UdpTransportInterface_t networkIntf;

    /**
     * @brief The user-defined interface for incorporating security mechanism of
     * adding client authentication in SNTP request as well as authenticating server
     * from SNTP response.
     *
     * @note If the application will not use security mechanism for any of the
     * configured servers, then this interface can be undefined.
     */
    SntpAuthenticationInterface_t authIntf;

    /**
     * @brief Cache of the resolved Ipv4 address of the current server being used for
     * time synchronization.
     * As a Best Practice functionality, the client library attempts to resolve the
     * DNS of the time-server every time the @ref Sntp_SendTimeRequest API is called.
     */
    uint32_t currentServerIpV4Addr;

    /**
     * @brief Cache of the timestamp of sending the last time request to a server
     * for replay attack protection by checking that the server response contains
     * the same timestamp in its "originate timestamp" field.
     */
    SntpTimestamp_t lastRequestTime;

    /**
     * @brief State variable for storing the size of the SNTP packet that includes
     * both #SNTP_PACKET_BASE_SIZE bytes plus any authentication data, if a security
     * mechanism is used.
     * This state variable is used for expecting the same size for an SNTP response
     * from the server.
     */
    size_t sntpPacketSize;
} SntpContext_t;

/**
 * @brief Initializes a context for SNTP client communication with SNTP/NTP
 * servers.
 *
 * @param[out] pContext The user-supplied memory for the context that will be
 * initialized to represent an SNTP client.
 * @param[in] pTimeServers The list of decreasing order of priority of time
 * servers that should be used by the SNTP client. This list MUST stay in
 * scope for all the time of use of the context.
 * @param[in] numOfServers The number of servers in the list, @p pTimeServers.
 * @param[in] pNetworkBuffer The user-supplied memory that will be used for
 * storing network data for SNTP client-server communication. The buffer
 * MUST stay in scope for all the time of use of the context.
 * @param[in] bufferSize The size of the passed buffer @p pNetworkBuffer. The buffer
 * SHOULD be appropriately sized for storing an entire SNTP packet which includes
 * both #SNTP_PACKET_BASE_SIZE bytes of standard SNTP packet size, and space for
 * authentication data, if security mechanism is used to communicate with any of
 * the time servers configured for use.
 * @param[in] resolveDnsFunc The user-defined function for DNS resolution of time
 * server.
 * @param[in] getSystemTimeFunc The user-defined function for querying system
 * time.
 * @param[in] setSystemTimeFunc The user-defined function for correcting system
 * time for every successful time response received from a server.
 * @param[in] pUdpTransportIntf The user-defined function for performing network
 * send/recv operations over UDP.
 * @param[in] pAuthIntf The user-defined interface for generating client authentication
 * in SNTP requests and authenticating servers in SNTP responses, if security mechanism
 * is used in SNTP communication with server(s). If security mechanism is not used in
 * communication with any of the configured servers (in @p pTimeServers), then the
 * @ref SntpAuthenticationInterface_t does not need to be defined and this parameter
 * can be NULL.
 *
 * @return This function returns one of the following:
 * - #SntpSuccess if the context is initialized.
 * - #SntpErrorBadParameter if any of the passed parameters in invalid.
 * - #SntpErrorBufferTooSmall if the buffer does not have the minimum size
 * required for a valid SNTP response packet.
 */
/* @[define_sntp_init] */
SntpStatus_t Sntp_Init( SntpContext_t * pContext,
                        const SntpServerInfo_t * pTimeServers,
                        size_t numOfServers,
                        uint8_t * pNetworkBuffer,
                        size_t bufferSize,
                        SntpResolveDns_t resolveDnsFunc,
                        SntpGetTime_t getSystemTimeFunc,
                        SntpSetTime_t setSystemTimeFunc,
                        const UdpTransportInterface_t * pTransportIntf,
                        const SntpAuthenticationInterface_t * pAuthIntf );
/* @[define_sntp_init] */


#endif /* ifndef CORE_SNTP_CLIENT_H_ */
