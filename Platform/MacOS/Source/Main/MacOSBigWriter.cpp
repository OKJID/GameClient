// MacOSBigWriter.cpp — BIGF: id, little-endian archive size, big-endian file count and header size,
// then big-endian offset/size + zero-terminated path per file, then the file bodies

#include "MacOSBigWriter.h"

#include "Common/System/NativeFileSystem.h"

#include <cstdint>
#include <cstdio>

namespace
{
	const char kBigIdentifier[] = { 'B', 'I', 'G', 'F' };
	const size_t kFixedHeaderSize = 16;
	const size_t kDirectoryRecordSize = 8;

	void appendLittleEndian( std::vector<unsigned char>& out, uint32_t value )
	{
		for ( int shift = 0; shift <= 24; shift += 8 )
			out.push_back( (unsigned char)( value >> shift ) );
	}

	void appendBigEndian( std::vector<unsigned char>& out, uint32_t value )
	{
		for ( int shift = 24; shift >= 0; shift -= 8 )
			out.push_back( (unsigned char)( value >> shift ) );
	}

	size_t headerSize( const std::vector<MacOSBigWriter::Entry>& entries )
	{
		size_t size = kFixedHeaderSize;
		for ( const MacOSBigWriter::Entry& entry : entries )
			size += kDirectoryRecordSize + entry.path.size() + 1;
		return size;
	}

	size_t archiveSize( const std::vector<MacOSBigWriter::Entry>& entries )
	{
		size_t size = headerSize( entries );
		for ( const MacOSBigWriter::Entry& entry : entries )
			size += entry.bytes.size();
		return size;
	}

	std::vector<unsigned char> header( const std::vector<MacOSBigWriter::Entry>& entries )
	{
		const size_t directoryEnd = headerSize( entries );
		std::vector<unsigned char> out( kBigIdentifier, kBigIdentifier + sizeof( kBigIdentifier ) );
		appendLittleEndian( out, (uint32_t)archiveSize( entries ) );
		appendBigEndian( out, (uint32_t)entries.size() );
		appendBigEndian( out, (uint32_t)directoryEnd );

		size_t offset = directoryEnd;
		for ( const MacOSBigWriter::Entry& entry : entries )
		{
			appendBigEndian( out, (uint32_t)offset );
			appendBigEndian( out, (uint32_t)entry.bytes.size() );
			out.insert( out.end(), entry.path.begin(), entry.path.end() );
			out.push_back( 0 );
			offset += entry.bytes.size();
		}
		return out;
	}
}

bool MacOSBigWriter::write( const std::string& enginePath, const std::vector<Entry>& entries )
{
	FILE* file = NativeFileSystem::fopen( enginePath, "wb" );
	if ( file == nullptr )
		return false;

	const std::vector<unsigned char> directory = header( entries );
	fwrite( directory.data(), 1, directory.size(), file );
	for ( const Entry& entry : entries )
		fwrite( entry.bytes.data(), 1, entry.bytes.size(), file );

	const bool isWritten = ferror( file ) == 0;
	fclose( file );
	return isWritten;
}
