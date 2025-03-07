//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
//

#include "stdafx.h"

#include "FgGuiApiDynamic.hpp"
#include "FgGuiWin.hpp"
#include "FgThrowWindows.hpp"
#include "FgMatrixC.hpp"
#include "FgBounds.hpp"

using namespace std;

namespace Fg {

struct  GuiDynamicWin : public GuiBaseImpl
{
    GuiDynamic                  m_api;
    GuiImplPtr                  m_win;
    Ustring                    m_store;
    HWND                        m_hwndParent;
    Vec2I                    m_lastMoveLo;
    Vec2I                    m_lastMoveSz;

    GuiDynamicWin(const GuiDynamic & api) :
        m_api(api)
    {}

    virtual void
    create(HWND parentHwnd,int,const Ustring & store,DWORD /*extStyle*/,bool visible)
    {
//fgout << fgnl << "GuiDynamicWin::create" << fgpush;
        // Ignore extStyle since this isn't a win32 window and it's not recursively passed.
        m_store = store;
        m_hwndParent = parentHwnd;
        // Since this is not a win32 window, we create the sub-windows here within the parent
        // WM_CREATE handler:
        m_win = m_api.makePane()->getInstance();
        m_win->create(parentHwnd,0,store,NULL,visible);
        m_api.updateFlag->checkUpdate();    // Avoid destroying and re-creating on startup
//fgout << fgpop;
    }

    virtual void
    destroy()
    {m_win->destroy(); }

    virtual Vec2UI
    getMinSize() const
    {
        if (m_win)
            return m_win->getMinSize();
        else
            return Vec2UI(64);
    }

    virtual Vec2B
    wantStretch() const
    {
        if (m_win)
            return m_win->wantStretch();
        else
            return Vec2B(false);
    }

    virtual void
    updateIfChanged()
    {
        if (m_api.updateFlag->checkUpdate()) {
            // This function is only called by the parent if this window is visible:
            m_win->destroy();
            m_win = m_api.makePane()->getInstance();
            m_win->create(m_hwndParent,0,m_store,NULL,true);
            // Must tell the panels what size to be before they can be displayed:
            m_win->moveWindow(m_lastMoveLo,m_lastMoveSz);
        }
        m_win->updateIfChanged();
    }

    virtual void
    moveWindow(Vec2I base,Vec2I size)
    {
//fgout << fgnl << "GuiDynamicWin::moveWindow " << base << " , " << size << fgpush;
        m_lastMoveLo = base;
        m_lastMoveSz = size;
        m_win->moveWindow(base,size);
//fgout << fgpop;
    }

    virtual void
    showWindow(bool s)
    {m_win->showWindow(s); }

    virtual void
    saveState()
    {m_win->saveState(); }
};

GuiImplPtr
guiGetOsImpl(const GuiDynamic & def)
{return GuiImplPtr(new GuiDynamicWin(def)); }

}
