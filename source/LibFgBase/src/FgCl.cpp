//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgCl.hpp"
#include "FgStdString.hpp"
#include "FgException.hpp"
#include "FgString.hpp"
#include "FgOut.hpp"

using namespace std;

#ifndef FG_SANDBOX

namespace Fg {

namespace fgCl {

bool preview = false;

bool
run(const string & cmd,bool throwIfError,int rvalMask)
{
    fgout << fgnl << cmd << "\n";   // DOS output lines will always start in zero'th column anyway
    int     retval = 0;
    if (!preview)
        retval = system(cmd.c_str());
    // Some commands such as robocopy have several non-error return codes.
    if ((retval & rvalMask) != 0)
    {
        if (throwIfError)
            fgThrow("Error exit code from command",toString(retval));
        else
            fgout << fgnl << "Error exit code from command: " << retval;
        return false;
    }
    return true;
}

}   // namespace

}

#endif
