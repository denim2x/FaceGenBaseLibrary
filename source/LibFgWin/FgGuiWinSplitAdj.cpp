//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
//

#include "stdafx.h"

#include "FgGuiApiSplit.hpp"
#include "FgGuiWin.hpp"
#include "FgThrowWindows.hpp"
#include "FgMatrixC.hpp"
#include "FgBounds.hpp"
#include "FgMetaFormat.hpp"

using namespace std;

namespace Fg {

static const int dividerThickness = 3;
static const int borderThickness = 2;

struct  GuiSplitWinDivider
{
    bool        drag=false;     // Is the user dragging this window ?
    // Last mouse pos in screen coords (since this window is moving !)
    // Only valid when drag == true:
    Vec2I    lastPos;
    Vec2UI   parentCursorLo;
    Vec2UI   parentCursorHi;

    LRESULT
    wndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
    {
        // We need to track this because we don't want to adjust if the user
        // initiated the drag in another window. Also it's an easier way of
        // calculating deltas than calling the complicated GetMouseMovePointsEx:
        if (msg == WM_LBUTTONDOWN) {
            if (wParam == MK_LBUTTON) {
                SetCapture(hwnd);
                lastPos = winScreenPos(hwnd,lParam);
                drag = true;
                HWND    pHwnd = GetParent(hwnd);
                POINT   point;
                point.x = parentCursorLo[0];
                point.y = parentCursorLo[1];
                FGASSERTWIN(ClientToScreen(pHwnd,&point));
                RECT    rect;
                rect.left = point.x;
                rect.top = point.y;
                rect.right = parentCursorHi[0] + (point.x - parentCursorLo[0]);
                rect.bottom = parentCursorHi[1] + (point.y - parentCursorLo[1]);
                FGASSERTWIN(ClipCursor(&rect));
            }
        }
        else if (msg == WM_LBUTTONUP) {
            if (drag) {
                ReleaseCapture();
                drag = false;
                ClipCursor(NULL);
            }
        }
        else if (msg == WM_MOUSEMOVE) {
            if (wParam == MK_LBUTTON) {
                if (drag) {
                    Vec2I    pos = winScreenPos(hwnd,lParam);
                    Vec2I    delta = pos-lastPos;
                    lastPos = pos;
                    // SendMessage doesn't return until msg is processed.
                    // We can't just forward the WM_MOUSEMOVE since the coords
                    // aren't translated and we need to specify which divider:
                    SendMessage(
                        GetParent(hwnd),
                        WM_USER,
                        WPARAM(2),
                        MAKELPARAM(WORD(delta[0]),WORD(delta[1])));
                }
            }
        }
        else if (msg == WM_CAPTURECHANGED) {
            drag = false;
            ClipCursor(NULL);
        }
        else
            return DefWindowProc(hwnd,msg,wParam,lParam);
        return 0;
    }
};

struct  GuiSplitAdjWin : public GuiBaseImpl
{
    GuiSplitAdj                 m_api;
    HWND                        hwndThis;   // Must receive user messages from divider
    GuiImplPtr                  m_pane0;
    GuiImplPtr                  m_pane1;
    double                      m_relSize;
    struct  Div
    {
        GuiSplitWinDivider      win;
        HWND                    hwnd;
    };
    Div                         m_div;
    Vec2I                    m_client;
    Ustring                    m_store;
    // Cache border-padded pane min sizes to avoid exponentially repeated calls:
    mutable Vec2UI           m_minSizes[2];  

    GuiSplitAdjWin(const GuiSplitAdj & api) :
        m_api(api),
        m_relSize(0.5)  // Default must be fixed since we can't yet call getMinSize of subwindows.
    {
        m_pane0 = api.pane0->getInstance();
        m_pane1 = api.pane1->getInstance();
    }

    virtual void
    create(HWND parentHwnd,int ident,const Ustring & store,DWORD extStyle,bool visible)
    {
//fgout << fgnl << "SplitAdj::create: visible: " << visible << " extStyle: " << extStyle << fgpush;
        m_store = store;
        double      rs;
        if (fgLoadXml(m_store+".xml",rs,false))
            if ((rs > 0.0) && (rs < 1.0))
                m_relSize = rs;
        WinCreateChild   cc;
        cc.extStyle = extStyle;
        cc.visible = visible;
        winCreateChild(parentHwnd,ident,this,cc);
//fgout << fgpop;
    }

    virtual void
    destroy()
    {
        // Automatically destroys children first:
        DestroyWindow(hwndThis);
    }

    void
    minSizes() const
    {
        m_minSizes[0] = m_pane0->getMinSize() + Vec2UI(2*borderThickness);
        m_minSizes[1] = m_pane1->getMinSize() + Vec2UI(2*borderThickness);
    }

    virtual Vec2UI
    getMinSize() const
    {
        minSizes();     // Upadate cache of subwindow sizes
        uint        sd = splitDim(),
                    nd = 1 - sd;
        Vec2UI   max = cMax(m_minSizes[0],m_minSizes[1]),
                    sum = m_minSizes[0] + m_minSizes[1],
                    ret;
        ret[sd] = sum[sd] + dividerThickness;
        ret[nd] = max[nd];
        return ret;
    }

