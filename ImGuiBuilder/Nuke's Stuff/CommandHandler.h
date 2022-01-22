#pragma once

#include "CommandsUtil.h"

enum CommandType
{
    CommandType_SetVar = 0,  // updates a variable, maybe execute a command
    CommandType_ExecCommand, // executes a command
};

class CommandVar
{
public:
    enum ComVarType : int
    {
        // return Var_Invalid if the pointer to the data is not valid
        ComVar_Invalid = -1,
        ComVar_Unknown,
        ComVar_Int32,
        ComVar_UInt32,
        ComVar_Int64,
        ComVar_Float,
        ComVar_Double,
        ComVar_String,

        ComVar_End
    };

    // TODO when implementing command suggestions:
    void IntToString(char* buf, size_t buf_size) const
    {
        switch (this->var_type)
        {
        case ComVar_Invalid:
        case ComVar_Unknown:
            assert(0);
            break;
            //snprintf(buf, buf_size, "<UNKNOWN>");
            //break;
        case ComVar_Int32:
            snprintf(buf, buf_size, "%d");
            break;
        case ComVar_Int64:
            snprintf(buf, buf_size, "%lld");
            break;
        
        case ComVar_End:
        default:
            break;
        }
    }

    void FloatToString(char* buf, size_t buf_size, const char* precision = "%.3f") const
    {
        switch (this->var_type)
        {
        case ComVar_Invalid:
        case ComVar_Unknown:
            assert(0);
            break;
            // strcpy_s(buf, buf_size, "<UNKNOWN>");
            // break;
        case ComVar_Float:
            snprintf(buf, buf_size, precision);
            break;
        case ComVar_Double:
            snprintf(buf, buf_size, precision);
            break;
        case ComVar_End:
        default:
            break;
        }
    }

    void AsString(char* buf, size_t buf_size) const
    {
        switch (this->var_type)
        {
        case ComVar_Invalid:
        case ComVar_Unknown:
            assert(0);
            break;
            // strcpy_s(buf, buf_size, "<UNKNOWN>");
            // break;
        case ComVar_String:
            strncpy_s(buf, buf_size, Get<char*>(), buf_size - 1);
        case ComVar_End:
        default:
            break;
        }
    }

    template<typename T> void Set(const T val) { if (ptr != nullptr) *(T*)ptr = val; };
    template<typename T> T Get() const { if (ptr != nullptr) return *(T*)ptr; };

    void* ptr;
    ComVarType var_type;
};

// change name, this can be used to execute the command without the console
typedef int(ImGui_ExecuteCommandCallback)(const char* command_line, size_t command_line_length, void* context);
typedef int(ImGui_CommandCallbackSetVar)(const char* command_line, size_t command_line_length, void* context);
typedef CommandVar(ImGui_QueryVarCallback)(void* context);

typedef int CommandFlags;

enum CommandFlags_
{
    CommandFlag_None = 0,
    CommandFlag_Hidden = 1 << 0, // will not display in help commands or anywhere else where this flag is tested
    CommandFlag_SetsVariable = 1 << 1  // internal, do not set by yourself, but you can test it
};

class ImGui_ConsoleCommand
{
public:
    virtual bool CommandSetsVariable() const { return (flags & CommandFlag_SetsVariable) != 0;  }

    const char* GetName() const
    {
        return name;
    }

    const char* GetDescription() const
    {
        return command_description;
    }

    int GetParameterCount() const
    {
        return parameter_count;
    }

    template<typename T = void> T GetUserData() const 
    { 
        return (T)user_data; 
    }

    CommandFlags GetFlags() const
    {
        return flags;
    }

    ImGui_ConsoleCommand(const char* _name, const char* _command_description, int _parameter_count, ImGui_ExecuteCommandCallback* _callback,
        CommandFlags _flags = CommandFlag_None);

    // executes the actual command
    void ExecuteCommand(const char* command_line, size_t command_line_length, void* context) const;

    // handles command line
    // returns true if command line has been handled
    static bool HandleCommandLine(const char* command_line, size_t command_line_length, void* context);

protected:
    CommandFlags flags;
    const char* name;
    const char* command_description;
    int parameter_count;
    void* user_data;

private:
    ImGui_ExecuteCommandCallback* p_execute_command_callback;
};

// this is needed because we want to display the value inside the 
// and display it in 
class ImGui_ConsoleVarCommand : public ImGui_ConsoleCommand
{
public:
    ImGui_ConsoleVarCommand(const char* _name, const char* _command_description, int _parameter_count, ImGui_QueryVarCallback* _query_var_callback, 
        ImGui_ExecuteCommandCallback* _callback, CommandFlags _flags = CommandFlag_None);

    CommandVar queryVariable(void* context);

private:
    ImGui_QueryVarCallback* p_query_variable_callback;
};

void InitializeCommandsMap();

extern std::unordered_set<void*> command_table;


