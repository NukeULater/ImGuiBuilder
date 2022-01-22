
#include "pch.h"
#include "ImGui_Console_Impl.h"

#include <rapidfuzz/fuzz.hpp>
#include <rapidfuzz/utils.hpp>

#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

extern bool draw_real_demo_window;

const char* command_error_bad_arg = "# exception catch (bad arg): ";
const char* command_error_not_enough_params = "# %s command error: invalid parameter count";

ImGui_Console2* GetMainConsoleInstance()
{
	static std::unique_ptr<ImGui_Console2> console(std::make_unique<ImGui_Console2>());
	return console.get();
}

ImGui_Console2::ImGui_Console2() :
	output(256, MAX_CONSOLE_INPUT_BUFFER) // output constructor 256 lines of 256 bytes each
	//completion(32, 64)
{
	console_opacity = 0.90f;
	auto_scroll = true;
	scroll_to_botom = false;
	history_string_index = -1;
	completion = NULL;
	memset(input_buffer, 0, sizeof(input_buffer));
}

// TODO: replace output functions with these
int ImGui_Console2::Output(StringHeaderFlags flags, const char* fmt)
{
	return 0;
}

int ImGui_Console2::OutputFmt(StringHeaderFlags flags, const char* fmt, ...)
{
	return 0;
}

bool InputTextContainsCommandSubstring(const char* command, const char* input_text, bool test_only_first_token)
{
	char input_text_upper_temp[MAX_CONSOLE_INPUT_BUFFER];
	char comand_text_upper[MAX_CONSOLE_INPUT_BUFFER];

	strncpy(input_text_upper_temp, input_text, MAX_CONSOLE_INPUT_BUFFER - 1);
	input_text_upper_temp[MAX_CONSOLE_INPUT_BUFFER - 1] = '\0';

	strncpy(comand_text_upper, command, MAX_CONSOLE_INPUT_BUFFER - 1);
	comand_text_upper[MAX_CONSOLE_INPUT_BUFFER - 1] = '\0';

	strupr(input_text_upper_temp);
	strupr(comand_text_upper);

	char* next_token_to_test = NULL;
	char* text_to_test = input_text_upper_temp;

	while (text_to_test != NULL)
	{
		// first we get the next token to test
		// and remove the spaces in the buffer
		char* input_text_space = strstr(text_to_test, " ");
		if (input_text_space == NULL)
		{
			next_token_to_test = NULL;
		}
		else
		{
			// remove the space and test the next string after the space
			*input_text_space = '\0';
			input_text_space++;
			next_token_to_test = input_text_space;
		}

		if (strstr(comand_text_upper, text_to_test) != NULL)
			return true;

		if (test_only_first_token)
			return false;

		// at the end set the next substring to test from the input, if we have spaces in it
		text_to_test = next_token_to_test;

		// check if there's nothing after the space, if so return false, since we didn't find anything close
		if ((text_to_test - input_text_upper_temp) >= MAX_CONSOLE_INPUT_BUFFER)
		{
			return false;
		}
	}

	// if we got this far, no match was found
	return false;
}

