#include "memory_stream.h"
#include "../system_backend.h"

using namespace llt;

MemoryStream::MemoryStream()
	: Stream()
{
}

MemoryStream::MemoryStream(void* memory, uint64_t length)
{
	open(memory, length);
}

MemoryStream& MemoryStream::open(void* memory, uint64_t length)
{
	p_stream = g_systemBackend->streamFromMemory(memory, length);
	return *this;
}

/////////////////////////////////////////////////////////

ConstMemoryStream::ConstMemoryStream()
	: Stream()
{
}

ConstMemoryStream::ConstMemoryStream(const void* memory, uint64_t length)
{
	open(memory, length);
}

ConstMemoryStream& ConstMemoryStream::open(const void* memory, uint64_t length)
{
	p_stream = g_systemBackend->streamFromConstMemory(memory, length);
	return *this;
}