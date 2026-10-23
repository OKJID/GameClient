// MacOSJsonWriter.h — streaming JSON writer for the files the launcher reads (hotkey dump, hotkey presets)

#pragma once

#include <cstdio>
#include <vector>

class MacOSJsonWriter
{
	FILE* m_file;

public:
	explicit MacOSJsonWriter( FILE* file ) : m_file( file ) {}

	void beginObject() { openScope( '{' ); }
	void endObject() { closeScope( '}' ); }
	void beginArray() { openScope( '[' ); }
	void endArray() { closeScope( ']' ); }

	void key( const char* name )
	{
		separate();
		writeQuoted( name );
		fputc( ':', m_file );
		m_isAfterKey = true;
	}

	void text( const char* value )
	{
		separate();
		writeQuoted( value );
	}

	void number( int value )
	{
		separate();
		fprintf( m_file, "%d", value );
	}

	void boolean( bool value )
	{
		separate();
		fputs( value ? "true" : "false", m_file );
	}

	void null()
	{
		separate();
		fputs( "null", m_file );
	}

private:
	std::vector<bool> m_scopeHasItems;
	bool m_isAfterKey = false;

	void openScope( char bracket )
	{
		separate();
		fputc( bracket, m_file );
		m_scopeHasItems.push_back( false );
	}

	void closeScope( char bracket )
	{
		m_scopeHasItems.pop_back();
		fputc( bracket, m_file );
	}

	void separate()
	{
		if ( m_isAfterKey )
		{
			m_isAfterKey = false;
			return;
		}

		if ( m_scopeHasItems.empty() )
			return;

		if ( m_scopeHasItems.back() )
			fputc( ',', m_file );

		m_scopeHasItems.back() = true;
	}

	void writeQuoted( const char* value )
	{
		fputc( '"', m_file );
		for ( const unsigned char* c = (const unsigned char*)value; *c; ++c )
			writeEscaped( *c );
		fputc( '"', m_file );
	}

	void writeEscaped( unsigned char c )
	{
		if ( c == '"' || c == '\\' )
		{
			fputc( '\\', m_file );
			fputc( c, m_file );
			return;
		}

		if ( c < 0x20 )
		{
			fprintf( m_file, "\\u%04x", c );
			return;
		}

		fputc( c, m_file );
	}
};
