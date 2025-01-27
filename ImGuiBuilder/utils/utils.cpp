#include "pch.h"

#include "utils.h"

std::vector<std::string> utils::split( const std::string& s, char delimiter )
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream( s );
	while ( std::getline( tokenStream, token, delimiter ) )
	{
		tokens.push_back( token );
	}
	return tokens;
}

bool utils::is_number( std::string& s )
{
	auto find = s.find( ' ' );
	if ( find != std::string::npos )
		s.replace( find, 1, "" );
	find = s.find( '\n' );
	if ( find != std::string::npos )
		s.replace( find, 1, "" );
	find = s.find( '\r' );
	if ( find != std::string::npos )
		s.replace( find, 1, "" );

	return std::regex_match( s, std::regex( "[+-]?([0-9]*[.])?[0-9]+" ) );
}

#define CASE_TO_STRING(x) \
case x:					  \
return #x;

const char* utils::Win32_system_error_to_string(DWORD error)
{
	switch (error)
	{
		CASE_TO_STRING(ERROR_FILE_NOT_FOUND);
		CASE_TO_STRING(ERROR_NO_MORE_FILES);

	default:
		return "<unknown>";
	}
}