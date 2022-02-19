#include "pch.h"

#include "CommandCollection.h"
#include "ImGuiConsoleImpl.h"

std::mutex commandInsertMtx;

bool readObjectIds = true;
std::map<std::string, unsigned int> objectIds;

const char command_error_bad_arg[] = "# exception catch (bad arg): ";

// don't forget to add '_cmd' after the name, 
// if you add a variable command created using `DECL_ComVarCommandPtr` macro
std::vector<ConsoleCommand*> CommandCollection::commandTable = {
	new ConsoleCommand("help", "outputs all commands, 0 - 1 parameter(s): <string>(optional): command name", 0, 1, CommandCollection::HelpCmd),
};

void CommandCollection::InitializeCommandsMap()
{
	static bool InitializeCommandsMap_initialized = false;
	if (InitializeCommandsMap_initialized) return;
	InitializeCommandsMap_initialized = true;

	atexit([]() -> void {
		for (auto command : commandTable)
		{
		}

		commandTable.clear();
		}
	);
}

void CommandCollection::InsertCommand(ConsoleCommand* newCommand)
{
	std::scoped_lock(commandInsertMtx);

	for (auto command : commandTable)
	{
		if (!strcmp(newCommand->GetName(), command->GetName()))
		{
			printf("%s - command %s already present!", __FUNCTION__, newCommand->GetName());
			return;
		}
	}

	commandTable.emplace_back(newCommand);
}

ConsoleVarCommand* CommandCollection::GetVarCommandByName(const std::string& name)
{
	std::scoped_lock(commandInsertMtx);

	for (auto command : commandTable)
	{
		if (!strcmp(name.c_str(), command->GetName()))
			return dynamic_cast<ConsoleVarCommand*>(command);
	}

	return nullptr;
}

// in case your variable needs to be set/updated
void CommandCollection::SetVarCommandPtr(const std::string& name, ComVar* varPtr)
{
	ConsoleVarCommand* varCmdPtr = GetVarCommandByName(name);
	if (varCmdPtr != nullptr)
	{
		varCmdPtr->UpdateVarPtr(varPtr);
	}
}

//////////////////////////////////////////////////////////////////////////
//	commands
//////////////////////////////////////////////////////////////////////////

int CommandCollection::BoolVarHandlerCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData cbData)
{
	IOutput* output = (IOutput*)cbData.strOutput;
	auto var = reinterpret_cast<ComVarTPtr<bool*>*>(cbData.commandVar);

	std::string exception;
	if (!var->SetValFromStr(tokens[1], 10, exception))
	{
		output->Output(StringFlag_None, command_error_bad_arg);
		output->OutputFmt(StringFlag_None, "	%s", exception.c_str());
	}
	return 0;
}

int CommandCollection::HelpCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData cbData)
{
	IOutput* output = cbData.strOutput;
	const ConsoleCommand* command_data = cbData.consoleCommandData;

	const std::string* commandToHelp = nullptr;

	int parameterCount = tokens.size() - 1; // only parameters
	bool singleCommandHelp = parameterCount >= 1;
	bool singleCommandHelpFound = false;

	if (singleCommandHelp)
		commandToHelp = &tokens[1];

	if (!singleCommandHelp)
		output->Output(StringFlag_None, "# available commands: ");

	for (auto command_entry : CommandCollection::commandTable)
	{
		if (singleCommandHelp && _strnicmp(command_entry->GetName(), commandToHelp->c_str(), commandToHelp->length()) != 0)
			continue;

		if (!command_entry->Hidden())
		{
			output->OutputFmt(StringFlag_None, "# %s ", command_entry->GetName());
			if (command_entry->GetDescription() != NULL)
			{
				output->OutputFmt(StringFlag_None, "    # command description: %s", command_entry->GetDescription());
			}
			else
			{
				output->OutputFmt(StringFlag_None, "	# command has no description");
			}

			if (singleCommandHelp)
			{
				singleCommandHelpFound = true;
				break;
			}
		}
	}

	if (singleCommandHelp && !singleCommandHelpFound)
		output->OutputFmt(StringFlag_None, "	# unknown command: %s", commandToHelp->c_str());

	return 0;
}

//////////////////////////////////////////////////////////////////////////
//	commands end
//////////////////////////////////////////////////////////////////////////