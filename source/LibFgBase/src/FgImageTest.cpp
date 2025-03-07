//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgImage.hpp"
#include "FgTime.hpp"
#include "FgImgDisplay.hpp"
#include "FgImage.hpp"
#include "FgTestUtils.hpp"
#include "FgApproxEqual.hpp"
#include "FgCommand.hpp"
#include "FgSyntax.hpp"

using namespace std;

namespace Fg {

namespace {

// MANUAL:

void
display(const CLArgs &)
{
    Ustring    dd = dataDir();
    string      testorig_jpg("base/test/testorig.jpg");
    ImgC4UC img;
    imgLoadAnyFormat(dd+testorig_jpg,img);
    imgDisplay(img);
    string      testorig_bmp("base/test/testorig.bmp");
    imgLoadAnyFormat(dd+testorig_bmp,img);
    imgDisplay(img);
}

void
resize(const CLArgs &)
{
    string              fname("base/test/testorig.jpg");
    ImgC4UC         img;
    imgLoadAnyFormat(dataDir()+fname,img);
    ImgC4UC         out(img.width()/2+1,img.height()+1);
    fgImgResize(img,out);
    imgDisplay(out);
}

void
sfs(const CLArgs &)
{
    ImgC4UC         orig = imgLoadAnyFormat(dataDir()+"base/Mandrill512.png");
    Img<Vec3F>   img(orig.dims());
    for (size_t ii=0; ii<img.numPixels(); ++ii)
        img[ii] = Vec3F(orig[ii].m_c.subMatrix<3,1>(0,0));
    FgTimer             time;
    for (uint ii=0; ii<100; ++ii)
        fgSmoothFloat(img,img,1);
    double              ms = time.read();
    fgout << fgnl << "smoothFloat time: " << ms;
    imgDisplay(img);
}

// AUTOMATIC:

void
composite(const CLArgs &)
{
    Ustring            dd = dataDir();
    ImgC4UC         overlay = imgLoadAnyFormat(dd+"base/Teeth512.png"),
                        base = imgLoadAnyFormat(dd+"base/Mandrill512.png");
    fgRegress(fgComposite(overlay,base),dd+"base/test/imgops/composite.png");
}

void
testConvolve(const CLArgs &)
{
    randSeedRepeatable();
    ImgF          tst(16,16);
    for (size_t ii=0; ii<tst.numPixels(); ++ii)
        tst[ii] = float(randUniform());
    ImgF          i0,i1;
    fgSmoothFloat(tst,i0,1);
    fgConvolveFloat(tst,Mat33F(1,2,1,2,4,2,1,2,1)/16.0f,i1,1);
    //fgout << fgnl << i0.m_data << fgnl << i1.m_data;
    FGASSERT(fgApproxEqual(i0.m_data,i1.m_data));
}

}

void
fgImageTestm(const CLArgs & args)
{
    vector<Cmd>   cmds;
    cmds.push_back(Cmd(resize,"resize"));
    cmds.push_back(Cmd(display,"display"));
    cmds.push_back(Cmd(sfs,"sfs","smoothFloat speed"));
    doMenu(args,cmds);
}

void    fgImgTestWrite(const CLArgs &);

void
fgImageTest(const CLArgs & args)
{
    vector<Cmd>       cmds;
    cmds.push_back(Cmd(composite,"composite"));
    cmds.push_back(Cmd(testConvolve,"conv"));
    cmds.push_back(Cmd(fgImgTestWrite,"write"));
    doMenu(args,cmds,true,false,true);
}

}
