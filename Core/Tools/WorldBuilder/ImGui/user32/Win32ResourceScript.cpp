#include "Win32ResourceScript.h"

#include <cctype>
#include <cstdlib>
#include <set>
#include <sstream>

namespace
{

bool isSeparator(char symbol)
{
	return std::isspace(static_cast<unsigned char>(symbol)) || symbol == ',' || symbol == '|';
}

size_t readQuoted(const std::string& line, size_t position, std::string& text)
{
	++position;
	while (position < line.size())
	{
		const char symbol = line[position];
		if (symbol != '"')
		{
			text += symbol;
			++position;
			continue;
		}

		const bool isEscapedQuote = position + 1 < line.size() && line[position + 1] == '"';
		if (!isEscapedQuote)
		{
			return position + 1;
		}
		text += "\\\"";
		position += 2;
	}
	return position;
}

char separatorKind(char current, char symbol)
{
	if (symbol == '|')
	{
		return '|';
	}
	if (symbol == ',' && current != '|')
	{
		return ',';
	}
	return current;
}

void tokenizeLine(const std::string& line, char& pendingSeparator, std::vector<Win32ResourceToken>& tokens)
{
	size_t position = 0;
	while (position < line.size())
	{
		const char symbol = line[position];
		if (isSeparator(symbol))
		{
			pendingSeparator = separatorKind(pendingSeparator, symbol);
			++position;
			continue;
		}
		if (symbol == '/' && position + 1 < line.size() && line[position + 1] == '/')
		{
			return;
		}

		Win32ResourceToken token;
		token.separator = pendingSeparator;
		pendingSeparator = ' ';
		if (symbol == '"')
		{
			token.isQuoted = true;
			position = readQuoted(line, position, token.text);
			tokens.push_back(token);
			continue;
		}

		while (position < line.size() && !isSeparator(line[position]) && line[position] != '"')
		{
			token.text += line[position];
			++position;
		}
		tokens.push_back(token);
	}
}

}

std::vector<std::string> Win32SplitLines(const std::string& contents)
{
	std::vector<std::string> lines;
	std::istringstream stream(contents);
	std::string line;
	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		lines.push_back(line);
	}
	return lines;
}

std::vector<Win32ResourceToken> Win32TokenizeResourceScript(const std::string& contents)
{
	std::vector<Win32ResourceToken> tokens;
	char pendingSeparator = ' ';
	for (const std::string& line : Win32SplitLines(contents))
	{
		const size_t first = line.find_first_not_of(" \t");
		if (first != std::string::npos && line[first] == '#')
		{
			continue;
		}
		tokenizeLine(line, pendingSeparator, tokens);
	}
	return tokens;
}

std::string Win32UnescapeResourceString(const std::string& text)
{
	std::string result;
	for (size_t index = 0; index < text.size(); ++index)
	{
		if (text[index] != '\\' || index + 1 >= text.size())
		{
			result += text[index];
			continue;
		}

		const char next = text[++index];
		if (next >= '0' && next <= '7')
		{
			int value = 0;
			size_t digits = 0;
			for (; digits < 3 && index < text.size() && text[index] >= '0' && text[index] <= '7'; ++digits, ++index)
			{
				value = value * 8 + (text[index] - '0');
			}
			--index;
			result += static_cast<char>(value);
			continue;
		}
		result += next == 'n' ? '\n' : next == 't' ? '\t' : next == 'a' ? '\a' : next == 'r' ? '\r' : next;
	}
	return result;
}
