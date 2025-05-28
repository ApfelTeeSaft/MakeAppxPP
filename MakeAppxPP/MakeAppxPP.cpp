#include "CommandLineParser.h"
#include <iostream>
#include <locale>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#endif

void SetupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
#endif

    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale());
    std::wcerr.imbue(std::locale());
    std::wcin.imbue(std::locale());
}

int wmain(int argc, wchar_t* argv[]) {
    SetupConsole();

    MakeAppxPP::CommandLineParser parser;
    MakeAppxPP::CommandLineArgs args;

    try {
        if (!parser.Parse(argc, argv, args)) {
            std::wcerr << L"Error: " << parser.GetLastError() << std::endl;
            std::wcerr << L"Use 'MakeAppxPro /?' for help." << std::endl;
            return 1;
        }

        if (args.showHelp) {
            if (args.specificCommand.empty()) {
                MakeAppxPP::CommandLineParser::ShowGeneralHelp();
            }
            else {
                MakeAppxPP::CommandLineParser::ShowCommandHelp(args.specificCommand);
            }
            return 0;
        }

        return MakeAppxPP::ExecuteCommand(args);
    }
    catch (const std::exception& e) {
        std::wcerr << L"Fatal error: ";
        std::wcerr << e.what() << std::endl;
        return 2;
    }
    catch (...) {
        std::wcerr << L"Fatal unexpected error occurred" << std::endl;
        return 2;
    }

    return 0;
}

#ifndef _WIN32
int main(int argc, char* argv[]) {
    std::vector<std::wstring> wideArgs;
    std::vector<wchar_t*> wideArgPtrs;

    for (int i = 0; i < argc; ++i) {
        std::wstring wideArg(argv[i], argv[i] + strlen(argv[i]));
        wideArgs.push_back(wideArg);
        wideArgPtrs.push_back(&wideArgs.back()[0]);
    }

    return wmain(argc, wideArgPtrs.data());
}
#endif