// ImGui text callback
int ImGui_Console2::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	ImGui_Console2* console_data = (ImGui_Console2*)data->UserData;

	// first update the state of command completion
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackAlways:
	case ImGuiInputTextFlags_CallbackEdit:
	case ImGuiInputTextFlags_CallbackHistory:
	case ImGuiInputTextFlags_CallbackCompletion:
		if (data->BufTextLen > 0
			&& console_data->CompletionAvailable()
			&& console_data->GetCompletionCandidatesCount() > 0
			&& console_data->history_string_index == -1)
		{
			data->CompletionData = console_data->completion;
		}
		break;
	}

	// then handle the event
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackAlways:
		// every frame we pass the cached completion candidates
		break;

	case ImGuiInputTextFlags_CallbackEdit:
	{
		// don't allow spaces when no other character have been inserted
		if (data->BufTextLen == 1 && data->Buf[data->BufTextLen - 1] == ' ')
		{
			// delete the space character
			data->DeleteChars(data->BufTextLen - 1, 1);
			break;
		}

		// reset the history index if we modified the buffer
		console_data->history_string_index = -1;

		// clear if new character is inserted
		//console_data->completion.Clear();

		if (data->BufTextLen > 0
			&& (data->Buf[data->BufTextLen - 1] == ' '
				|| data->Buf[data->BufTextLen - 1] == '\n'
				|| data->Buf[data->BufTextLen - 1] == '\t')
			|| data->BufTextLen == 0
			)
		{
			break;
		}

		struct CommandCompletionScore
		{
			void* command_ptr;
			// double score;
		};

		// create the completion buffer
		std::vector<CommandCompletionScore> completion_commands;
		for (void* command_ptr : command_table)
		{
			ImGui_ConsoleCommand* command = (ImGui_ConsoleCommand*)command_ptr;

			if (InputTextContainsCommandSubstring(command->GetName(), data->Buf, true))
			{
				completion_commands.push_back(CommandCompletionScore{ command });
			}

			// double score_obtained = rapidfuzz::fuzz::ratio(data->Buf, command->GetName());
			// if (score_obtained > 50.0f)
			// {
			// 	command_complete_scores.push_back(CommandCompletionScore{ command, score_obtained });
			// 	printf("	added: %s with score of %3.f\n", command->GetName(), score_obtained);
			// }
			// else
			// {
			// 	printf("	discarded: %s with score of %3.f\n", command->GetName(), score_obtained);
			// }
		}

		if (completion_commands.size() > 1)
		{
			std::sort(completion_commands.begin(), completion_commands.end(), [](const CommandCompletionScore& a, const CommandCompletionScore& b) -> bool
				{
					ImGui_ConsoleCommand* command1 = (ImGui_ConsoleCommand*)a.command_ptr;
					ImGui_ConsoleCommand* command2 = (ImGui_ConsoleCommand*)b.command_ptr;
					return strcmp(command1->GetName(), command2->GetName()) < 0;
				}
			);
		}

		if (console_data->completion == NULL
			|| (console_data->completion != NULL
				&& completion_commands.size() > console_data->GetCompletionCandidatesCount()))
		{
			// this also clears the old buffer
			console_data->AllocateCompletionCandidatesBuf(completion_commands.size());
		}

		// in case we didn't need to re-allocate memory, set the count and selected index
		console_data->completion->SelectedCandidateIndex = -1;
		console_data->completion->Count = completion_commands.size();

		for (int i = 0; i < completion_commands.size(); i++)
		{
			ImGui_ConsoleCommand* command = (ImGui_ConsoleCommand*)completion_commands[i].command_ptr;
			// console_data->completion.AddString(0, command->GetName());

			memset(&console_data->completion->CompletionCandidate[i], 0, sizeof(ImGuiTextInputCompletionCandidate));
			console_data->completion->CompletionCandidate[i].CompletionText = command->GetName();
		}

		// after we found the candidates, update the callback data with the pointers to the buffer
		data->CompletionData = console_data->completion;
	}
	break;

	case ImGuiInputTextFlags_CallbackHistory:
	{
		// if we have completion data, we do not want to use history
		// unless the current string is actually history
		if (data->BufTextLen > 0
			&& console_data->CompletionAvailable()
			&& console_data->history_string_index == -1)
		{
			const int prev_selected_completion_index = console_data->completion->SelectedCandidateIndex;
			if (data->EventKey == ImGuiKey_UpArrow)
				console_data->completion->SelectedCandidateIndex--;
			else if (data->EventKey == ImGuiKey_DownArrow)
				console_data->completion->SelectedCandidateIndex++;

			// clamp between -1 and string header size - 1
			console_data->completion->SelectedCandidateIndex = IM_CLAMP(console_data->completion->SelectedCandidateIndex, -1, (int)(console_data->GetCompletionCandidatesCount() - 1));
		}
		// check if we actually have something to display first
		else if (console_data->output.GetStringHeaderSize() > 0)
		{
			const int prev_history_index = console_data->history_string_index;
			if (data->EventKey == ImGuiKey_UpArrow)
				console_data->history_string_index++;
			else if (data->EventKey == ImGuiKey_DownArrow)
				console_data->history_string_index--;

			// clamp between -1 and string header size - 1
			console_data->history_string_index = IM_CLAMP(console_data->history_string_index, -1, (int)(console_data->output.GetStringHeaderSize() - 1));

			// TODO cleanup and maybe? simplify
			// wrote this at ~ 1 am and hackily fixed the bugs the next day
			if (prev_history_index != console_data->history_string_index)
			{
				if (console_data->history_string_index >= 0)
				{
					const char* new_text_box_from_history = NULL;
					for (int i = (int)console_data->output.GetStringHeaderSize() - 1; i >= 0; i--)
					{
						auto& string_header = console_data->output.GetHeader(i);
						int string_header_index_to_history_index = (int)console_data->output.GetStringHeaderSize() - 1 - i;
						if (string_header.flags & StringFlag_History)
						{
							if ((prev_history_index == -1 && console_data->history_string_index > prev_history_index)
								|| (console_data->history_string_index > prev_history_index && prev_history_index < string_header_index_to_history_index)
								|| (console_data->history_string_index < prev_history_index && prev_history_index > string_header_index_to_history_index)
								)
							{
								new_text_box_from_history = console_data->output.GetStringAtIndex(i);
								console_data->history_string_index = string_header_index_to_history_index;

								// if we are scrolling back down, don't stop at the first string we find
								if ((prev_history_index == -1 && console_data->history_string_index > prev_history_index)
									|| (console_data->history_string_index > prev_history_index && prev_history_index < string_header_index_to_history_index))
								{
									break;
								}
							}
							// if there's just one history string available, and we hit the button down
							// clear the text from the console
							// instead of clearing it everytime
							else if (i == 0 && prev_history_index == string_header_index_to_history_index)
							{
								data->DeleteChars(0, data->BufTextLen);
							}
						}
						// if there's no other string marked as history when using up key, just reset the index and then break
						// so we don't actually increase it for no reason
						else if (i == 0)
						{
							if (console_data->history_string_index == string_header_index_to_history_index)
							{
								console_data->history_string_index = prev_history_index;
							}
						}
					}

					// if history text is NULL, it means we didn't find any hostory inside the output buffer
					if (new_text_box_from_history != NULL)
					{
						data->DeleteChars(0, data->BufTextLen);
						data->InsertChars(0, new_text_box_from_history);
					}
				}
				else
				{
					data->DeleteChars(0, data->BufTextLen);
				}
			}
		}
	}
	break;

	case ImGuiInputTextFlags_CallbackCompletion:
		// here we handle TAB key press
		// we just override the buffer with the command at the current completion index
		if (data->BufTextLen > 0
			&& console_data->CompletionAvailable()
			&& console_data->completion->SelectedCandidateIndex != -1
			&& console_data->history_string_index == -1)
		{
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, console_data->completion->CompletionCandidate[console_data->completion->SelectedCandidateIndex].CompletionText);
			console_data->completion->SelectedCandidateIndex = -1;
		}

		break;


	case ImGuiInputTextFlags_CallbackCharFilter:
	case ImGuiInputTextFlags_CallbackResize:
		break;
	}

	return 0;
}

