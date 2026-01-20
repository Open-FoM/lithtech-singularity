#include "lt_stream.h"

#include <cstdio>
#include <cstring>

namespace
{
class FileStream final : public CGenLTStream
{
public:
	explicit FileStream(FILE* file)
		: file_(file)
	{
	}

	~FileStream() override
	{
		if (file_)
		{
			std::fclose(file_);
			file_ = nullptr;
		}
	}

	void Release() override
	{
		delete this;
	}

	LTRESULT Read(void* data, uint32 size) override
	{
		if (!file_)
		{
			return LT_ERROR;
		}

		if (size == 0)
		{
			return LT_OK;
		}

		const size_t bytes_read = std::fread(data, 1, size, file_);
		if (bytes_read == size)
		{
			return LT_OK;
		}

		std::memset(data, 0, size);
		error_ = true;
		return LT_ERROR;
	}

	LTRESULT Write(const void* /*data*/, uint32 /*size*/) override
	{
		error_ = true;
		return LT_ERROR;
	}

	LTRESULT ErrorStatus() override
	{
		return error_ ? LT_ERROR : LT_OK;
	}

	LTRESULT SeekTo(uint32 offset) override
	{
		if (!file_)
		{
			return LT_ERROR;
		}

		return (std::fseek(file_, static_cast<long>(offset), SEEK_SET) == 0) ? LT_OK : LT_ERROR;
	}

	LTRESULT GetPos(uint32* offset) override
	{
		if (!file_ || !offset)
		{
			return LT_ERROR;
		}

		const long pos = std::ftell(file_);
		if (pos < 0)
		{
			*offset = 0;
			return LT_ERROR;
		}

		*offset = static_cast<uint32>(pos);
		return LT_OK;
	}

	LTRESULT GetLen(uint32* len) override
	{
		if (!file_ || !len)
		{
			return LT_ERROR;
		}

		const long cur = std::ftell(file_);
		if (cur < 0)
		{
			*len = 0;
			return LT_ERROR;
		}

		std::fseek(file_, 0, SEEK_END);
		const long end = std::ftell(file_);
		std::fseek(file_, cur, SEEK_SET);
		if (end < 0)
		{
			*len = 0;
			return LT_ERROR;
		}

		*len = static_cast<uint32>(end);
		return LT_OK;
	}

private:
	FILE* file_ = nullptr;
	bool error_ = false;
};
} // namespace

ILTStream* OpenFileStream(const std::string& path)
{
	FILE* file = std::fopen(path.c_str(), "rb");
	if (!file)
	{
		return nullptr;
	}

	return new FileStream(file);
}
