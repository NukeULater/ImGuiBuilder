#pragma once

#include "ComVar.h"
#include "CommandHandler.h"

namespace CommandCollection
{
	extern std::vector<ConsoleCommand*> commandTable;

	void InsertCommand(ConsoleCommand* newCommand);
	ConsoleVarCommand* GetVarCommandByName(const std::string& name);
	void SetVarCommandPtr(const std::string& name, ComVar* varPtr);
	void InitializeCommandsMap();

	// commands
	int HelpCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData cbData);

	int BoolVarHandlerCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData cbData);
}