void ImGui_Console2::ClearOutput() 
{
	output.Clear();
};

void ImGui_Console2::ExecCommand(const char* command_line, size_t command_line_length)
{
	history_string_index = -1;

	if (!ImGui_ConsoleCommand::HandleCommandLine(command_line, command_line_length, this))
	{
		output.AddString(StringFlag_None, "# unknown command: ");
		output.AddString(StringFlag_History, command_line);
		scroll_to_botom = true;
	}
}

void ImGui_Console2::AllocateCompletionCandidatesBuf(unsigned int candidates_count)
{
	DiscardCompletionCandidatesBuf();
	// TODO maybe move this to function for object using dynamic allocated memory
	unsigned int buffer_size = sizeof(ImGuiTextInputCompletion) + sizeof(ImGuiTextInputCompletionCandidate) * candidates_count;
	completion = (ImGuiTextInputCompletion*)new char[buffer_size];
	memset(completion, 0, buffer_size);
	new (completion) ImGuiTextInputCompletion; // in case this will have an constructor
	completion->SelectedCandidateIndex = -1;
	completion->Count = candidates_count;
	// completion candidates are stored contiguously
	completion->CompletionCandidate = (ImGuiTextInputCompletionCandidate*)((char*)completion + sizeof(ImGuiTextInputCompletion));
}

