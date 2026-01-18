// POSIX/SDL entry point.

#include <string>

#include "bdefs.h"

extern int ltjs_run_client_main(void* hInstance, const char* cmdline);

static void append_argument(std::string& out, const char* arg)
{
	bool needs_quotes = false;
	for (const char* p = arg; *p != '\0'; ++p)
	{
		if (*p == ' ' || *p == '\t' || *p == '"')
		{
			needs_quotes = true;
			break;
		}
	}

	if (!needs_quotes)
	{
		out.append(arg);
		return;
	}

	out.push_back('"');

	int backslash_count = 0;
	for (const char* p = arg; *p != '\0'; ++p)
	{
		if (*p == '\\')
		{
			++backslash_count;
			continue;
		}

		if (*p == '"')
		{
			out.append(static_cast<size_t>(backslash_count) * 2U + 1U, '\\');
			out.push_back('"');
		}
		else
		{
			if (backslash_count > 0)
			{
				out.append(static_cast<size_t>(backslash_count), '\\');
			}
			out.push_back(*p);
		}

		backslash_count = 0;
	}

	if (backslash_count > 0)
	{
		out.append(static_cast<size_t>(backslash_count) * 2U, '\\');
	}

	out.push_back('"');
}

static std::string build_command_line(int argc, char** argv)
{
	std::string cmdline;
	for (int i = 1; i < argc; ++i)
	{
		if (!cmdline.empty())
		{
			cmdline.push_back(' ');
		}

		append_argument(cmdline, argv[i]);
	}

	return cmdline;
}

int main(int argc, char** argv)
{
	const auto cmdline = build_command_line(argc, argv);
	return ltjs_run_client_main(nullptr, cmdline.c_str());
}