    virtual Vec2B
    wantStretch() const
    {return fgOr(m_pane0->wantStretch(),m_pane1->wantStretch()); }

    virtual void
    updateIfChanged()
    {
//fgout << fgnl << "SplitAdj::updateIfChanged" << fgpush;
        m_pane0->updateIfChanged();
        m_pane1->updateIfChanged();
//fgout << fgpop;
    }

    virtual void
    moveWindow(Vec2I lo,Vec2I sz)
    {
//fgout << fgnl << "SplitAdj::moveWindow: " << lo << "," << sz << fgpush;
        MoveWindow(hwndThis,lo[0],lo[1],sz[0],sz[1],FALSE);
//fgout << fgpop;
    }

    virtual void
    showWindow(bool s)
    {
//fgout << fgnl << "SplitAdj::showWindow: " << s << fgpush;
        ShowWindow(hwndThis,s ? SW_SHOW : SW_HIDE);
//fgout << fgpop;
    }

    virtual void
    saveState()
    {
        fgSaveXml(m_store+".xml",m_relSize,false);
        m_pane0->saveState();
        m_pane1->saveState();
    }

    LRESULT
    wndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
    {
        if (msg == WM_CREATE) {
//fgout << fgnl << "SplitAdj::WM_CREATE" << fgpush;
            hwndThis = hwnd;
            DWORD       extStyle = WS_EX_CLIENTEDGE;
            m_pane0->create(hwnd,0,m_store+"_0",extStyle);
            m_pane1->create(hwnd,1,m_store+"_1",extStyle);
            WinCreateChild   cc;
            cc.cursor = LoadCursor(NULL,m_api.horiz ? IDC_SIZEWE : IDC_SIZENS);
            m_div.hwnd = winCreateChild(hwnd,2,&m_div.win,cc);
//fgout << fgpop;
        }
        else if (msg == WM_SIZE) {      // Sends new size of client area:
            minSizes();     // Upadate cache of subwindow sizes
            m_client = Vec2I(LOWORD(lParam),HIWORD(lParam));
            if (m_client[0] * m_client[1] == 0)
                return 0;
//fgout << fgnl << "SplitAdj::WM_SIZE: " << m_client << fgpush;
            // Set the adjustment cursor bounds:
            uint        sd = splitDim(),
                        nd = 1-sd;
            m_div.win.parentCursorLo[nd] = borderThickness;
            m_div.win.parentCursorLo[sd] = m_minSizes[0][sd];
            m_div.win.parentCursorHi[nd] = m_client[nd] - borderThickness;
            m_div.win.parentCursorHi[sd] = m_client[sd] - m_minSizes[1][sd];
            resizePanes();
//fgout << fgpop;
        }
        else if (msg == WM_USER) {      // Divider has been moved:
            minSizes();     // Upadate cache of subwindow sizes
            Vec2I    delta(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
            uint        sd = splitDim();
            if (delta[sd] != 0)
            {
                int     sizet = m_client[sd] - dividerThickness;
                double  reld = double(delta[sd]) / double(sizet);
                m_relSize += reld;
                resizePanes();
                InvalidateRect(hwndThis,NULL,TRUE);
            }
        }
//case WM_PAINT:
//fgout << fgnl << "SplitAdj::WM_PAINT";
        else
            return DefWindowProc(hwnd,msg,wParam,lParam);
        return 0;
    }

    uint
    splitDim() const
    {return m_api.horiz ? 0 : 1; }

    struct  Pos
    {
        Vec2I    lo;
        Vec2I    sz;
    };
    struct  Layout
    {
        Pos         pane0;
        Pos         divider;
        Pos         pane1;
    };
    Layout
    layout()
    {
        Layout      ret;
        uint        sd = splitDim(),
                    nd = 1-sd;
        int         sizei = m_client[sd]-dividerThickness;
        double      sized = sizei;
        Vec2I    lo(0),sz(0);
        sz[nd] = m_client[nd];
        sz[sd] = round<int>(m_relSize*sized);
        // Adjust relative sizing if window resize hits one of the mins:
        int         min0 = m_minSizes[0][sd],
                    min1 = m_minSizes[1][sd];
        if (sz[sd] < min0)
            sz[sd] = min0;
        if (sizei-sz[sd] < min1)
            sz[sd] = sizei - min1;
        m_relSize = double(sz[sd]) / sized;
        ret.pane0.lo = lo;
        ret.pane0.sz = sz;
        lo[sd] = sz[sd];
        sz[sd] = dividerThickness;
        ret.divider.lo = lo;
        ret.divider.sz = sz;
        sz[sd] = sizei - lo[sd];
        lo[sd] += dividerThickness;
        ret.pane1.lo = lo;
        ret.pane1.sz = sz;
        return ret;
    }

    void
    resizePanes()
    {
        Layout      lt = layout();
        m_pane0->moveWindow(lt.pane0.lo,lt.pane0.sz);
        MoveWindow(m_div.hwnd,lt.divider.lo[0],lt.divider.lo[1],lt.divider.sz[0],lt.divider.sz[1],TRUE);
        m_pane1->moveWindow(lt.pane1.lo,lt.pane1.sz);
    }
};

GuiImplPtr
guiGetOsImpl(const GuiSplitAdj & def)
{return GuiImplPtr(new GuiSplitAdjWin(def)); }

}
