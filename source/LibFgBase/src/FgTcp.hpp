//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// These functions are defined in the OS-specific library

#ifndef FGTCP_HPP
#define FGTCP_HPP

#include "FgStdString.hpp"
#include "FgTypes.hpp"

namespace Fg {

// Returns false if unable to connect to server:
bool
fgTcpClient(
    const String &      hostname,       // DNS or IP
    uint16              port,
    const String &      data,
    bool                getResponse,
    String &            response);      // Ignored if 'getResponse' == false

inline
bool
fgTcpClient(const String & hostname,uint16 port,const String & data)
{
    String     dummy;
    return fgTcpClient(hostname,port,data,false,dummy);
}

inline
bool
fgTcpClient(const String & hostname,uint16 port,const String & data,String & response)
{return fgTcpClient(hostname,port,data,true,response); }

typedef std::function<bool        // Return false to terminate server
    (const String &,                // IP Address of the client
     const String &,                // Data from the client
     String &)>                     // Data to be returned to client (ignored if server not supposed to respond)
     FgFuncTcpHandler;

void
fgTcpServer(
    uint16              port,
    // If true, don't disconnect client until handler returns, then respond. Hander must complete
    // before TCP timeout in this case:
    bool                respond,
    FgFuncTcpHandler    handler,
    size_t              maxRecvBytes);  // Maximum number of bytes to receive in incomimg message

}

#endif
