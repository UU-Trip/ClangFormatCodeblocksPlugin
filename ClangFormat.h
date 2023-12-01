/***************************************************************
 * Name:      ClangFormatPlugin
 * Purpose:   Code::Blocks plugin
 * Author:     ()
 * Created:   2023-11-28
 * Copyright:
 * License:   GPL
 **************************************************************/

#ifndef CLANGFORMAT_H_INCLUDED
#define CLANGFORMAT_H_INCLUDED

// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <cbplugin.h> // for "class cbPlugin"

class ClangFormat : public cbPlugin
{
public:
    // Default generated methods:
    ClangFormat();
    virtual ~ClangFormat();
    virtual void BuildMenu(wxMenuBar* menuBar) {}
    virtual void BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data = 0) {}
    virtual bool BuildToolBar(wxToolBar* toolBar) { return false; }

protected:
    virtual void OnAttach();
    virtual void OnRelease(bool appShutDown);

private:
    // Formatting occurs on each save event.
    void OnEditorSaved(CodeBlocksEvent& event);
};

#endif // CLANGFORMAT_H_INCLUDED
