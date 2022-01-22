#include "pch.h"

#include "CommandHandler.h"

std::unordered_set<void*> command_table;

ImGui_ConsoleCommand::ImGui_ConsoleCommand(const char* _name, const char* _command_description, int _parameter_count, ImGui_ExecuteCommandCallback* _callback,
    CommandFlags _flags)
{
    flags = _flags;
    name = _name;
    command_description = _command_description;
    parameter_count = _parameter_count;
    p_execute_command_callback = _callback;
    user_data = NULL;
}

bool ImGui_ConsoleCommand::HandleCommandLine(const char* command_line, size_t command_line_length, void* context)
{
    bool ret = false;

    std::vector<std::string> command_first_tokens;
    if (tokenize(command_line, command_line_length, " ", command_first_tokens, 2))
    {
        ImGui_ConsoleCommand* command_to_execute = NULL;
        for (auto& command_ptr : command_table)
        {
            ImGui_ConsoleCommand* command = (ImGui_ConsoleCommand*)command_ptr;
            if (strnicmp(command->GetName(), command_first_tokens.at(0).c_str(), strlen(command->GetName())) == 0)
            {
                command_to_execute = command;
            }
        }

        if (command_to_execute != NULL)
        {
            command_to_execute->ExecuteCommand(command_line, command_line_length, context);
            ret = true;
        }
    }

    return ret;
}

void ImGui_ConsoleCommand::ExecuteCommand(const char* command_line, size_t command_line_length, void* context) const
{
    // create a copy of the current command, then set the user data
    // this should allow using this this command in multiple threads if needed

    // quite ugly here, might confuse some ppl when they see "context"
    if (CommandSetsVariable())
    {
        // we do this because we allocate more memory with ImGui_VarCommand
        // maybe find cleaner way?
        ImGui_ConsoleVarCommand* varCommand = (ImGui_ConsoleVarCommand*)this;
        ImGui_ConsoleVarCommand current_command(*varCommand);
        current_command.user_data = context;
        p_execute_command_callback(command_line, command_line_length, &current_command);
    }
    else
    {
        ImGui_ConsoleCommand current_command(*this);
        current_command.user_data = context;
        p_execute_command_callback(command_line, command_line_length, &current_command);
    }
}

CommandVar ImGui_ConsoleVarCommand::queryVariable(void* context)
{
    return p_query_variable_callback(context);
}

ImGui_ConsoleVarCommand::ImGui_ConsoleVarCommand(const char* _name, const char* _command_description, int _parameter_count, ImGui_QueryVarCallback* _query_var_callback,
    ImGui_ExecuteCommandCallback* _callback, CommandFlags _flags) :
    ImGui_ConsoleCommand(_name, _command_description, _parameter_count, _callback, flags | CommandFlag_SetsVariable)
{
    p_query_variable_callback = _query_var_callback;
}

// externs
// just create functions with _Initialize_Command_List added to the file's name as the name function
// and add them here
extern void ImGui_Console_impl_Initialize_Command_List();

void InitializeCommandsMap()
{
    static bool InitializeCommandsMap_initialized = false;
    if (InitializeCommandsMap_initialized) return;
    InitializeCommandsMap_initialized = true;

    ImGui_Console_impl_Initialize_Command_List();

    atexit([]() -> void {
        for (auto& command : command_table)
        {

        }

        command_table.clear();
        }
    );
}