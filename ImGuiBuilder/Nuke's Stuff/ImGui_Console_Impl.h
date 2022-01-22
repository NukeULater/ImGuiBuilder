#pragma once

#include "imgui.h"
#include "CommandHandler.h"

#define MAX_CONSOLE_INPUT_BUFFER 256

class ImGui_Console2
{
private:
    // variables
    bool                                auto_scroll;
    bool                                scroll_to_botom;

    char                                input_buffer[MAX_CONSOLE_INPUT_BUFFER];
    ImGui_StringBuffer                  output;
    // ImGui_StringBuffer                  completion;
    ImGuiTextInputCompletion*           completion;
    int                                 history_string_index;
    ImGuiTextFilter                     filter;

    static int TextEditCallback(ImGuiInputTextCallbackData* data);

    void ExecCommand(const char* command_line, size_t command_line_length);

    void ClearOutput();

public:
    float                           console_opacity;

    ImGui_Console2();

    int Output(StringHeaderFlags flags, const char* fmt);
    int OutputFmt(StringHeaderFlags flags, const char* fmt, ...);

    void Draw(const char* title, bool* p_open);

    void AllocateCompletionCandidatesBuf(unsigned int candidates_count);
    void DiscardCompletionCandidatesBuf();

    bool CompletionAvailable()
    {
        return completion != NULL ? true : false;
    };

    unsigned int GetCompletionCandidatesCount() {
        if (!CompletionAvailable()) return 0;
        return completion->Count;
    };

    // commands callbacks
    static int help_cb(const char* command_line, size_t command_line_length, void* context);
    static int clear_cb(const char* command_line, size_t command_line_length, void* context);

    static CommandVar ImGui_Console2::get_opacity_var_cb(void* context);
    static int set_opacity_cb(const char* command_line, size_t command_line_length, void* context);
};

void ImGui_Console2_OpenDefault(const char*, bool*);
