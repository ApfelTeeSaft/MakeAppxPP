#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "AppxPackage.h"

namespace MakeAppxPP {

    enum class Command {
        None,
        Pack,
        Unpack,
        Bundle,
        Unbundle,
        Encrypt,
        Decrypt,
        ConvertCGM,
        Build,
        Help
    };

    struct CommandLineArgs {
        Command command = Command::None;
        std::wstring inputPath;
        std::wstring outputPath;
        std::wstring layoutFile;
        std::wstring keyFile;
        std::wstring sourceCGM;
        std::wstring targetCGM;
        MakeAppxCore::CompressionLevel compression = MakeAppxCore::CompressionLevel::Normal;
        MakeAppxCore::OverwriteMode overwrite = MakeAppxCore::OverwriteMode::Ask;
        bool verbose = false;
        bool quiet = false;
        bool showHelp = false;
        std::wstring specificCommand;
    };

    class CommandLineParser {
    private:
        std::vector<std::wstring> m_args;
        std::wstring m_lastError;

        Command ParseCommand(const std::wstring& cmdStr);
        bool ParsePackArgs(CommandLineArgs& args, size_t& index);
        bool ParseUnpackArgs(CommandLineArgs& args, size_t& index);
        bool ParseBundleArgs(CommandLineArgs& args, size_t& index);
        bool ParseUnbundleArgs(CommandLineArgs& args, size_t& index);
        bool ParseEncryptArgs(CommandLineArgs& args, size_t& index);
        bool ParseDecryptArgs(CommandLineArgs& args, size_t& index);
        bool ParseConvertCGMArgs(CommandLineArgs& args, size_t& index);
        bool ParseBuildArgs(CommandLineArgs& args, size_t& index);

        std::wstring GetNextArg(size_t& index);
        bool IsFlag(const std::wstring& arg);
        void SetError(const std::wstring& error);

    public:
        CommandLineParser() = default;
        ~CommandLineParser() = default;

        bool Parse(int argc, wchar_t* argv[], CommandLineArgs& args);
        std::wstring GetLastError() const { return m_lastError; }

        static void ShowGeneralHelp();
        static void ShowCommandHelp(const std::wstring& command);
    };

    void ConsoleProgressCallback(const MakeAppxCore::ProgressInfo& progress);

    std::wstring FormatFileSize(uint64_t bytes);
    int ExecuteCommand(const CommandLineArgs& args);
}