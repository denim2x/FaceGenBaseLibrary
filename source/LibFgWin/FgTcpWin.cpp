//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
//

#include "stdafx.h"
#include "FgTcp.hpp"
#include "FgThrowWindows.hpp"
#include "FgStdString.hpp"
#include "FgDiagnostics.hpp"
#include "FgOut.hpp"

// Tell compiler to link to these libs:
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// Don't warn about deprecation ('inet_ntoa'). TODO: Upgrade TCP stuff for IPV6
#  pragma warning(disable:4996)

namespace Fg {

struct  FgWinsockDll
{
    WSADATA     wsaData;

    FgWinsockDll() {
        // Initialize Winsock DLL version 2.2:
        int         itmp = WSAStartup(MAKEWORD(2,2),&wsaData);
        if(itmp != 0)
            FGASSERT_FALSE1(toString(itmp));
    }

    ~FgWinsockDll() {
        // Every successful call to WSAStartup must be matched by a WSACleanup()
        // (windows uses a counter):
        WSACleanup();
    }
};

static
void
initWinsock()
{
    static  FgWinsockDll  wsd;
}

bool
fgTcpClient(
    const String &      hostname,
    uint16              port,
    const String &      data,
    bool                getResponse,
    String &            response)
{
    initWinsock();
    SOCKET              socketHandle;
    struct addrinfo *   addressInfo;
    socketHandle = INVALID_SOCKET;
    addressInfo = NULL;
    struct addrinfo     hints;
    int                 itmp;
    ZeroMemory(&hints,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port.
    itmp = getaddrinfo(
        hostname.c_str(),           // Hostname or IP #
        toString(port).c_str(),   // Service name or port #
        &hints,
        &addressInfo);              // RETURNED
    if (itmp != 0)
        return false;               // Server does not appear to exist
    struct addrinfo *ptr = NULL;
    // Attempt to connect to an IP address until one succeeds. It's possible
    // for more than 1 to be returned if there are more than 1 namespace service
    // providers (for instance local hosts file vs. DNS server), according to
    // MS docs:
    for(ptr=addressInfo; ptr != NULL ;ptr=ptr->ai_next) {
        socketHandle =
            socket(
                ptr->ai_family,
                ptr->ai_socktype, 
                ptr->ai_protocol);
        if (socketHandle == INVALID_SOCKET) {
            // Couldn't use scope guard for freeaddrinfo due to compile issues:
            freeaddrinfo(addressInfo);
            FGASSERT_FALSE1(toString(WSAGetLastError()));
        }
        // Set the timeout so the user isn't waiting for ages if the connection fails:
        DWORD           timeout = 5000;     // 5 seconds
        setsockopt(socketHandle,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
        // Try to connect to the server
        itmp = connect(socketHandle,ptr->ai_addr,(int)ptr->ai_addrlen);
        if (itmp == 0)
            break;
        else {
            closesocket(socketHandle);
            socketHandle = INVALID_SOCKET;
        }
    }
    freeaddrinfo(addressInfo);
    if (socketHandle == INVALID_SOCKET)
        return false;               // Unable to connect, server not listening ?
    // 'send' will block for buffering since we've created a blocking socket, so the
    // value returned is always either the data size or an error:
    itmp = send(socketHandle,data.data(),int(data.size()),0);
    FGASSERT1(itmp != SOCKET_ERROR,toString(WSAGetLastError()));
    // close socket for sending to cause server's recv/read to return a zero
    // size data packet if server is waiting for more (ie to flush the stream).
    itmp = shutdown(socketHandle,SD_SEND);
    FGASSERT1(itmp != SOCKET_ERROR,toString(WSAGetLastError()));
    if (getResponse) {
        response.clear();
        do {
            char    buff[1024];
            // If server doesn't respond and closes connection we'll immediately get
            // a value of zero here and nothing will be placed in buff. Otherwise
            // we'll continue to receive data until server closes connection causing
            // the zero message:
            itmp = recv(socketHandle,buff,sizeof(buff),0);
            // This can happen for many reasons (eg. connection interrupted) so don't throw:
            if (itmp == SOCKET_ERROR) {
                //toString(WSAGetLastError()));
                closesocket(socketHandle);
                return false;
            }
            if (itmp > 0)
                response += String(buff,itmp);
        }
        while (itmp > 0);
        FGASSERT(itmp == 0);
    }
    // Couldn't use scope guard due to compile issues:
    closesocket(socketHandle);
    return true;
}

void
fgTcpServer(
    uint16              port,
    bool                respond,
    FgFuncTcpHandler    handler,
    size_t              maxRecvBytes)
{
    initWinsock();
    SOCKET      sockListen = INVALID_SOCKET;

    // Set up the listening socket:
    struct addrinfo     hints;
    ZeroMemory(&hints,sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // A reliable, 2-way, stream-based connection (requires TCP)
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo     *addrInfoPtr = NULL;
    int itmp = getaddrinfo(NULL,toString(port).c_str(),&hints,&addrInfoPtr);
    FGASSERT1(itmp == 0,toString(itmp));
    sockListen = socket(addrInfoPtr->ai_family,addrInfoPtr->ai_socktype,addrInfoPtr->ai_protocol);
    if (sockListen == INVALID_SOCKET) {
        freeaddrinfo(addrInfoPtr);
        FGASSERT_FALSE1(toString(WSAGetLastError()));
    }
    itmp = bind(sockListen,addrInfoPtr->ai_addr,(int)addrInfoPtr->ai_addrlen);
    if (itmp == SOCKET_ERROR) {
        closesocket(sockListen);
        freeaddrinfo(addrInfoPtr);
        FGASSERT_FALSE1(toString(WSAGetLastError()));
    }
    itmp = listen(sockListen, SOMAXCONN);
    if (itmp == SOCKET_ERROR) {
        closesocket(sockListen);
        freeaddrinfo(addrInfoPtr);
        FGASSERT_FALSE1(toString(WSAGetLastError()));
    }
    freeaddrinfo(addrInfoPtr);

    // Receive messages and respond until finished:
    SOCKET      sockClient;
    bool        handlerRetval = true;
    do {
        fgout << fgnl << "> " << std::flush;
        sockaddr_in     sa;
        sa.sin_family = AF_INET;
        socklen_t       sz = sizeof(sa);
        sockClient = accept(sockListen,(sockaddr*)(&sa),&sz);
        if (sockClient == INVALID_SOCKET) {
            closesocket(sockListen);
            FGASSERT_FALSE1(toString(WSAGetLastError()));
        }
        // Set the timeout. Very important since the default is to never time out so in some
        // cases a broken connection causes 'recv' below to block forever:
        DWORD           timeout = 5000;     // 5 seconds
        setsockopt(sockClient,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
		char * clientStringPtr = inet_ntoa(sa.sin_addr);
            FGASSERT(clientStringPtr != NULL);
        String     ipAddr = String(clientStringPtr);
        //fgout << "receiving from " << ipAddr << " ... " << std::flush;
        String     dataBuff;
        int retVal = 0;
        do {
            char    recvbuf[1024];
            // recv() will return when either it has filled the buffer, copied over everthing
            // from the socket input buffer (only if non-empty), or when the the read connection
            // is closed by the client. Otherwise it will block (ie if input buffer empty):
            retVal = recv(sockClient,recvbuf,sizeof(recvbuf),0);
            fgout << "." << std::flush;
            if (retVal > 0)
                dataBuff += String(recvbuf,retVal);
        }
        while ((retVal > 0) && (dataBuff.size() <= maxRecvBytes));
        if (retVal != 0) {
            closesocket(sockClient);
            if (retVal < 0)
                fgout << "TCP RECV ERROR: " << retVal;
            else if (retVal > 0)
                fgout << " OVERSIZE MESSAGE IGNORED.";
            fgout << std::flush;
            continue;
        }
        fgout << ": " << std::flush;
        if (!respond)   // Avoid timeout errors on the data socket for long handlers that don't respond:
            closesocket(sockClient);
        String     response;
        try {
            handlerRetval = handler(ipAddr,dataBuff,response);
        }
        catch(FgException const & e) {
            fgout << "Handler exception (FG exception): " << e.no_tr_message();
        }
        catch(std::exception const & e) {
            fgout << "Handler exception (std::exception): " << e.what();
        }
        catch(...) {
            fgout << "Handler exception (unknown type)";
        }
        if (respond) {
            if (!response.empty()) {
                int     bytesSent = send(sockClient,response.data(),int(response.size()),0);
                shutdown(sockClient,SD_SEND);
                if (bytesSent != int(response.size()))
                    fgout << "TCP SEND ERROR: " << bytesSent << " (of " << response.size() << ").";
            }
            closesocket(sockClient);
        }
        fgout << std::flush;
    } while (handlerRetval == true);
    closesocket(sockListen);
}

}

// */
