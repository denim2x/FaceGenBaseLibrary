//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"
#include "FgDiagnostics.hpp"
#include "FgOut.hpp"
#include "FgString.hpp"

using namespace std;

namespace Fg {

string
fgDiagString(const char * fname,int line)
{
    ostringstream   retval;
    retval << fname << ": " << line;
    return retval.str();
}

string
fgDiagString(const char * fname,int line,const string & msg)
{
    ostringstream   retval;
    retval << fname << ": " << line;
    if (msg.size() > 0)
        retval << " Info: " << msg;
    return retval.str();
}

void
fgAssert(const char * fname,int line,const string &  msg)
{
    fgThrow("Internal program error", fgDiagString(fname,line,msg));
}

void
fgWarn(const std::string & msg,const std::string & dataUtf8)
{
    fgout << "\nWARNING: " << msg << ": " << dataUtf8;
}

void
fgWarn(const char * fname,int line,const string & msg)
{
    fgout << "\nWARNING at : " <<  fgDiagString(fname,line,msg);
}

}