void ImGui_Console2::DiscardCompletionCandidatesBuf()
{
	if (completion != NULL)
	{
		char* buffer_to_delete = (char*)completion; // initially just char buffer
		delete[] buffer_to_delete;
	}
	completion = NULL;
}

void ImGui_Console2::Draw(const char* title, bool* p_open)
{
	if (!*p_open) return;

	static bool  console_window_initialized = false;

	ImGuiIO& io = ImGui::GetIO();
	const ImGuiStyle& style = ImGui::GetStyle();
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowSizeConstraints(
		ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y / 3.0f), 
		ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y / 2.0f)
	);

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver); // in the left upside corner

	ImGuiWindowFlags console_main_window_flags = 0
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoSavedSettings
		// | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_MenuBar
		//| ImGuiWindowFlags_NoScrollbar
		//| ImGuiWindowFlags_NoScrollWithMouse
		//| ImGuiWindowFlags_AlwaysAutoResize
		;

	ImGui::SetNextWindowBgAlpha(console_opacity);

	if (!ImGui::Begin(title, p_open, console_main_window_flags))
	{
		ImGui::End();
		return;
	}

	static bool bring_console_window_to_focus = false;
	bring_console_window_to_focus = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Test1"))
		{
			ImGui::MenuItem("Draw Demo Window", NULL, &draw_real_demo_window);
			if (ImGui::MenuItem("Clear")) { ClearOutput(); }
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// Poput Context Window
	if (ImGui::BeginPopupContextWindow(NULL, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
	{
		if (ImGui::MenuItem("Close Window")) { *p_open = false; }
		ImGui::EndPopup();
	}

	ImGui::Separator();

	// console text window
	ImGuiWindowFlags console_log_child_window_flags = 0
		| ImGuiWindowFlags_HorizontalScrollbar
		;

	//float footer_height_to_reserve = 0.0f;
	float footer_height_to_reserve = (ImGui::GetFrameHeightWithSpacing());
	ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
	ImGui::SetNextWindowBgAlpha(console_opacity);
	ImGui::BeginChild("##consolelog", ImVec2(-1, -footer_height_to_reserve), true, console_log_child_window_flags);
	
	// Child log Poput Context Window
	if (ImGui::BeginPopupContextWindow(NULL, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
	{
		if (output.GetStringHeaderSize() > 0)
			if (ImGui::MenuItem("Clear")) { ClearOutput(); }
		if (ImGui::MenuItem("Close Window")) { *p_open = false; }
		ImGui::EndPopup();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
	for (int i = 0; i < output.GetStringHeaderSize(); i++)
	{
		// TODO add color support
		char console_text_id[256];
		snprintf(console_text_id, ARRAYSIZE(console_text_id), "##consolelogid%d", i);
		ImGui::PushID(console_text_id);

		auto& stringHeader = output.GetHeader(i);
		const char* stringToAdd = output.GetStringAtOffset(stringHeader.offset);
		if (stringHeader.flags & StringFlag_CopyToClipboard)
			ImGui::LogToClipboard();

		ImGui::Text(stringToAdd, stringToAdd + (stringHeader.size - 1));

		if (stringHeader.flags & StringFlag_CopyToClipboard)
		{
			stringHeader.flags &= ~(StringFlag_CopyToClipboard);
			ImGui::LogFinish();
		}

		if (ImGui::BeginPopupContextItem("##copy to clipboard"))
		{
			if (ImGui::MenuItem("Copy to clipboard")) { stringHeader.flags |= StringFlag_CopyToClipboard; }
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}
	ImGui::PopStyleVar();

	if (scroll_to_botom || (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		ImGui::SetScrollHereY(1.0f);
	scroll_to_botom = false;

	ImGui::PopStyleColor();
	ImGui::EndChild();

	static bool reclaim_input_box_focus = false;

	// ImGui::PushItemWidth(-(ImGui::CalcTextSize("Enter").x + style.FramePadding.x * 2.0f + 5.0f + 1.0f));
	ImGuiInputTextFlags input_text_flags = 0
		| ImGuiInputTextFlags_EnterReturnsTrue
		| ImGuiInputTextFlags_AutoSelectAll
		| ImGuiInputTextFlags_CallbackCompletion
		| ImGuiInputTextFlags_CallbackHistory
		| ImGuiInputTextFlags_CallbackEdit
		| ImGuiInputTextFlags_DisplaySuggestions
		| ImGuiInputTextFlags_CallbackAlways
		;

	ImGui::SetNextItemWidth(-(ImGui::CalcTextSize("Enter").x + style.FramePadding.x * 2.0f + 5.0f + 1.0f));
	if (ImGui::InputTextWithHint("##command1", "<command>", input_buffer, IM_ARRAYSIZE(input_buffer), input_text_flags, TextEditCallback, this))
	{
		size_t input_buffer_string_length = strnlen_s(input_buffer, sizeof(input_buffer) - 1);
		if (input_buffer_string_length > 0)
		{
			ExecCommand(input_buffer, input_buffer_string_length);
			memset(input_buffer, 0, 2);
			reclaim_input_box_focus = true;
		}
	}
	bool input_text_active = ImGui::IsItemActive();

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_input_box_focus)
	{
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
		reclaim_input_box_focus = false;
	}

	//ImGui::PopItemWidth();

	// enter button
	ImGui::SameLine(0.0f, 5.0f);
	if (ImGui::Button("Enter"))
	{
		size_t input_buffer_string_length = strnlen_s(input_buffer, sizeof(input_buffer) - 1);
		if (input_buffer_string_length > 0)
		{
			ExecCommand(input_buffer, input_buffer_string_length);
			memset(input_buffer, 0, 2);
		}
	}

	if (input_text_active)
	{
		// Command-line
		// ImGui::Separator();

		ImGuiWindowFlags completion_window_flags = 0
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoFocusOnAppearing
			// | ImGuiWindowFlags_NoResize
			;

		// window cursor positon 
		ImVec2 main_window_last_screen_pos_cursor = ImGui::GetCursorScreenPos(); // will use this to display the command completion candidates

		ImVec2 req_text_input_window_size
			= ImVec2(main_viewport->WorkSize.x - style.WindowPadding.x - style.ItemSpacing.x, 0.0f);

		//ImGui::SetNextWindowSizeConstraints(ImVec2(req_text_input_window_size.x, -1), ImVec2(req_text_input_window_size.x, -1));
		//ImGui::SetNextWindowPos(ImVec2(main_window_last_screen_pos_cursor.x, main_window_last_screen_pos_cursor.y));
		// ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
		// if (ImGui::Begin("###command completion candidates", NULL, completion_window_flags))
		// {
		// 	for (int i = 0; i < 5; i++)
		// 		if (ImGui::Selectable("test completion", true)) {}
		// }
		// ImGui::PopStyleVar();
		//ImGui::End();
	}

	console_window_initialized = true;

	ImGui::End();
}
void ImGui_Console2_OpenDefault(const char* title, bool* p_open)
{
	auto console = GetMainConsoleInstance();
	console->Draw(title, p_open);
}

// -------------
// commands
// -------------

void ImGui_Console_impl_Initialize_Command_List()
{
	command_table.insert(new ImGui_ConsoleCommand("help", "outputs all commands", 0, ImGui_Console2::help_cb));
	command_table.insert(new ImGui_ConsoleCommand("help2", "outputs all commands", 0, ImGui_Console2::help_cb));
	command_table.insert(new ImGui_ConsoleCommand("clear", "clear the output of the current console and history", 0, ImGui_Console2::clear_cb));
	command_table.insert(new ImGui_ConsoleVarCommand("var_console_opacity", "opacity of the console", 1, ImGui_Console2::get_opacity_var_cb, ImGui_Console2::set_opacity_cb));
}

// #### TODO code de-duplication and implementation improvements

// this defines: 
//		* command_line_argument_tokens
//		* console_data
//		* command_data
// returns from function if validation fails
#define IMGUI_CONSOLE_COMMAND_PROLOG(_commandType) \
std::vector<std::string> command_line_tokens; \
_commandType* command_data = (_commandType*)context; \
ImGui_Console2* console_data = command_data->GetUserData<ImGui_Console2*>(); \
if (console_data == NULL) return 0; \
if (!tokenize(command_line, command_line_length, " ", command_line_tokens)) return 0; \
if (command_line_tokens.size() > command_data->GetParameterCount() + 1) \
{ \
	console_data->output.AddStringFmt(StringFlag_None, command_error_not_enough_params, command_data->GetName()); \
	return 0; \
}

// clear command
int ImGui_Console2::clear_cb(const char* command_line, size_t command_line_length, void* context)
{
	IMGUI_CONSOLE_COMMAND_PROLOG(ImGui_ConsoleCommand);

	console_data->ClearOutput();
	return 0;
}

// help command
int ImGui_Console2::help_cb(const char* command_line, size_t command_line_length, void* context)
{
	IMGUI_CONSOLE_COMMAND_PROLOG(ImGui_ConsoleCommand);

	console_data->output.AddStringFmt(StringFlag_History, "%s", command_data->GetName());

	console_data->output.AddString(StringFlag_None, "# available commands: ");
	for (auto& command_entry : command_table)
	{
		ImGui_ConsoleCommand* command = (ImGui_ConsoleCommand*)command_entry;
		if ((command->GetFlags() & CommandFlag_Hidden) == 0)
		{
			console_data->output.AddStringFmt(StringFlag_None, "# %s ", command->GetName());
			if (command->GetDescription() != NULL)
			{
				console_data->output.AddStringFmt(StringFlag_None, "    # command description: %s", command->GetDescription());
			}
		}
	}

	return 0;
}

// gets pointer to data
// (will be useful for command suggestion)
CommandVar ImGui_Console2::get_opacity_var_cb(void* context)
{
	ImGui_ConsoleVarCommand* command_data = (ImGui_ConsoleVarCommand*)context;
	ImGui_Console2* console_data = command_data->GetUserData<ImGui_Console2*>(); // we pass the console as user data

	CommandVar ret;
	ret.ptr = &console_data->console_opacity;
	ret.var_type = CommandVar::ComVar_Float;

	return ret;
}

// set opacity command
int ImGui_Console2::set_opacity_cb(const char* command_line, size_t command_line_length, void* context)
{
	IMGUI_CONSOLE_COMMAND_PROLOG(ImGui_ConsoleVarCommand);

	console_data->output.AddStringFmt(StringFlag_History, command_line);

	try
	{
		StringToVal& val = reinterpret_cast<StringToVal&>(command_line_tokens[1]);
		float opacity_value = val.Get<float>();
		opacity_value = IM_CLAMP(opacity_value, 0.0f, 1.0f);
		command_data->queryVariable(command_data).Set<float>(opacity_value);
	}
	catch (const std::exception& e)
	{
		console_data->output.AddString(StringFlag_None, command_error_bad_arg);
		console_data->output.AddStringFmt(StringFlag_None, "	%s", e.what());
		return 0;
	}

	return 0;
}