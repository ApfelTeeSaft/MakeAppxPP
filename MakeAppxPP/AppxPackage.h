#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <filesystem>

namespace MakeAppxCore {

    enum class CompressionLevel {
        None = 0,
        Fast = 1,
        Normal = 2,
        Maximum = 3
    };

    enum class OverwriteMode {
        Ask,
        Yes,
        No
    };

    struct PackageFile {
        std::wstring localPath;
        std::wstring packagePath;
        uint64_t size;
        uint32_t attributes;
    };

    struct ProgressInfo {
        uint64_t totalFiles;
        uint64_t processedFiles;
        uint64_t totalBytes;
        uint64_t processedBytes;
        std::wstring currentFile;
    };

    using ProgressCallback = std::function<void(const ProgressInfo&)>;

    struct BuildOptions {
        std::wstring layoutFile;
        std::wstring outputPath;
        CompressionLevel compression = CompressionLevel::Normal;
        bool verbose = false;
    };

    class IAppxBuilder {
    public:
        virtual ~IAppxBuilder() = default;
        virtual bool Build(const BuildOptions& options, ProgressCallback callback = nullptr) = 0;
        virtual bool ConvertCGM(const std::wstring& sourceCGM, const std::wstring& outputCGM) = 0;
        virtual std::wstring GetLastError() const = 0;
    };

    std::unique_ptr<IAppxBuilder> CreateAppxBuilder();

    class IAppxPackage {
    public:
        virtual ~IAppxPackage() = default;
        virtual bool Pack(const std::wstring& inputPath, const std::wstring& outputPath,
            CompressionLevel compression = CompressionLevel::Normal,
            ProgressCallback callback = nullptr) = 0;
        virtual bool Unpack(const std::wstring& inputPath, const std::wstring& outputPath,
            OverwriteMode overwrite = OverwriteMode::Ask,
            ProgressCallback callback = nullptr) = 0;
        virtual bool Encrypt(const std::wstring& inputPath, const std::wstring& outputPath,
            const std::wstring& keyFile) = 0;
        virtual bool Decrypt(const std::wstring& inputPath, const std::wstring& outputPath,
            const std::wstring& keyFile) = 0;
        virtual std::wstring GetLastError() const = 0;
    };

    class IAppxBundle {
    public:
        virtual ~IAppxBundle() = default;
        virtual bool Bundle(const std::wstring& inputPath, const std::wstring& outputPath,
            CompressionLevel compression = CompressionLevel::Normal,
            ProgressCallback callback = nullptr) = 0;
        virtual bool Unbundle(const std::wstring& inputPath, const std::wstring& outputPath,
            OverwriteMode overwrite = OverwriteMode::Ask,
            ProgressCallback callback = nullptr) = 0;
        virtual std::wstring GetLastError() const = 0;
    };

    std::unique_ptr<IAppxPackage> CreateAppxPackage();
    std::unique_ptr<IAppxBundle> CreateAppxBundle();
}