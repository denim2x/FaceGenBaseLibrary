//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgGuiApiButton.hpp"

using namespace std;

namespace Fg {

GuiPtr
guiButton(const Ustring & label,FgFnVoid2Void action)
{
    GuiButton      b;
    b.label = label;
    b.action = action;
    return std::make_shared<GuiButton>(b);
}

GuiPtr
guiButtonTr(const string & label,FgFnVoid2Void action)
{
    GuiButton      b;
    b.label = label;
    b.action = action;
    return std::make_shared<GuiButton>(b);
}

}

// */
