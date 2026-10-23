// MacOSBigWriter.h — writes a BIGF archive in the layout StdBIGFileSystem::openArchiveFile reads

#pragma once

#include <string>
#include <vector>

namespace MacOSBigWriter
{
	struct Entry
	{
		std::string path;
		std::vector<unsigned char> bytes;
	};

	bool write( const std::string& enginePath, const std::vector<Entry>& entries );
}
