#include "CommandLineParser.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace MakeAppxPP {

    bool CommandLineParser::Parse(int argc, wchar_t* argv[], CommandLineArgs& args) {
        m_args.clear();
        m_lastError.clear();

        for (int i = 1; i < argc; ++i) {
            m_args.push_back(argv[i]);
        }

        if (m_args.empty()) {
            args.showHelp = true;
            return true;
        }

        size_t index = 0;
        std::wstring cmdStr = GetNextArg(index);
        args.command = ParseCommand(cmdStr);

        if (args.command == Command::None) {
            SetError(L"Invalid command: " + cmdStr);
            return false;
        }

        if (args.command == Command::Help) {
            args.showHelp = true;
            if (index < m_args.size()) {
                args.specificCommand = GetNextArg(index);
            }
            return true;
        }

        switch (args.command) {
        case Command::Pack:
            return ParsePackArgs(args, index);
        case Command::Unpack:
            return ParseUnpackArgs(args, index);
        case Command::Bundle:
            return ParseBundleArgs(args, index);
        case Command::Unbundle:
            return ParseUnbundleArgs(args, index);
        case Command::Encrypt:
            return ParseEncryptArgs(args, index);
        case Command::Decrypt:
            return ParseDecryptArgs(args, index);
        case Command::ConvertCGM:
            return ParseConvertCGMArgs(args, index);
        case Command::Build:
            return ParseBuildArgs(args, index);
        default:
            SetError(L"Unknown command");
            return false;
        }
    }

    Command CommandLineParser::ParseCommand(const std::wstring& cmdStr) {
        std::wstring cmd = cmdStr;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::towlower);

        if (cmd == L"pack") return Command::Pack;
        if (cmd == L"unpack") return Command::Unpack;
        if (cmd == L"bundle") return Command::Bundle;
        if (cmd == L"unbundle") return Command::Unbundle;
        if (cmd == L"encrypt") return Command::Encrypt;
        if (cmd == L"decrypt") return Command::Decrypt;
        if (cmd == L"convertcgm") return Command::ConvertCGM;
        if (cmd == L"build") return Command::Build;
        if (cmd == L"help" || cmd == L"/?" || cmd == L"-help" || cmd == L"--help") return Command::Help;

        return Command::None;
    }

    bool CommandLineParser::ParsePackArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"pack";
                return true;
            }
            else if (arg == L"-d" || arg == L"/d") {
                args.inputPath = GetNextArg(index);
                if (args.inputPath.empty()) {
                    SetError(L"Missing directory path for -d option");
                    return false;
                }
            }
            else if (arg == L"-p" || arg == L"/p") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing package path for -p option");
                    return false;
                }
            }
            else if (arg == L"-c" || arg == L"/c") {
                std::wstring compressionStr = GetNextArg(index);
                if (compressionStr == L"none") {
                    args.compression = MakeAppxCore::CompressionLevel::None;
                }
                else if (compressionStr == L"fast") {
                    args.compression = MakeAppxCore::CompressionLevel::Fast;
                }
                else if (compressionStr == L"normal") {
                    args.compression = MakeAppxCore::CompressionLevel::Normal;
                }
                else if (compressionStr == L"max") {
                    args.compression = MakeAppxCore::CompressionLevel::Maximum;
                }
                else {
                    SetError(L"Invalid compression level: " + compressionStr);
                    return false;
                }
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.inputPath.empty()) {
            SetError(L"Missing required -d (directory) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -p (package) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseUnpackArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"unpack";
                return true;
            }
            else if (arg == L"-p" || arg == L"/p") {
                args.inputPath = GetNextArg(index);
                if (args.inputPath.empty()) {
                    SetError(L"Missing package path for -p option");
                    return false;
                }
            }
            else if (arg == L"-d" || arg == L"/d") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing directory path for -d option");
                    return false;
                }
            }
            else if (arg == L"-o" || arg == L"/o") {
                args.overwrite = MakeAppxCore::OverwriteMode::Yes;
            }
            else if (arg == L"-s" || arg == L"/s") {
                args.overwrite = MakeAppxCore::OverwriteMode::No;
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.inputPath.empty()) {
            SetError(L"Missing required -p (package) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -d (directory) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseBundleArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"bundle";
                return true;
            }
            else if (arg == L"-d" || arg == L"/d") {
                args.inputPath = GetNextArg(index);
                if (args.inputPath.empty()) {
                    SetError(L"Missing directory path for -d option");
                    return false;
                }
            }
            else if (arg == L"-p" || arg == L"/p") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing bundle path for -p option");
                    return false;
                }
            }
            else if (arg == L"-c" || arg == L"/c") {
                std::wstring compressionStr = GetNextArg(index);
                if (compressionStr == L"none") {
                    args.compression = MakeAppxCore::CompressionLevel::None;
                }
                else if (compressionStr == L"fast") {
                    args.compression = MakeAppxCore::CompressionLevel::Fast;
                }
                else if (compressionStr == L"normal") {
                    args.compression = MakeAppxCore::CompressionLevel::Normal;
                }
                else if (compressionStr == L"max") {
                    args.compression = MakeAppxCore::CompressionLevel::Maximum;
                }
                else {
                    SetError(L"Invalid compression level: " + compressionStr);
                    return false;
                }
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.inputPath.empty()) {
            SetError(L"Missing required -d (directory) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -p (bundle) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseUnbundleArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"unbundle";
                return true;
            }
            else if (arg == L"-p" || arg == L"/p") {
                args.inputPath = GetNextArg(index);
                if (args.inputPath.empty()) {
                    SetError(L"Missing bundle path for -p option");
                    return false;
                }
            }
            else if (arg == L"-d" || arg == L"/d") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing directory path for -d option");
                    return false;
                }
            }
            else if (arg == L"-o" || arg == L"/o") {
                args.overwrite = MakeAppxCore::OverwriteMode::Yes;
            }
            else if (arg == L"-s" || arg == L"/s") {
                args.overwrite = MakeAppxCore::OverwriteMode::No;
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.inputPath.empty()) {
            SetError(L"Missing required -p (bundle) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -d (directory) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseEncryptArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"encrypt";
                return true;
            }
            else if (arg == L"-p" || arg == L"/p") {
                args.inputPath = GetNextArg(index);
                if (args.inputPath.empty()) {
                    SetError(L"Missing package path for -p option");
                    return false;
                }
            }
            else if (arg == L"-ep" || arg == L"/ep") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing encrypted package path for -ep option");
                    return false;
                }
            }
            else if (arg == L"-kf" || arg == L"/kf") {
                args.keyFile = GetNextArg(index);
                if (args.keyFile.empty()) {
                    SetError(L"Missing key file path for -kf option");
                    return false;
                }
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.inputPath.empty()) {
            SetError(L"Missing required -p (package) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -ep (encrypted package) option");
            return false;
        }
        if (args.keyFile.empty()) {
            SetError(L"Missing required -kf (key file) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseDecryptArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"decrypt";
                return true;
            }
            else if (arg == L"-ep" || arg == L"/ep") {
                args.inputPath = GetNextArg(index);
                if (args.inputPath.empty()) {
                    SetError(L"Missing encrypted package path for -ep option");
                    return false;
                }
            }
            else if (arg == L"-p" || arg == L"/p") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing package path for -p option");
                    return false;
                }
            }
            else if (arg == L"-kf" || arg == L"/kf") {
                args.keyFile = GetNextArg(index);
                if (args.keyFile.empty()) {
                    SetError(L"Missing key file path for -kf option");
                    return false;
                }
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.inputPath.empty()) {
            SetError(L"Missing required -ep (encrypted package) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -p (package) option");
            return false;
        }
        if (args.keyFile.empty()) {
            SetError(L"Missing required -kf (key file) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseConvertCGMArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"convertCGM";
                return true;
            }
            else if (arg == L"-s" || arg == L"/s") {
                args.sourceCGM = GetNextArg(index);
                if (args.sourceCGM.empty()) {
                    SetError(L"Missing source CGM path for -s option");
                    return false;
                }
            }
            else if (arg == L"-f" || arg == L"/f") {
                args.targetCGM = GetNextArg(index);
                if (args.targetCGM.empty()) {
                    SetError(L"Missing target CGM path for -f option");
                    return false;
                }
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.sourceCGM.empty()) {
            SetError(L"Missing required -s (source CGM) option");
            return false;
        }
        if (args.targetCGM.empty()) {
            SetError(L"Missing required -f (target CGM) option");
            return false;
        }

        return true;
    }

    bool CommandLineParser::ParseBuildArgs(CommandLineArgs& args, size_t& index) {
        while (index < m_args.size()) {
            std::wstring arg = GetNextArg(index);

            if (arg == L"/?" || arg == L"-help" || arg == L"--help") {
                args.showHelp = true;
                args.specificCommand = L"build";
                return true;
            }
            else if (arg == L"-f" || arg == L"/f") {
                args.layoutFile = GetNextArg(index);
                if (args.layoutFile.empty()) {
                    SetError(L"Missing layout file path for -f option");
                    return false;
                }
            }
            else if (arg == L"-op" || arg == L"/op") {
                args.outputPath = GetNextArg(index);
                if (args.outputPath.empty()) {
                    SetError(L"Missing output path for -op option");
                    return false;
                }
            }
            else if (arg == L"-c" || arg == L"/c") {
                std::wstring compressionStr = GetNextArg(index);
                if (compressionStr == L"none") {
                    args.compression = MakeAppxCore::CompressionLevel::None;
                }
                else if (compressionStr == L"fast") {
                    args.compression = MakeAppxCore::CompressionLevel::Fast;
                }
                else if (compressionStr == L"normal") {
                    args.compression = MakeAppxCore::CompressionLevel::Normal;
                }
                else if (compressionStr == L"max") {
                    args.compression = MakeAppxCore::CompressionLevel::Maximum;
                }
                else {
                    SetError(L"Invalid compression level: " + compressionStr);
                    return false;
                }
            }
            else if (arg == L"-v" || arg == L"/v") {
                args.verbose = true;
            }
            else if (arg == L"-q" || arg == L"/q") {
                args.quiet = true;
            }
            else {
                SetError(L"Unknown option: " + arg);
                return false;
            }
        }

        if (args.layoutFile.empty()) {
            SetError(L"Missing required -f (layout file) option");
            return false;
        }
        if (args.outputPath.empty()) {
            SetError(L"Missing required -op (output path) option");
            return false;
        }

        return true;
    }

    std::wstring CommandLineParser::GetNextArg(size_t& index) {
        if (index >= m_args.size()) {
            return L"";
        }
        return m_args[index++];
    }

    bool CommandLineParser::IsFlag(const std::wstring& arg) {
        return !arg.empty() && (arg[0] == L'-' || arg[0] == L'/');
    }

    void CommandLineParser::SetError(const std::wstring& error) {
        m_lastError = error;
    }

    void CommandLineParser::ShowGeneralHelp() {
        std::wcout << L"MakeAppxPro v1.0 - Enhanced Microsoft App Package Tool" << std::endl;
        std::wcout << L"Copyright (C) 2025. All rights reserved." << std::endl;
        std::wcout << L"Memory-optimized implementation with BCrypt encryption support." << std::endl;
        std::wcout << std::endl;
        std::wcout << L"Usage:" << std::endl;
        std::wcout << L"------" << std::endl;
        std::wcout << L"    MakeAppxPro <command> [options]" << std::endl;
        std::wcout << std::endl;
        std::wcout << L"Valid commands:" << std::endl;
        std::wcout << L"---------------" << std::endl;
        std::wcout << L"    pack        --  Create a new app package from files on disk" << std::endl;
        std::wcout << L"    unpack      --  Extract an existing app package to files on disk" << std::endl;
        std::wcout << L"    bundle      --  Create a new app bundle from files on disk" << std::endl;
        std::wcout << L"    unbundle    --  Extract an existing app bundle to files on disk" << std::endl;
        std::wcout << L"    encrypt     --  Encrypt an existing app package or bundle (AES-256)" << std::endl;
        std::wcout << L"    decrypt     --  Decrypt an existing app package or bundle (AES-256)" << std::endl;
        std::wcout << L"    convertCGM  --  Convert a source content group map (CGM) to the final content group map" << std::endl;
        std::wcout << L"    build       --  Build packages using a packaging layout file" << std::endl;
        std::wcout << std::endl;
        std::wcout << L"For help with a specific command, enter \"MakeAppxPro <command> /?\"" << std::endl;
        std::wcout << std::endl;
        std::wcout << L"Examples:" << std::endl;
        std::wcout << L"    MakeAppxPro pack -d \"C:\\MyApp\" -p \"MyApp.msix\"" << std::endl;
        std::wcout << L"    MakeAppxPro encrypt -p \"MyApp.msix\" -ep \"MyApp.encrypted\" -kf \"key.bin\"" << std::endl;
    }

    void CommandLineParser::ShowCommandHelp(const std::wstring& command) {
        std::wstring cmd = command;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::towlower);

        if (cmd == L"pack") {
            std::wcout << L"Creates a package from files in a directory." << std::endl;
            std::wcout << L"Usage: MakeAppxPro pack [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -d <directory>    Source directory containing files to package" << std::endl;
            std::wcout << L"  -p <package>      Output package file (.appx or .msix)" << std::endl;
            std::wcout << L"  -c <compression>  Compression level: none, fast, normal, max (default: normal)" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"unpack") {
            std::wcout << L"Extracts files from a package to a directory." << std::endl;
            std::wcout << L"Usage: MakeAppxPro unpack [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -p <package>      Source package file (.appx or .msix)" << std::endl;
            std::wcout << L"  -d <directory>    Output directory for extracted files" << std::endl;
            std::wcout << L"  -o                Overwrite existing files without prompting" << std::endl;
            std::wcout << L"  -s                Skip existing files without prompting" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"bundle") {
            std::wcout << L"Creates a bundle from packages in a directory." << std::endl;
            std::wcout << L"Usage: MakeAppxPro bundle [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -d <directory>    Source directory containing .appx/.msix files" << std::endl;
            std::wcout << L"  -p <bundle>       Output bundle file (.appxbundle or .msixbundle)" << std::endl;
            std::wcout << L"  -c <compression>  Compression level: none, fast, normal, max (default: normal)" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"unbundle") {
            std::wcout << L"Extracts packages from a bundle to a directory." << std::endl;
            std::wcout << L"Usage: MakeAppxPro unbundle [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -p <bundle>       Source bundle file (.appxbundle or .msixbundle)" << std::endl;
            std::wcout << L"  -d <directory>    Output directory for extracted packages" << std::endl;
            std::wcout << L"  -o                Overwrite existing files without prompting" << std::endl;
            std::wcout << L"  -s                Skip existing files without prompting" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"encrypt") {
            std::wcout << L"Encrypts a package or bundle using AES-256." << std::endl;
            std::wcout << L"Usage: MakeAppxPro encrypt [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -p <package>      Source package/bundle file" << std::endl;
            std::wcout << L"  -ep <encrypted>   Output encrypted file" << std::endl;
            std::wcout << L"  -kf <keyfile>     Key file (32 bytes for AES-256)" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"decrypt") {
            std::wcout << L"Decrypts an encrypted package or bundle." << std::endl;
            std::wcout << L"Usage: MakeAppxPro decrypt [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -ep <encrypted>   Source encrypted file" << std::endl;
            std::wcout << L"  -p <package>      Output decrypted package/bundle file" << std::endl;
            std::wcout << L"  -kf <keyfile>     Key file (32 bytes for AES-256)" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"convertcgm") {
            std::wcout << L"Converts a source content group map to final format." << std::endl;
            std::wcout << L"Usage: MakeAppxPro convertCGM [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -s <source>       Source CGM file" << std::endl;
            std::wcout << L"  -f <final>        Output final CGM file" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else if (cmd == L"build") {
            std::wcout << L"Builds a package using a layout file." << std::endl;
            std::wcout << L"Usage: MakeAppxPro build [options]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  -f <layoutfile>   Layout file specifying file mappings" << std::endl;
            std::wcout << L"  -op <output>      Output package file" << std::endl;
            std::wcout << L"  -c <compression>  Compression level: none, fast, normal, max (default: normal)" << std::endl;
            std::wcout << L"  -v                Verbose output" << std::endl;
            std::wcout << L"  -q                Quiet mode" << std::endl;
        }
        else {
            std::wcout << L"Unknown command: " << command << std::endl;
            ShowGeneralHelp();
        }
    }

    void ConsoleProgressCallback(const MakeAppxCore::ProgressInfo& progress) {
        static auto lastUpdate = std::chrono::steady_clock::now();
        static bool isComplete = false;
        static bool finalizationMessageShown = false;

        auto now = std::chrono::steady_clock::now();
        bool currentComplete = (progress.processedFiles >= progress.totalFiles);

        if (!currentComplete && !isComplete &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() < 100) {
            return;
        }

        lastUpdate = now;
        isComplete = currentComplete;

        double filePercent = progress.totalFiles > 0 ?
            (double)progress.processedFiles / progress.totalFiles * 100.0 : 0.0;

        const int barWidth = 20;
        int filledWidth = (int)(filePercent / 100.0 * barWidth);

        std::wstringstream ss;

        ss << L"\r[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < filledWidth) {
                ss << L"#";
            }
            else {
                ss << L"-";
            }
        }

        ss << L"] " << std::fixed << std::setprecision(1)
            << filePercent << L"% (" << progress.processedFiles
            << L"/" << progress.totalFiles << L" files, "
            << FormatFileSize(progress.processedBytes) << L"/"
            << FormatFileSize(progress.totalBytes) << L")";

        if (!progress.currentFile.empty() && !currentComplete) {
            std::wstring displayFile = progress.currentFile;
            if (displayFile.length() > 40) {
                displayFile = L"..." + displayFile.substr(displayFile.length() - 37);
            }
            ss << L" - " << displayFile;
        }

        ss << L"                    ";

        std::wcout << ss.str();

        if (currentComplete) {
            std::wcout << std::endl;

            if (!finalizationMessageShown) {
                std::wcout << L"Processing with zlib, this might take a while..." << std::endl;
                finalizationMessageShown = true;
            }

            isComplete = false;
        }

        std::wcout.flush();
    }

    std::wstring FormatFileSize(uint64_t bytes) {
        const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }

        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1) << size << L" " << units[unitIndex];
        return ss.str();
    }

    int ExecuteCommand(const CommandLineArgs& args) {
        try {
            switch (args.command) {
            case Command::Pack: {
                if (!args.quiet) {
                    std::wcout << L"Creating package from: " << args.inputPath << std::endl;
                    std::wcout << L"Output: " << args.outputPath << std::endl;
                }

                auto package = MakeAppxCore::CreateAppxPackage();
                auto callback = args.quiet ? nullptr : ConsoleProgressCallback;

                bool success = package->Pack(args.inputPath, args.outputPath,
                    args.compression, callback);

                if (!args.quiet) {
                    std::wcout << std::endl;
                }

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"Package created successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << package->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::Unpack: {
                if (!args.quiet) {
                    std::wcout << L"Extracting package: " << args.inputPath << std::endl;
                    std::wcout << L"Output directory: " << args.outputPath << std::endl;
                }

                auto package = MakeAppxCore::CreateAppxPackage();
                auto callback = args.quiet ? nullptr : ConsoleProgressCallback;

                bool success = package->Unpack(args.inputPath, args.outputPath,
                    args.overwrite, callback);

                if (!args.quiet) {
                    std::wcout << std::endl;
                }

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"Package extracted successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << package->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::Bundle: {
                if (!args.quiet) {
                    std::wcout << L"Creating bundle from: " << args.inputPath << std::endl;
                    std::wcout << L"Output: " << args.outputPath << std::endl;
                }

                auto bundle = MakeAppxCore::CreateAppxBundle();
                auto callback = args.quiet ? nullptr : ConsoleProgressCallback;

                bool success = bundle->Bundle(args.inputPath, args.outputPath,
                    args.compression, callback);

                if (!args.quiet) {
                    std::wcout << std::endl;
                }

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"Bundle created successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << bundle->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::Unbundle: {
                if (!args.quiet) {
                    std::wcout << L"Extracting bundle: " << args.inputPath << std::endl;
                    std::wcout << L"Output directory: " << args.outputPath << std::endl;
                }

                auto bundle = MakeAppxCore::CreateAppxBundle();
                auto callback = args.quiet ? nullptr : ConsoleProgressCallback;

                bool success = bundle->Unbundle(args.inputPath, args.outputPath,
                    args.overwrite, callback);

                if (!args.quiet) {
                    std::wcout << std::endl;
                }

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"Bundle extracted successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << bundle->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::Encrypt: {
                if (!args.quiet) {
                    std::wcout << L"Encrypting: " << args.inputPath << std::endl;
                    std::wcout << L"Output: " << args.outputPath << std::endl;
                    std::wcout << L"Using key file: " << args.keyFile << std::endl;
                }

                auto package = MakeAppxCore::CreateAppxPackage();
                bool success = package->Encrypt(args.inputPath, args.outputPath, args.keyFile);

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"File encrypted successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << package->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::Decrypt: {
                if (!args.quiet) {
                    std::wcout << L"Decrypting: " << args.inputPath << std::endl;
                    std::wcout << L"Output: " << args.outputPath << std::endl;
                    std::wcout << L"Using key file: " << args.keyFile << std::endl;
                }

                auto package = MakeAppxCore::CreateAppxPackage();
                bool success = package->Decrypt(args.inputPath, args.outputPath, args.keyFile);

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"File decrypted successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << package->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::ConvertCGM: {
                if (!args.quiet) {
                    std::wcout << L"Converting CGM: " << args.sourceCGM << std::endl;
                    std::wcout << L"Output: " << args.targetCGM << std::endl;
                }

                auto builder = MakeAppxCore::CreateAppxBuilder();
                bool success = builder->ConvertCGM(args.sourceCGM, args.targetCGM);

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"CGM converted successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << builder->GetLastError() << std::endl;
                    return 1;
                }
            }

            case Command::Build: {
                if (!args.quiet) {
                    std::wcout << L"Building package from layout: " << args.layoutFile << std::endl;
                    std::wcout << L"Output: " << args.outputPath << std::endl;
                }

                MakeAppxCore::BuildOptions buildOpts;
                buildOpts.layoutFile = args.layoutFile;
                buildOpts.outputPath = args.outputPath;
                buildOpts.compression = args.compression;
                buildOpts.verbose = args.verbose;

                auto builder = MakeAppxCore::CreateAppxBuilder();
                bool success = builder->Build(buildOpts);

                if (success) {
                    if (!args.quiet) {
                        std::wcout << L"Package built successfully." << std::endl;
                    }
                    return 0;
                }
                else {
                    std::wcerr << L"Error: " << builder->GetLastError() << std::endl;
                    return 1;
                }
            }

            default:
                std::wcerr << L"Error: Unknown command" << std::endl;
                return 1;
            }
        }
        catch (const std::exception& e) {
            std::wcerr << L"Unexpected error: ";
            std::wcerr << e.what() << std::endl;
            return 1;
        }
        catch (...) {
            std::wcerr << L"Unexpected error occurred" << std::endl;
            return 1;
        }
    }

}