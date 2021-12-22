#pragma once

namespace utils
{
	std::vector<std::string> split( const std::string& s, char delimiter );
	bool is_number( std::string& s );

	// Windows utilities
	const char* Win32_system_error_to_string(DWORD error);
}

