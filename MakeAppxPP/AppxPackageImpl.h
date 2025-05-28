#pragma once
#include "AppxPackage.h"
#include <zip.h>
#include <memory>
#include <filesystem>
#include <vector>

namespace MakeAppxCore {

    namespace fs = std::filesystem;

    std::string WideToUtf8Safe(const std::wstring& wstr);
    std::wstring Utf8ToWideSafe(const std::string& str);

    class AppxPackageImpl : public IAppxPackage {
    private:
        std::wstring m_lastError;
        static constexpr size_t BUFFER_SIZE = 8192;

        bool ValidateManifest(const std::wstring& manifestPath);
        bool ProcessFileTree(const std::wstring& rootPath,
            std::vector<PackageFile>& files);
        void SetError(const std::wstring& error);
        std::wstring WideToUtf8(const std::wstring& wide);
        std::wstring Utf8ToWide(const std::string& utf8);
        bool PromptUserOverwrite(const std::wstring& filePath);

    public:
        AppxPackageImpl() = default;
        ~AppxPackageImpl() = default;

        bool Pack(const std::wstring& inputPath, const std::wstring& outputPath,
            CompressionLevel compression = CompressionLevel::Normal,
            ProgressCallback callback = nullptr) override;

        bool Unpack(const std::wstring& inputPath, const std::wstring& outputPath,
            OverwriteMode overwrite = OverwriteMode::Ask,
            ProgressCallback callback = nullptr) override;

        bool Encrypt(const std::wstring& inputPath, const std::wstring& outputPath,
            const std::wstring& keyFile) override;

        bool Decrypt(const std::wstring& inputPath, const std::wstring& outputPath,
            const std::wstring& keyFile) override;

        std::wstring GetLastError() const override { return m_lastError; }
    };

    class AppxBundleImpl : public IAppxBundle {
    private:
        std::wstring m_lastError;
        void SetError(const std::wstring& error);
        std::wstring GenerateBundleManifest(const std::vector<fs::path>& packageFiles);
        std::wstring ExtractPackageIdentity(const fs::path& packagePath);
        bool PromptUserOverwrite(const std::wstring& filePath);

    public:
        AppxBundleImpl() = default;
        ~AppxBundleImpl() = default;

        bool Bundle(const std::wstring& inputPath, const std::wstring& outputPath,
            CompressionLevel compression = CompressionLevel::Normal,
            ProgressCallback callback = nullptr) override;

        bool Unbundle(const std::wstring& inputPath, const std::wstring& outputPath,
            OverwriteMode overwrite = OverwriteMode::Ask,
            ProgressCallback callback = nullptr) override;

        std::wstring GetLastError() const override { return m_lastError; }
    };

    class AppxBuilderImpl : public IAppxBuilder {
    private:
        std::wstring m_lastError;
        void SetError(const std::wstring& error);
        bool ParseLayoutFile(const std::wstring& layoutFile, std::vector<PackageFile>& files);

        bool ValidateCGMContent(const std::wstring& content);
        std::wstring TransformCGMContent(const std::wstring& sourceContent);
        std::wstring ParseAndTransformContentGroups(const std::wstring& sourceContent);
        std::wstring TransformFilesSection(const std::wstring& filesSection);
        std::wstring ExtractAttribute(const std::wstring& xmlElement, const std::wstring& attributeName);

    public:
        AppxBuilderImpl() = default;
        ~AppxBuilderImpl() = default;

        bool Build(const BuildOptions& options, ProgressCallback callback = nullptr) override;
        bool ConvertCGM(const std::wstring& sourceCGM, const std::wstring& outputCGM) override;
        std::wstring GetLastError() const override { return m_lastError; }
    };
}