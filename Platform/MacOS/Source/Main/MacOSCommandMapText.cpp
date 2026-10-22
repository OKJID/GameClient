// MacOSCommandMapText.cpp — sets the key of one CommandMap block inside CommandMap.ini text

#include "MacOSCommandMapText.h"

#include <strings.h>
#include <vector>

namespace
{
	const char* const kBlockKeyword = "CommandMap";
	const char* const kEndKeyword = "End";
	const char* const kKeyField = "Key";
	const char* const kModifiersField = "Modifiers";
	const char* const kIndent = "  ";

	struct Line
	{
		std::string text;
		std::string ending;
	};

	std::vector<Line> splitLines( const std::string& ini )
	{
		std::vector<Line> lines;
		size_t start = 0;
		while ( start < ini.size() )
		{
			const size_t newline = ini.find( '\n', start );
			const size_t end = newline == std::string::npos ? ini.size() : newline;
			const size_t textEnd = ( end > start && ini[end - 1] == '\r' ) ? end - 1 : end;
			lines.push_back( { ini.substr( start, textEnd - start ), ini.substr( textEnd, ( newline == std::string::npos ? end : end + 1 ) - textEnd ) } );
			start = newline == std::string::npos ? ini.size() : newline + 1;
		}
		return lines;
	}

	std::string joinLines( const std::vector<Line>& lines )
	{
		std::string ini;
		for ( const Line& line : lines )
			ini += line.text + line.ending;
		return ini;
	}

	std::vector<std::string> tokensOf( const std::string& text )
	{
		const std::string code = text.substr( 0, text.find( ';' ) );
		std::vector<std::string> tokens;
		size_t start = code.find_first_not_of( " \t=" );
		while ( start != std::string::npos )
		{
			const size_t end = code.find_first_of( " \t=", start );
			tokens.push_back( code.substr( start, end == std::string::npos ? std::string::npos : end - start ) );
			start = end == std::string::npos ? std::string::npos : code.find_first_not_of( " \t=", end );
		}
		return tokens;
	}

	bool isToken( const std::vector<std::string>& tokens, size_t index, const std::string& expected )
	{
		return tokens.size() > index && strcasecmp( tokens[index].c_str(), expected.c_str() ) == 0;
	}

	size_t blockStartOf( const std::vector<Line>& lines, const std::string& name )
	{
		for ( size_t i = 0; i < lines.size(); ++i )
		{
			const std::vector<std::string> tokens = tokensOf( lines[i].text );
			if ( isToken( tokens, 0, kBlockKeyword ) && isToken( tokens, 1, name ) )
				return i;
		}
		return std::string::npos;
	}

	size_t blockEndOf( const std::vector<Line>& lines, size_t start )
	{
		for ( size_t i = start + 1; i < lines.size(); ++i )
		{
			if ( isToken( tokensOf( lines[i].text ), 0, kEndKeyword ) )
				return i;
		}
		return std::string::npos;
	}

	std::string lineEndingOf( const std::string& ini )
	{
		return ini.find( "\r\n" ) != std::string::npos ? "\r\n" : "\n";
	}

	void setField( std::vector<Line>& lines, size_t start, size_t& end, const std::string& field, const std::string& value )
	{
		const std::string text = std::string( kIndent ) + field + " = " + value;
		for ( size_t i = start + 1; i < end; ++i )
		{
			if ( isToken( tokensOf( lines[i].text ), 0, field ) )
			{
				lines[i].text = text;
				return;
			}
		}

		lines.insert( lines.begin() + end, { text, lines[start].ending } );
		++end;
	}

	std::string withNewBlock( const std::string& ini, const MacOSCommandMapEntry& entry )
	{
		const std::string ending = lineEndingOf( ini );
		std::string block = ini;
		if ( !block.empty() && block.back() != '\n' )
			block += ending;

		block += ending + kBlockKeyword + " " + entry.name + ending;
		block += std::string( kIndent ) + kKeyField + " = " + entry.key + ending;
		block += std::string( kIndent ) + "Transition = " + entry.transition + ending;
		block += std::string( kIndent ) + kModifiersField + " = " + entry.modifiers + ending;
		block += std::string( kIndent ) + "UseableIn = " + entry.usableIn + ending;
		block += std::string( kIndent ) + "Category = " + entry.category + ending;
		block += std::string( kEndKeyword ) + ending;
		return block;
	}
}

std::string MacOSCommandMapText::withEntry( const std::string& ini, const MacOSCommandMapEntry& entry )
{
	std::vector<Line> lines = splitLines( ini );
	const size_t start = blockStartOf( lines, entry.name );
	if ( start == std::string::npos )
		return withNewBlock( ini, entry );

	size_t end = blockEndOf( lines, start );
	if ( end == std::string::npos )
		return withNewBlock( ini, entry );

	setField( lines, start, end, kKeyField, entry.key );
	setField( lines, start, end, kModifiersField, entry.modifiers );
	return joinLines( lines );
}
