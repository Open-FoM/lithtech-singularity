#include <cstdio>

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	std::fputs(
		"DEdit is currently Windows-only (MFC-based).\n"
		"This macOS build is a stub placeholder while the editor is ported.\n",
		stderr);
	return 1;
}
