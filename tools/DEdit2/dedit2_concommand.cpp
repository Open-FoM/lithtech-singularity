#include "dedit2_concommand.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
struct ConsoleVar
{
	enum class Type
	{
		Int,
		Float,
		String
	};

	std::string name;
	Type type = Type::String;
	void* target = nullptr;
	std::string value;
};

using CommandFn = void (*)(int argc, const char* argv[]);

struct ConsoleCommand
{
	std::string name;
	CommandFn fn = nullptr;
};

std::unordered_map<std::string, ConsoleVar> g_Vars;
std::unordered_map<std::string, ConsoleCommand> g_Commands;
std::vector<std::string> g_CommandOrder;
std::vector<std::string> g_Log;
DEdit2ConsoleBindings g_Bindings;

std::string ToLower(const std::string& value)
{
	std::string out = value;
	for (char& ch : out)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return out;
}

std::vector<std::string> Tokenize(const char* command)
{
	std::vector<std::string> tokens;
	if (!command)
	{
		return tokens;
	}

	std::string current;
	bool in_quotes = false;
	bool escaped = false;
	for (const char* ptr = command; *ptr; ++ptr)
	{
		char ch = *ptr;
		if (escaped)
		{
			current.push_back(ch);
			escaped = false;
			continue;
		}

		if (in_quotes && ch == '\\')
		{
			escaped = true;
			continue;
		}

		if (ch == '"')
		{
			in_quotes = !in_quotes;
			continue;
		}

		if (!in_quotes && std::isspace(static_cast<unsigned char>(ch)))
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
			continue;
		}

		current.push_back(ch);
	}

	if (!current.empty())
	{
		tokens.push_back(current);
	}

	return tokens;
}

void AddCommand(const char* name, CommandFn fn)
{
	if (!name || !fn)
	{
		return;
	}
	const std::string key = ToLower(name);
	ConsoleCommand cmd{std::string(name), fn};
	g_Commands[key] = cmd;
	g_CommandOrder.push_back(cmd.name);
}

void AddVar(const char* name, ConsoleVar::Type type, void* target)
{
	if (!name)
	{
		return;
	}
	const std::string key = ToLower(name);
	ConsoleVar var;
	var.name = name;
	var.type = type;
	var.target = target;
	g_Vars[key] = var;
}

bool ParseInt(const char* text, int& out)
{
	if (!text)
	{
		return false;
	}
	char* end = nullptr;
	long value = std::strtol(text, &end, 10);
	if (end == text)
	{
		return false;
	}
	out = static_cast<int>(value);
	return true;
}

bool ParseFloat(const char* text, float& out)
{
	if (!text)
	{
		return false;
	}
	char* end = nullptr;
	float value = std::strtof(text, &end);
	if (end == text)
	{
		return false;
	}
	out = value;
	return true;
}

const char* VarToString(const ConsoleVar& var)
{
	static char buffer[128];
	buffer[0] = '\0';
	if (var.type == ConsoleVar::Type::Int)
	{
		int value = 0;
		if (var.target)
		{
			value = *static_cast<int*>(var.target);
		}
		std::snprintf(buffer, sizeof(buffer), "%d", value);
		return buffer;
	}
	if (var.type == ConsoleVar::Type::Float)
	{
		float value = 0.0f;
		if (var.target)
		{
			value = *static_cast<float*>(var.target);
		}
		std::snprintf(buffer, sizeof(buffer), "%0.6f", value);
		return buffer;
	}
	if (var.target)
	{
		return static_cast<std::string*>(var.target)->c_str();
	}
	return var.value.c_str();
}

void CmdClear(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_ClearLog();
	DEdit2_Log("Console cleared.");
}

void CmdEcho(int argc, const char* argv[])
{
	if (argc <= 0)
	{
		DEdit2_Log("(no args)");
		return;
	}
	std::string message;
	for (int i = 0; i < argc; ++i)
	{
		if (i > 0)
		{
			message += ' ';
		}
		message += argv[i];
	}
	DEdit2_Log("%s", message.c_str());
}

void CmdHelp(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_Log("Commands:");
	for (const auto& name : g_CommandOrder)
	{
		DEdit2_Log("  %s", name.c_str());
	}
	DEdit2_Log("Variables:");
	for (const auto& pair : g_Vars)
	{
		const ConsoleVar& var = pair.second;
		DEdit2_Log("  %s = %s", var.name.c_str(), VarToString(var));
	}
}

void DumpNodes(const std::vector<TreeNode>& nodes, int node_id, int indent)
{
	if (node_id < 0 || static_cast<size_t>(node_id) >= nodes.size())
	{
		return;
	}
	const TreeNode& node = nodes[node_id];
	std::string padding(static_cast<size_t>(indent) * 2, ' ');
	DEdit2_Log("%s- %s", padding.c_str(), node.name.c_str());
	for (int child_id : node.children)
	{
		DumpNodes(nodes, child_id, indent + 1);
	}
}

