#include "ClangFormat.h"

#include <cbeditor.h>
#include <cbproject.h>
#include <cbstyledtextctrl.h>
#include <editorbase.h>
#include <editormanager.h>
#include <logmanager.h>
#include <sdk.h>
#include <sdk_events.h>
#include <tinyxml2.h>
#include <wx/wxscintilla.h>

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
// Register the plugin with Code::Blocks.
PluginRegistrant<ClangFormat> reg(_T("ClangFormat"));

//! @brief  This function executes a shell command and captures stdout.
//! @param[in]  cmd  Shell command to be executed.
//! @return  std::pair<int, std::string>  Pair containing the command's exit status and the captured stdout.
std::pair<int, std::string> executeShellCommand(std::string_view cmd)
{
#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

    std::array<char, 2048> buffer;
    int exit_code = -1;
    std::string result;

    { // scope unique ptr to force pclose before final return statement.
        auto closeAndCapture = [&exit_code](auto handle) { exit_code = PCLOSE(handle); };
        std::unique_ptr<FILE, decltype(closeAndCapture)> pipe(POPEN(cmd.data(), "r"), closeAndCapture);
        if (!pipe)
        {
            return std::make_pair(exit_code, "no_pipe");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
    }
    return std::make_pair(exit_code, result);
}
} // namespace

ClangFormat::ClangFormat() {}

ClangFormat::~ClangFormat() {}

void ClangFormat::OnAttach()
{
    // Assure clang-format is available on the system.
    // to do: make clang-format location configurable.
    auto [status, output] = executeShellCommand("which clang-format");
    if (status == 0)
    {
        // clang-format is triggered on each save event. This means that the formatting is also applied before building.
        Manager::Get()
            ->RegisterEventSink(cbEVT_EDITOR_SAVE,
                                new cbEventFunctor<ClangFormat, CodeBlocksEvent>(this, &ClangFormat::OnEditorSaved));

        Manager::Get()->GetLogManager()->Log(_("ClangFormat: Plugin was attached."));
    }
    else
    {
        Manager::Get()->GetLogManager()->Log(
            _("ClangFormat: No clang-format executable detected: \" " + output + " \""));
    }
}

void ClangFormat::OnRelease(bool appShutDown) {}

void ClangFormat::OnEditorSaved(CodeBlocksEvent& event)
{
    auto project = event.GetProject();
    if ((!project) || (!project->IsLoaded()))
        return;

    // assure the project contains a .clang-format file.
    auto clangFormatFile = event.GetProject()->GetFileByFilename(".clang-format");
    if (!clangFormatFile)
        return;

    // only handle the build-in editor.
    auto baseEditor = event.GetEditor();
    if ((!baseEditor) || (!baseEditor->IsBuiltinEditor()))
        return;

    auto editor = Manager::Get()->GetEditorManager()->GetBuiltinEditor(baseEditor);
    if (!editor)
        return;

    auto control = editor->GetControl();
    if (!control)
        return;

    auto filename = event.GetEditor()->GetFilename();
    if (!filename)
        return;

    // only format C/C++ sources.
    // to do: make matched file types configurable.
    bool matched = false;
    for (auto extension : {".cpp", ".cxx", ".c", ".cc", ".hpp", ".hxx", ".h"})
    {
        if (filename.EndsWith(extension))
        {
            matched = true;
            break;
        }
    }

    if (!matched)
        return;

    // execute clang-format in the base dir.
    auto toplevelPath = event.GetProject()->GetCommonTopLevelPath();

    // clang-format is executed with the XML-output option so that targetted changes can be extracted from its output.
    auto [status, output] =
        executeShellCommand("cd " + std::string{toplevelPath} + ";clang-format --output-replacements-xml -style=file " +
                            std::string{filename} + " 2>&1"); // also catch stderr.
    if (status != 0)
    {
        Manager::Get()->GetLogManager()->Log(
            _("ClangFormat: clang-format exited with statuscode " + std::to_string(status) + ": " + output));
        return;
    }

    // we're using tinyxml2, as cb's default choice of an outdated tinyxml can't handle whitespace very well.
    // use pedantic whitespace to assure all whitespace characters are preserved in the replacements.
    tinyxml2::XMLDocument doc{true, tinyxml2::PEDANTIC_WHITESPACE};
    doc.Parse(output.data());
    auto replacementsNode = doc.FirstChildElement("replacements");
    if (!replacementsNode)
        return;

    // iterate and apply in reverse order to assure offsets remain correct.
    for (auto element = replacementsNode->LastChildElement("replacement"); element;
         element = element->PreviousSiblingElement("replacement"))
    {
        auto offsetAttr = element->Attribute("offset");
        auto lengthAttr = element->Attribute("length");

        if ((!offsetAttr) || (!lengthAttr))
        {
            Manager::Get()->GetLogManager()->Log(
                _("ClangFormat: Could not extract offset and length from replacement."));
            break;
        }

        auto valuePtr = element->GetText();
        std::string value{valuePtr ? valuePtr : ""};

        try
        {
            auto offset = std::stoll(offsetAttr);
            auto length = std::stoll(lengthAttr);

            if (length == 0)
            {
                control->InsertText(offset, value);
            }
            else
            {
                control->Replace(offset, offset + length, value);
            }
        }
        catch (const std::exception& e)
        {
            Manager::Get()->GetLogManager()->Log(_("ClangFormat: Could not convert offset ('" +
                                                   std::string{offsetAttr} + "') and/or length ('" +
                                                   std::string{lengthAttr} + "') in replacement."));
        }
    }

    // apply all changes.
    editor->Save();
}
