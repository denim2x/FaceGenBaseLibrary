//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgDataflow.hpp"
#include "FgCommand.hpp"

using namespace std;

namespace Fg {

DfgInput::~DfgInput()
{
    // boost serialization doesn't work properly in an exception and we shouldn't call another
    // function that could throw (ie. the file save) in this context anyway:
    if (!uncaught_exception()) {
        if (!data.empty() && onDestruct) {
            try {onDestruct(data);}
            catch (...) {}      // Destructors cannot throw
        }
    }
}

const boost::any &
DfgInput::getDataCref() const
{
    if (data.empty())
        fgThrow("DfgInput data not allocated",signature(data));
    return data;
}

void
DfgInput::addSink(const DfgDPtr & snk)
{sinks.push_back(snk); }

void
DfgInput::makeDirty() const
{
//fgout << fgnl << "Dirty DfgInput: " << signature(data) << fgpush;
    for (const DfgDPtr & snk : sinks)
        // Structure is dynamic so some of the sinks may have expired (all cannot be or this node won't exist):
        if (!snk.expired())
            snk.lock()->markDirty();
//fgout << fgpop;
}

boost::any &
DfgInput::getDataRef() const
{
    makeDirty();
    return data;
}

void
DfgInput::setToDefault() const
{
    if (!dataDefault.empty()) {
        data = dataDefault;     // Might re-allocate but what can you do
        makeDirty();
    }
}

// The update algorithm does not need to track visited nodes to avoid exponential re-visiting
// for highly connected graphs because the dirty bit already handles that:
void
DfgOutput::update() const
{
    if (!dirty)
        return;
    // Change flag here because we want to mark clean even if there is an exception so that we
    // don't keep throwing the same exception:
    dirty = false;
//fgout << fgnl << "Update DfgOutput: " << signature(data) << fgpush;
    for (const DfgNPtr & src : sources)
        src->update();      // Ensure sources updated
//fgout << fgpop;
    try {
        func(sources,data);
    }
    catch(FgException & e)
    {
        e.pushMsg("Executing DfgOutput link",signature(data));
        throw;
    }
    catch(std::exception const & e)
    {
        FgException     ex("std::exception",e.what());
        ex.pushMsg("Executing DfgOutput link",signature(data));
        throw ex;
    }
    catch(...)
    {
        fgThrow("Unknown exception executing DfgOutput link",signature(data));
    }
}
void
DfgOutput::markDirty() const
{
    // If an DfgOutput is dirty, all of its dependents must be dirty too since markDirty() marks all
    // dependents and update() on any one of them would have forced an update on this one:
    if (dirty)
        return;
    dirty = true;
//fgout << fgnl << "Dirty DfgOutput: " << signature(data) << fgpush;
    for (const DfgDPtr & snk : sinks)
        // Structure is dynamic so some of the sinks may have expired (all cannot be or this node won't exist):
        if (!snk.expired())
            snk.lock()->markDirty();
//fgout << fgpop;
}

const boost::any &
DfgOutput::getDataCref() const
{
    update();
    return data;
}
void
DfgOutput::addSource(const DfgNPtr & src)
{
    sources.push_back(src);
    markDirty();
}
void
DfgOutput::addSink(const DfgDPtr & snk)
{
    sinks.push_back(snk);
}

void DfgReceptor::update() const
{
    FGASSERT(src);
    if (!dirty)
        return;
//fgout << fgnl << "Update DfgReceptor:" << fgpush;
    dirty = false;
    src->update();
//fgout << fgpop;
}
void DfgReceptor::markDirty() const
{
    if (dirty)
        return;
    dirty = true;
//fgout << fgnl << "Dirty DfgReceptor:" << fgpush;
    for (const DfgDPtr & snk : sinks)
        if (!snk.expired())
            snk.lock()->markDirty();
//fgout << fgpop;
}
const boost::any & DfgReceptor::getDataCref() const
{
    update();
    return src->getDataCref();
}
void DfgReceptor::addSink(const DfgDPtr & snk)
{
    sinks.push_back(snk);
}

void DfgReceptor::setSource(DfgNPtr const & nptr)
{
    FGASSERT(!src);
    src = nptr;
}

void
DirtyFlag::markDirty() const
{
//fgout << fgnl << "Dirty Flag";
    dirty = true;
}
bool
DirtyFlag::checkUpdate() const
{
    if (dirty) {
//fgout << fgnl << "Update Flag:" << fgpush;
        // Update flag first in case sources throw an exception - avoids repeated throws:
        dirty = false;
        for (const DfgNPtr & src : sources)
            src->update();
//fgout << fgpop;
        return true;
    }
    else
        return false;
}

void
addLink(const DfgNPtr & src,const DfgOPtr & snk)
{
    src->addSink(snk);
    snk->addSource(src);
}

DfgFPtr
makeUpdateFlag(const DfgNPtrs & nptrs)
{
    DfgFPtr        ret = std::make_shared<DirtyFlag>(nptrs);
    for (const DfgNPtr & nptr : nptrs)
        nptr->addSink(ret);
    return ret;
}

void setInputsToDefault(const DfgNPtrs & nptrs)
{
    for (const DfgNPtr & nptr : nptrs) {
        if (const DfgInput * iptr = dynamic_cast<const DfgInput*>(nptr.get()))
            iptr->setToDefault();
        else if (const DfgOutput *optr = dynamic_cast<const DfgOutput*>(nptr.get()))
            setInputsToDefault(optr->sources);
        else if (const DirtyFlag *dptr = dynamic_cast<const DirtyFlag*>(nptr.get()))
            setInputsToDefault(dptr->sources);
        else
            fgThrow("setInputsToDefault unhandled type");
    }
}

void
fgCmdTestDfg(const CLArgs &)
{
    IPT<int>        n0 = makeIPT(5),
                    n1 = makeIPT(6);
    NPT<int>        n2 = linkN<int,int>(fgSvec<NPT<int> >(n0,n1),cSum<int>);
    FGASSERT(n2.cref() == 11);
    n0.ref() = 6;
    FGASSERT(n2.val() == 12);
    NPT<int>        n3 = link1<int,int>(n2,[](int x){return x+2;});
    FGASSERT(n3.val() == 14);
}

// Old code for turning DAG into DOT into PDF:

//string
//fgDepGraph2Dot(
//    const ... lg,
//    const string &                          label,
//    const vector<uint> &                    paramInds)
//{
//    ostringstream    ret;
//    ret << "digraph DepGraph\n{\n";
//    ret << "  graph [label=\"" << label << "\"];\n  {\n    node [shape=box]\n";
//    for (size_t ii=0; ii<paramInds.size(); ++ii)
//        ret << "    \"" << lg.nodeData(paramInds[ii]).name(paramInds[ii]) << "\" [shape=doubleoctagon]\n";
//    for (uint ii=0; ii<lg.numLinks(); ii++) {
//        vector<uint>    sources = lg.linkSources(ii);
//        vector<uint>    sinks = lg.linkSinks(ii);
//        // Skip stubs (used by GUI) as they obsfucate:
//        if ((sinks.size() == 1) && (lg.nodeData(sinks[0]).label == "stub"))
//            continue;
//        ret << "    L" << ii << " [shape=oval];\n";
//        for (uint jj=0; jj<sources.size(); jj++)
//            ret << "    \"" << lg.nodeData(sources[jj]).name(sources[jj]) 
//                << "\" -> L" << ii << ";\n";
//        for (uint jj=0; jj<sinks.size(); jj++)
//            ret << "    L" << ii << " -> \""
//                << lg.nodeData(sinks[jj]).name(sinks[jj]) << "\";\n";
//    }
//    ret << "  }\n";
//    ret << "}\n";
//    return ret.str();
//}
//
//void
//fgDotToPdf(
//    const std::string &     dotFile,
//    const std::string &     pdfFile)
//{
//    // Uses 'dot.exe' from Graphviz (2.38 as of last test use):
//    string      cmd = "dot -Tpdf -o" + pdfFile + " " + dotFile;
//#ifndef FG_SANDBOX
//    if (system(cmd.c_str()) != 0)
//        fgout << fgnl << "Command failed.";
//#endif
//}
//
//void
//fgDepGraph2Pdf(
//    const ...  lg,
//    const std::string &                     rootName,
//    const vector<uint> &                    paramInds)
//{
//    Ofstream      ofs(rootName+".dot");
//    ofs << fgDepGraph2Dot(lg,"",paramInds);
//    ofs.close();
//    fgDotToPdf(rootName+".dot",rootName+".pdf");
//}

}

// */