void CmdListNodes(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	if (g_Bindings.project_nodes && !g_Bindings.project_nodes->empty())
	{
		DEdit2_Log("Project nodes:");
		DumpNodes(*g_Bindings.project_nodes, 0, 0);
	}
	else
	{
		DEdit2_Log("Project nodes unavailable.");
	}

	if (g_Bindings.scene_nodes && !g_Bindings.scene_nodes->empty())
	{
		DEdit2_Log("Scene nodes:");
		DumpNodes(*g_Bindings.scene_nodes, 0, 0);
	}
	else
	{
		DEdit2_Log("Scene nodes unavailable.");
	}
}

void CmdFreeTextures(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_Log("freetextures is not implemented in DEdit2 yet.");
}

void CmdReplaceTex(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_Log("replacetex is not implemented in DEdit2 yet.");
}

} // namespace

int g_DEdit2MipMapOffset = 0;
int g_DEdit2Bilinear = 1;
int g_DEdit2MaxTextureMemory = 128 * 1024 * 1024;

void DEdit2_InitConsoleCommands()
{
	g_Commands.clear();
	g_CommandOrder.clear();
	g_Vars.clear();

	AddCommand("clear", CmdClear);
	AddCommand("echo", CmdEcho);
	AddCommand("help", CmdHelp);
	AddCommand("listnodes", CmdListNodes);
	AddCommand("freetextures", CmdFreeTextures);
	AddCommand("replacetex", CmdReplaceTex);

	AddVar("mipmapoffset", ConsoleVar::Type::Int, &g_DEdit2MipMapOffset);
	AddVar("bilinear", ConsoleVar::Type::Int, &g_DEdit2Bilinear);
	AddVar("MaxTextureMemory", ConsoleVar::Type::Int, &g_DEdit2MaxTextureMemory);

	DEdit2_Log("DEdit2 console initialized. Type 'help' for commands.");
}

void DEdit2_TermConsoleCommands()
{
	g_Commands.clear();
	g_CommandOrder.clear();
	g_Vars.clear();
	g_Log.clear();
}

void DEdit2_CommandHandler(const char* command)
{
	if (!command)
	{
		return;
	}
	const std::string raw(command);
	if (raw.find_first_not_of(" \t\n\r") == std::string::npos)
	{
		return;
	}

	DEdit2_Log("> %s", raw.c_str());
	std::vector<std::string> tokens = Tokenize(command);
	if (tokens.empty())
	{
		return;
	}

	const std::string key = ToLower(tokens[0]);
	auto cmd_it = g_Commands.find(key);
	if (cmd_it != g_Commands.end())
	{
		std::vector<const char*> args;
		for (size_t i = 1; i < tokens.size(); ++i)
		{
			args.push_back(tokens[i].c_str());
		}
		cmd_it->second.fn(static_cast<int>(args.size()), args.data());
		return;
	}

	auto var_it = g_Vars.find(key);
	if (tokens.size() == 1)
	{
		if (var_it != g_Vars.end())
		{
			DEdit2_Log("%s = %s", var_it->second.name.c_str(), VarToString(var_it->second));
		}
		else
		{
			DEdit2_Log("Unknown command or variable '%s'.", tokens[0].c_str());
		}
		return;
	}

	std::string value = tokens[1];
	for (size_t i = 2; i < tokens.size(); ++i)
	{
		value += ' ';
		value += tokens[i];
	}

	if (var_it == g_Vars.end())
	{
		ConsoleVar var;
		var.name = tokens[0];
		var.type = ConsoleVar::Type::String;
		var.value = value;
		g_Vars[key] = var;
		DEdit2_Log("%s = %s", var.name.c_str(), var.value.c_str());
		return;
	}

	ConsoleVar& var = var_it->second;
	bool updated = false;
	switch (var.type)
	{
		case ConsoleVar::Type::Int:
		{
			int parsed = 0;
			if (ParseInt(value.c_str(), parsed))
			{
				if (var.target)
				{
					*static_cast<int*>(var.target) = parsed;
				}
				updated = true;
			}
			break;
		}
		case ConsoleVar::Type::Float:
		{
			float parsed = 0.0f;
			if (ParseFloat(value.c_str(), parsed))
			{
				if (var.target)
				{
					*static_cast<float*>(var.target) = parsed;
				}
				updated = true;
			}
			break;
		}
		case ConsoleVar::Type::String:
		{
			if (var.target)
			{
				*static_cast<std::string*>(var.target) = value;
			}
			else
			{
				var.value = value;
			}
			updated = true;
			break;
		}
	}

	if (!updated)
	{
		DEdit2_Log("Invalid value for %s: %s", var.name.c_str(), value.c_str());
		return;
	}

	DEdit2_Log("%s = %s", var.name.c_str(), VarToString(var));
}

void DEdit2_SetConsoleBindings(const DEdit2ConsoleBindings& bindings)
{
	g_Bindings = bindings;
}

void DEdit2_LogV(const char* fmt, va_list args)
{
	if (!fmt)
	{
		return;
	}

	char buffer[1024];
	std::vsnprintf(buffer, sizeof(buffer), fmt, args);

	g_Log.emplace_back(buffer);
	std::fprintf(stderr, "%s\n", buffer);
}

void DEdit2_Log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	DEdit2_LogV(fmt, args);
	va_end(args);
}

void DEdit2_ClearLog()
{
	g_Log.clear();
}

const std::vector<std::string>& DEdit2_GetLog()
{
	return g_Log;
}
