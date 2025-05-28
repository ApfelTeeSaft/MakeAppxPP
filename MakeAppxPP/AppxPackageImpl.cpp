#include "AppxPackageImpl.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <Windows.h>
#include <bcrypt.h>
#include <ntstatus.h>
#include <random>
#include <iostream>

#pragma comment(lib, "bcrypt.lib")

namespace fs = std::filesystem;

namespace MakeAppxCore {

    std::unique_ptr<IAppxPackage> CreateAppxPackage() {
        return std::make_unique<AppxPackageImpl>();
    }

    std::unique_ptr<IAppxBundle> CreateAppxBundle() {
        return std::make_unique<AppxBundleImpl>();
    }

    std::unique_ptr<IAppxBuilder> CreateAppxBuilder() {
        return std::make_unique<AppxBuilderImpl>();
    }

    void AppxPackageImpl::SetError(const std::wstring& error) {
        m_lastError = error;
    }

    std::wstring AppxPackageImpl::WideToUtf8(const std::wstring& wide) {
        if (wide.empty()) return std::wstring();
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string narrow = converter.to_bytes(wide);
        return std::wstring(narrow.begin(), narrow.end());
    }

    std::wstring AppxPackageImpl::Utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return std::wstring();
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(utf8);
    }

    bool AppxPackageImpl::PromptUserOverwrite(const std::wstring& filePath) {
        std::wcout << L"File exists: " << filePath << std::endl;
        std::wcout << L"Overwrite? (y)es, (n)o, (a)ll, (s)kip all: ";

        wchar_t response;
        std::wcin >> response;
        response = towlower(response);

        switch (response) {
        case L'y':
            return true;
        case L'n':
            return false;
        case L'a':
            return true;
        case L's':
            return false;
        default:
            std::wcout << L"Invalid response. Skipping file." << std::endl;
            return false;
        }
    }

    bool AppxPackageImpl::ValidateManifest(const std::wstring& manifestPath) {
        if (!fs::exists(manifestPath)) {
            SetError(L"AppxManifest.xml not found");
            return false;
        }

        std::ifstream file(manifestPath);
        if (!file.is_open()) {
            SetError(L"Cannot open AppxManifest.xml");
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        if (content.find("<Package") == std::string::npos) {
            SetError(L"Invalid AppxManifest.xml - missing Package element");
            return false;
        }

        return true;
    }

    std::wstring AppxBundleImpl::GenerateBundleManifest(const std::vector<fs::path>& packageFiles) {
        std::wstringstream manifest;

        manifest << L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
        manifest << L"<Bundle xmlns=\"http://schemas.microsoft.com/appx/2013/bundle\" \n";
        manifest << L"        xmlns:b4=\"http://schemas.microsoft.com/appx/2018/bundle\" \n";
        manifest << L"        SchemaVersion=\"4.0.0.0\">\n";
        manifest << L"  <Identity Name=\"BundleIdentity\" \n";
        manifest << L"            Publisher=\"CN=Publisher\" \n";
        manifest << L"            Version=\"1.0.0.0\" />\n";
        manifest << L"  <Packages>\n";

        for (const auto& packageFile : packageFiles) {
            std::wstring identity = ExtractPackageIdentity(packageFile);
            if (!identity.empty()) {
                manifest << L"    <Package Type=\"application\" Version=\"1.0.0.0\" Architecture=\"x64\">\n";
                manifest << L"      <Resources>\n";
                manifest << L"        <Resource Language=\"en-US\" />\n";
                manifest << L"      </Resources>\n";
                manifest << L"      <File Name=\"" << packageFile.filename().wstring() << L"\" />\n";
                manifest << L"    </Package>\n";
            }
        }

        manifest << L"  </Packages>\n";
        manifest << L"</Bundle>\n";

        return manifest.str();
    }

    std::wstring AppxBundleImpl::ExtractPackageIdentity(const fs::path& packagePath) {
        return L"Package_" + packagePath.stem().wstring();
    }

    bool AppxBundleImpl::PromptUserOverwrite(const std::wstring& filePath) {
        std::wcout << L"File exists: " << filePath << std::endl;
        std::wcout << L"Overwrite? (y)es, (n)o, (a)ll, (s)kip all: ";

        wchar_t response;
        std::wcin >> response;
        response = towlower(response);

        switch (response) {
        case L'y':
            return true;
        case L'n':
            return false;
        case L'a':
            return true;
        case L's':
            return false;
        default:
            std::wcout << L"Invalid response. Skipping file." << std::endl;
            return false;
        }
    }

    void AppxBuilderImpl::SetError(const std::wstring& error) {
        m_lastError = error;
    }

    std::wstring Utf8ToWideString(const std::string& utf8) {
        if (utf8.empty()) return std::wstring();
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(utf8);
    }

    bool AppxBuilderImpl::Build(const BuildOptions& options, ProgressCallback callback) {
        if (!fs::exists(options.layoutFile)) {
            SetError(L"Layout file does not exist: " + options.layoutFile);
            return false;
        }

        std::vector<PackageFile> files;
        if (!ParseLayoutFile(options.layoutFile, files)) {
            return false;
        }

        auto package = CreateAppxPackage();

        std::wstring tempDir = fs::temp_directory_path().wstring() + L"\\MakeAppxBuild_" +
            std::to_wstring(GetCurrentProcessId());

        try {
            fs::create_directories(tempDir);

            for (const auto& file : files) {
                fs::path destPath = fs::path(tempDir) / file.packagePath;
                fs::create_directories(destPath.parent_path());
                fs::copy_file(file.localPath, destPath, fs::copy_options::overwrite_existing);
            }

            bool result = package->Pack(tempDir, options.outputPath, options.compression, callback);

            fs::remove_all(tempDir);

            if (!result) {
                SetError(package->GetLastError());
            }

            return result;
        }
        catch (const std::exception& e) {
            try {
                fs::remove_all(tempDir);
            }
            catch (...) {
            }

            SetError(L"Build failed: " + Utf8ToWideString(e.what()));
            return false;
        }
    }

    bool AppxBuilderImpl::ParseLayoutFile(const std::wstring& layoutFile, std::vector<PackageFile>& files) {
        std::wifstream file(layoutFile);
        if (!file.is_open()) {
            SetError(L"Cannot open layout file");
            return false;
        }

        std::wstring line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == L'#') continue;

            size_t firstQuote = line.find(L'"');
            if (firstQuote == std::wstring::npos) continue;

            size_t secondQuote = line.find(L'"', firstQuote + 1);
            if (secondQuote == std::wstring::npos) continue;

            size_t thirdQuote = line.find(L'"', secondQuote + 1);
            if (thirdQuote == std::wstring::npos) continue;

            size_t fourthQuote = line.find(L'"', thirdQuote + 1);
            if (fourthQuote == std::wstring::npos) continue;

            PackageFile pf;
            pf.localPath = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            pf.packagePath = line.substr(thirdQuote + 1, fourthQuote - thirdQuote - 1);

            if (fs::exists(pf.localPath)) {
                try {
                    pf.size = fs::file_size(pf.localPath);
                    files.push_back(pf);
                }
                catch (...) {
                    continue;
                }
            }
        }

        return !files.empty();
    }

    bool AppxBuilderImpl::ConvertCGM(const std::wstring& sourceCGM, const std::wstring& outputCGM) {
        if (!fs::exists(sourceCGM)) {
            SetError(L"Source CGM file does not exist");
            return false;
        }

        try {
            std::wifstream sourceFile(sourceCGM);
            if (!sourceFile.is_open()) {
                SetError(L"Cannot open source CGM file");
                return false;
            }

            std::wstring content((std::istreambuf_iterator<wchar_t>(sourceFile)),
                std::istreambuf_iterator<wchar_t>());
            sourceFile.close();

            if (!ValidateCGMContent(content)) {
                return false;
            }

            std::wstring convertedContent = TransformCGMContent(content);
            if (convertedContent.empty()) {
                SetError(L"CGM transformation failed");
                return false;
            }

            std::wofstream outputFile(outputCGM);
            if (!outputFile.is_open()) {
                SetError(L"Cannot create output CGM file");
                return false;
            }

            outputFile << convertedContent;
            outputFile.close();

            return true;
        }
        catch (const std::exception& e) {
            SetError(L"CGM conversion failed: " + Utf8ToWideString(e.what()));
            return false;
        }
    }

    bool AppxBuilderImpl::ValidateCGMContent(const std::wstring& content) {
        if (content.find(L"<ContentGroupMap") == std::wstring::npos) {
            SetError(L"Invalid source CGM - missing ContentGroupMap element");
            return false;
        }

        if (content.find(L"<Automatic") == std::wstring::npos &&
            content.find(L"<Required") == std::wstring::npos &&
            content.find(L"<Optional") == std::wstring::npos) {
            SetError(L"Invalid source CGM - no content groups defined");
            return false;
        }

        return true;
    }

    std::wstring AppxBuilderImpl::TransformCGMContent(const std::wstring& sourceContent) {
        std::wstringstream result;

        result << L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        result << L"<ContentGroupMap xmlns=\"http://schemas.microsoft.com/appx/2016/contentgroupmap\"\n";
        result << L"                xmlns:s=\"http://schemas.microsoft.com/appx/2016/sourcecgm\">\n";

        std::wstring transformedGroups = ParseAndTransformContentGroups(sourceContent);
        result << transformedGroups;

        result << L"</ContentGroupMap>\n";

        return result.str();
    }

    std::wstring AppxBuilderImpl::ParseAndTransformContentGroups(const std::wstring& sourceContent) {
        std::wstringstream result;

        size_t pos = 0;
        std::wstring searchStr = L"<Automatic";
        while ((pos = sourceContent.find(searchStr, pos)) != std::wstring::npos) {
            size_t endPos = sourceContent.find(L">", pos);
            if (endPos == std::wstring::npos) break;

            std::wstring groupDef = sourceContent.substr(pos, endPos - pos + 1);
            result << L"  <Automatic>\n";

            size_t filesStart = sourceContent.find(L"<Files", endPos);
            size_t filesEnd = sourceContent.find(L"</Files>", filesStart);

            if (filesStart != std::wstring::npos && filesEnd != std::wstring::npos) {
                std::wstring filesSection = sourceContent.substr(filesStart, filesEnd - filesStart + 8);
                result << L"    " << TransformFilesSection(filesSection) << L"\n";
            }

            result << L"  </Automatic>\n";
            pos = endPos + 1;
        }

        pos = 0;
        searchStr = L"<Required";
        while ((pos = sourceContent.find(searchStr, pos)) != std::wstring::npos) {
            size_t endPos = sourceContent.find(L">", pos);
            if (endPos == std::wstring::npos) break;

            result << L"  <Required>\n";

            size_t filesStart = sourceContent.find(L"<Files", endPos);
            size_t filesEnd = sourceContent.find(L"</Files>", filesStart);

            if (filesStart != std::wstring::npos && filesEnd != std::wstring::npos) {
                std::wstring filesSection = sourceContent.substr(filesStart, filesEnd - filesStart + 8);
                result << L"    " << TransformFilesSection(filesSection) << L"\n";
            }

            result << L"  </Required>\n";
            pos = endPos + 1;
        }

        pos = 0;
        searchStr = L"<Optional";
        while ((pos = sourceContent.find(searchStr, pos)) != std::wstring::npos) {
            size_t endPos = sourceContent.find(L">", pos);
            if (endPos == std::wstring::npos) break;

            std::wstring groupName = ExtractAttribute(sourceContent.substr(pos, endPos - pos), L"Name");

            result << L"  <Optional";
            if (!groupName.empty()) {
                result << L" Name=\"" << groupName << L"\"";
            }
            result << L">\n";

            size_t filesStart = sourceContent.find(L"<Files", endPos);
            size_t filesEnd = sourceContent.find(L"</Files>", filesStart);

            if (filesStart != std::wstring::npos && filesEnd != std::wstring::npos) {
                std::wstring filesSection = sourceContent.substr(filesStart, filesEnd - filesStart + 8);
                result << L"    " << TransformFilesSection(filesSection) << L"\n";
            }

            result << L"  </Optional>\n";
            pos = endPos + 1;
        }

        return result.str();
    }

    std::wstring AppxBuilderImpl::TransformFilesSection(const std::wstring& filesSection) {
        std::wstringstream result;
        result << L"<Files>\n";

        size_t pos = 0;
        std::wstring searchStr = L"<File";
        while ((pos = filesSection.find(searchStr, pos)) != std::wstring::npos) {
            size_t endPos = filesSection.find(L"/>", pos);
            if (endPos == std::wstring::npos) {
                endPos = filesSection.find(L">", pos);
                if (endPos == std::wstring::npos) break;
            }

            std::wstring fileDef = filesSection.substr(pos, endPos - pos + 2);

            std::wstring fileName = ExtractAttribute(fileDef, L"Name");
            if (!fileName.empty()) {
                result << L"      <File Name=\"" << fileName << L"\" />\n";
            }

            pos = endPos + 2;
        }

        result << L"    </Files>";
        return result.str();
    }

    std::wstring AppxBuilderImpl::ExtractAttribute(const std::wstring& xmlElement, const std::wstring& attributeName) {
        std::wstring searchStr = attributeName + L"=\"";
        size_t pos = xmlElement.find(searchStr);
        if (pos == std::wstring::npos) return L"";

        pos += searchStr.length();
        size_t endPos = xmlElement.find(L"\"", pos);
        if (endPos == std::wstring::npos) return L"";

        return xmlElement.substr(pos, endPos - pos);
    }

    bool AppxPackageImpl::ProcessFileTree(const std::wstring& rootPath,
        std::vector<PackageFile>& files) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
                if (entry.is_regular_file()) {
                    PackageFile pf;
                    pf.localPath = entry.path().wstring();
                    pf.packagePath = fs::relative(entry.path(), rootPath).wstring();
                    pf.size = entry.file_size();
                    pf.attributes = static_cast<uint32_t>(entry.status().permissions());
                    files.push_back(pf);
                }
            }
            return true;
        }
        catch (const std::exception& e) {
            std::wstring error = L"Error processing file tree: ";
            error += Utf8ToWide(e.what());
            SetError(error);
            return false;
        }
    }

    bool AppxPackageImpl::Pack(const std::wstring& inputPath, const std::wstring& outputPath,
        CompressionLevel compression, ProgressCallback callback) {

        if (!fs::exists(inputPath) || !fs::is_directory(inputPath)) {
            SetError(L"Input path does not exist or is not a directory");
            return false;
        }

        std::wstring manifestPath = inputPath + L"\\AppxManifest.xml";
        if (!ValidateManifest(manifestPath)) {
            return false;
        }

        fs::path outputDir = fs::path(outputPath).parent_path();
        if (!outputDir.empty() && !fs::exists(outputDir)) {
            fs::create_directories(outputDir);
        }

        std::vector<PackageFile> files;
        if (!ProcessFileTree(inputPath, files)) {
            return false;
        }

        int compressionMethod = ZIP_CM_DEFAULT;
        switch (compression) {
        case CompressionLevel::None: compressionMethod = ZIP_CM_STORE; break;
        case CompressionLevel::Fast: compressionMethod = ZIP_CM_DEFLATE; break;
        case CompressionLevel::Normal: compressionMethod = ZIP_CM_DEFLATE; break;
        case CompressionLevel::Maximum: compressionMethod = ZIP_CM_DEFLATE; break;
        }

        std::string outputPathUtf8 = std::string(outputPath.begin(), outputPath.end());
        ZipPtr zip(zip_open(outputPathUtf8.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr));

        if (!zip) {
            SetError(L"Failed to create output package");
            return false;
        }

        ProgressInfo progress = {};
        progress.totalFiles = files.size();
        progress.totalBytes = 0;

        for (const auto& file : files) {
            progress.totalBytes += file.size;
        }

        uint64_t processedBytes = 0;
        for (size_t i = 0; i < files.size(); ++i) {
            const auto& file = files[i];

            if (callback) {
                progress.processedFiles = i;
                progress.processedBytes = processedBytes;
                progress.currentFile = file.packagePath;
                callback(progress);
            }

            std::string packagePathUtf8(file.packagePath.begin(), file.packagePath.end());
            std::string localPathUtf8(file.localPath.begin(), file.localPath.end());

            zip_source_t* source = zip_source_file(zip.get(), localPathUtf8.c_str(), 0, -1);
            if (!source) {
                SetError(L"Failed to create source for file: " + file.packagePath);
                return false;
            }

            zip_int64_t index = zip_file_add(zip.get(), packagePathUtf8.c_str(), source, ZIP_FL_OVERWRITE);
            if (index < 0) {
                zip_source_free(source);
                SetError(L"Failed to add file to package: " + file.packagePath);
                return false;
            }

            zip_set_file_compression(zip.get(), index, compressionMethod, 0);

            processedBytes += file.size;
        }

        if (callback) {
            progress.processedFiles = files.size();
            progress.processedBytes = processedBytes;
            progress.currentFile = L"Finalizing...";
            callback(progress);
        }

        return true;
    }

    bool AppxPackageImpl::Unpack(const std::wstring& inputPath, const std::wstring& outputPath,
        OverwriteMode overwrite, ProgressCallback callback) {

        std::string inputPathUtf8(inputPath.begin(), inputPath.end());
        ZipPtr zip(zip_open(inputPathUtf8.c_str(), ZIP_RDONLY, nullptr));

        if (!zip) {
            SetError(L"Failed to open package file");
            return false;
        }

        if (!fs::exists(outputPath)) {
            fs::create_directories(outputPath);
        }

        zip_int64_t numEntries = zip_get_num_entries(zip.get(), 0);
        if (numEntries < 0) {
            SetError(L"Failed to get package contents");
            return false;
        }

        ProgressInfo progress = {};
        progress.totalFiles = static_cast<uint64_t>(numEntries);
        progress.totalBytes = 0;

        for (zip_int64_t i = 0; i < numEntries; ++i) {
            const char* name = zip_get_name(zip.get(), i, 0);
            if (!name) continue;

            std::wstring fileName = Utf8ToWide(name);
            std::wstring fullPath = outputPath + L"\\" + fileName;

            if (callback) {
                progress.processedFiles = static_cast<uint64_t>(i);
                progress.currentFile = fileName;
                callback(progress);
            }

            if (fs::exists(fullPath)) {
                if (overwrite == OverwriteMode::No) continue;
                if (overwrite == OverwriteMode::Ask) {
                    if (!PromptUserOverwrite(fileName)) {
                        continue;
                    }
                }
            }

            fs::path filePath(fullPath);
            if (filePath.has_parent_path()) {
                fs::create_directories(filePath.parent_path());
            }

            zip_file_t* zipFile = zip_fopen_index(zip.get(), i, 0);
            if (!zipFile) continue;

            std::ofstream outFile(fullPath, std::ios::binary);
            if (!outFile.is_open()) {
                zip_fclose(zipFile);
                continue;
            }

            char buffer[BUFFER_SIZE];
            zip_int64_t bytesRead;
            while ((bytesRead = zip_fread(zipFile, buffer, BUFFER_SIZE)) > 0) {
                outFile.write(buffer, bytesRead);
                progress.processedBytes += static_cast<uint64_t>(bytesRead);
            }

            outFile.close();
            zip_fclose(zipFile);
        }

        if (callback) {
            progress.processedFiles = static_cast<uint64_t>(numEntries);
            progress.currentFile = L"Complete";
            callback(progress);
        }

        return true;
    }

    bool AppxPackageImpl::Encrypt(const std::wstring& inputPath, const std::wstring& outputPath,
        const std::wstring& keyFile) {

        if (!fs::exists(inputPath)) {
            SetError(L"Input package file does not exist");
            return false;
        }

        if (!fs::exists(keyFile)) {
            SetError(L"Key file does not exist");
            return false;
        }

        try {
            std::ifstream keyFileStream(keyFile, std::ios::binary);
            if (!keyFileStream.is_open()) {
                SetError(L"Cannot open key file");
                return false;
            }

            std::vector<uint8_t> keyData((std::istreambuf_iterator<char>(keyFileStream)),
                std::istreambuf_iterator<char>());
            keyFileStream.close();

            if (keyData.size() != 32) {
                SetError(L"Invalid key file - must be exactly 32 bytes for AES-256");
                return false;
            }

            BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
            BCRYPT_KEY_HANDLE hKey = nullptr;
            NTSTATUS status;

            status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0);
            if (!BCRYPT_SUCCESS(status)) {
                SetError(L"Failed to open AES algorithm provider");
                return false;
            }

            status = BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE,
                (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
            if (!BCRYPT_SUCCESS(status)) {
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Failed to set chaining mode");
                return false;
            }
            status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, nullptr, 0,
                keyData.data(), 32, 0);
            if (!BCRYPT_SUCCESS(status)) {
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Failed to generate symmetric key");
                return false;
            }
            uint8_t iv[16];
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint8_t> dis(0, 255);
            for (int i = 0; i < 16; ++i) {
                iv[i] = dis(gen);
            }

            std::ifstream inputFile(inputPath, std::ios::binary);
            std::ofstream outputFile(outputPath, std::ios::binary);

            if (!inputFile.is_open() || !outputFile.is_open()) {
                BCryptDestroyKey(hKey);
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Failed to open input or output file");
                return false;
            }

            outputFile.write(reinterpret_cast<char*>(iv), 16);

            const size_t CHUNK_SIZE = 8192;
            uint8_t inputBuffer[CHUNK_SIZE];
            uint8_t outputBuffer[CHUNK_SIZE + 16];
            ULONG bytesEncrypted;

            while (inputFile.read(reinterpret_cast<char*>(inputBuffer), CHUNK_SIZE)) {
                std::streamsize bytesRead = inputFile.gcount();

                if (bytesRead < CHUNK_SIZE) {
                    size_t padSize = 16 - (bytesRead % 16);
                    if (padSize == 16 && bytesRead % 16 == 0) padSize = 0;
                    for (size_t i = 0; i < padSize; ++i) {
                        inputBuffer[bytesRead + i] = static_cast<uint8_t>(padSize);
                    }
                    bytesRead += padSize;
                }

                status = BCryptEncrypt(hKey, inputBuffer, static_cast<ULONG>(bytesRead),
                    nullptr, iv, 16, outputBuffer, sizeof(outputBuffer),
                    &bytesEncrypted, 0);

                if (!BCRYPT_SUCCESS(status)) {
                    BCryptDestroyKey(hKey);
                    BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                    SetError(L"Encryption failed");
                    return false;
                }

                outputFile.write(reinterpret_cast<char*>(outputBuffer), bytesEncrypted);

                memcpy(iv, outputBuffer + bytesEncrypted - 16, 16);
            }

            inputFile.close();
            outputFile.close();
            BCryptDestroyKey(hKey);
            BCryptCloseAlgorithmProvider(hAlgorithm, 0);

            return true;
        }
        catch (const std::exception& e) {
            SetError(L"Encryption failed: " + Utf8ToWide(e.what()));
            return false;
        }
    }

    bool AppxPackageImpl::Decrypt(const std::wstring& inputPath, const std::wstring& outputPath,
        const std::wstring& keyFile) {

        if (!fs::exists(inputPath)) {
            SetError(L"Input encrypted file does not exist");
            return false;
        }

        if (!fs::exists(keyFile)) {
            SetError(L"Key file does not exist");
            return false;
        }

        try {
            std::ifstream keyFileStream(keyFile, std::ios::binary);
            if (!keyFileStream.is_open()) {
                SetError(L"Cannot open key file");
                return false;
            }

            std::vector<uint8_t> keyData((std::istreambuf_iterator<char>(keyFileStream)),
                std::istreambuf_iterator<char>());
            keyFileStream.close();

            if (keyData.size() != 32) {
                SetError(L"Invalid key file - must be exactly 32 bytes for AES-256");
                return false;
            }

            BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
            BCRYPT_KEY_HANDLE hKey = nullptr;
            NTSTATUS status;

            status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0);
            if (!BCRYPT_SUCCESS(status)) {
                SetError(L"Failed to open AES algorithm provider");
                return false;
            }

            status = BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE,
                (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
            if (!BCRYPT_SUCCESS(status)) {
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Failed to set chaining mode");
                return false;
            }

            status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, nullptr, 0,
                keyData.data(), 32, 0);
            if (!BCRYPT_SUCCESS(status)) {
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Failed to generate symmetric key");
                return false;
            }

            std::ifstream inputFile(inputPath, std::ios::binary);
            std::ofstream outputFile(outputPath, std::ios::binary);

            if (!inputFile.is_open() || !outputFile.is_open()) {
                BCryptDestroyKey(hKey);
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Failed to open input or output file");
                return false;
            }

            uint8_t iv[16];
            inputFile.read(reinterpret_cast<char*>(iv), 16);
            if (inputFile.gcount() != 16) {
                BCryptDestroyKey(hKey);
                BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                SetError(L"Invalid encrypted file - missing IV");
                return false;
            }

            const size_t CHUNK_SIZE = 8192;
            uint8_t inputBuffer[CHUNK_SIZE];
            uint8_t outputBuffer[CHUNK_SIZE];
            ULONG bytesDecrypted;
            bool isLastChunk = false;

            while (inputFile.read(reinterpret_cast<char*>(inputBuffer), CHUNK_SIZE)) {
                std::streamsize bytesRead = inputFile.gcount();

                if (bytesRead < CHUNK_SIZE) {
                    isLastChunk = true;
                }

                status = BCryptDecrypt(hKey, inputBuffer, static_cast<ULONG>(bytesRead),
                    nullptr, iv, 16, outputBuffer, sizeof(outputBuffer),
                    &bytesDecrypted, 0);

                if (!BCRYPT_SUCCESS(status)) {
                    BCryptDestroyKey(hKey);
                    BCryptCloseAlgorithmProvider(hAlgorithm, 0);
                    SetError(L"Decryption failed");
                    return false;
                }

                if (isLastChunk && bytesDecrypted > 0) {
                    uint8_t padSize = outputBuffer[bytesDecrypted - 1];
                    if (padSize <= 16 && padSize <= bytesDecrypted) {
                        bytesDecrypted -= padSize;
                    }
                }

                if (bytesDecrypted > 0) {
                    outputFile.write(reinterpret_cast<char*>(outputBuffer), bytesDecrypted);
                }

                memcpy(iv, inputBuffer + bytesRead - 16, 16);
            }

            inputFile.close();
            outputFile.close();
            BCryptDestroyKey(hKey);
            BCryptCloseAlgorithmProvider(hAlgorithm, 0);

            return true;
        }
        catch (const std::exception& e) {
            SetError(L"Decryption failed: " + Utf8ToWide(e.what()));
            return false;
        }
    }

    void AppxBundleImpl::SetError(const std::wstring& error) {
        m_lastError = error;
    }

    bool AppxBundleImpl::Bundle(const std::wstring& inputPath, const std::wstring& outputPath,
        CompressionLevel compression, ProgressCallback callback) {

        if (!fs::exists(inputPath) || !fs::is_directory(inputPath)) {
            SetError(L"Input path does not exist or is not a directory");
            return false;
        }

        std::vector<fs::path> packageFiles;
        try {
            for (const auto& entry : fs::directory_iterator(inputPath)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().wstring();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                    if (ext == L".appx" || ext == L".msix") {
                        packageFiles.push_back(entry.path());
                    }
                }
            }
        }
        catch (const std::exception& e) {
            SetError(L"Error scanning input directory");
            return false;
        }

        if (packageFiles.empty()) {
            SetError(L"No .appx or .msix files found in input directory");
            return false;
        }

        std::wstring bundleManifest = GenerateBundleManifest(packageFiles);
        if (bundleManifest.empty()) {
            return false;
        }

        fs::path outputDir = fs::path(outputPath).parent_path();
        if (!outputDir.empty() && !fs::exists(outputDir)) {
            fs::create_directories(outputDir);
        }

        int compressionMethod = (compression == CompressionLevel::None) ? ZIP_CM_STORE : ZIP_CM_DEFLATE;
        std::string outputPathUtf8(outputPath.begin(), outputPath.end());

        zip_t* zip = zip_open(outputPathUtf8.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr);
        if (!zip) {
            SetError(L"Failed to create bundle file");
            return false;
        }

        ProgressInfo progress = {};
        progress.totalFiles = packageFiles.size() + 1;
        progress.totalBytes = 0;

        for (const auto& file : packageFiles) {
            try {
                progress.totalBytes += fs::file_size(file);
            }
            catch (...) {
                continue;
            }
        }

        zip_source_t* manifestSource = zip_source_buffer(zip,
            bundleManifest.c_str(), bundleManifest.length() * sizeof(wchar_t), 0);
        if (!manifestSource || zip_file_add(zip, "AppxBundleManifest.xml", manifestSource, ZIP_FL_OVERWRITE) < 0) {
            zip_close(zip);
            SetError(L"Failed to add bundle manifest");
            return false;
        }

        uint64_t processedBytes = 0;
        for (size_t i = 0; i < packageFiles.size(); ++i) {
            const auto& packageFile = packageFiles[i];

            if (callback) {
                progress.processedFiles = i + 1;
                progress.processedBytes = processedBytes;
                progress.currentFile = packageFile.filename().wstring();
                callback(progress);
            }

            std::string localPathUtf8 = packageFile.string();
            std::string packageName = packageFile.filename().string();

            zip_source_t* source = zip_source_file(zip, localPathUtf8.c_str(), 0, -1);
            if (!source) {
                zip_close(zip);
                SetError(L"Failed to create source for: " + packageFile.wstring());
                return false;
            }

            zip_int64_t index = zip_file_add(zip, packageName.c_str(), source, ZIP_FL_OVERWRITE);
            if (index < 0) {
                zip_source_free(source);
                zip_close(zip);
                SetError(L"Failed to add package to bundle: " + packageFile.wstring());
                return false;
            }

            zip_set_file_compression(zip, index, compressionMethod, 0);

            try {
                processedBytes += fs::file_size(packageFile);
            }
            catch (...) {
            }
        }

        if (callback) {
            progress.processedFiles = packageFiles.size() + 1;
            progress.processedBytes = processedBytes;
            progress.currentFile = L"Finalizing bundle...";
            callback(progress);
        }

        int result = zip_close(zip);
        if (result != 0) {
            SetError(L"Failed to finalize bundle");
            return false;
        }

        return true;
    }

    bool AppxBundleImpl::Unbundle(const std::wstring& inputPath, const std::wstring& outputPath,
        OverwriteMode overwrite, ProgressCallback callback) {

        std::string inputPathUtf8(inputPath.begin(), inputPath.end());
        zip_t* zip = zip_open(inputPathUtf8.c_str(), ZIP_RDONLY, nullptr);

        if (!zip) {
            SetError(L"Failed to open bundle file");
            return false;
        }

        if (!fs::exists(outputPath)) {
            fs::create_directories(outputPath);
        }

        zip_int64_t numEntries = zip_get_num_entries(zip, 0);
        if (numEntries < 0) {
            zip_close(zip);
            SetError(L"Failed to get bundle contents");
            return false;
        }

        ProgressInfo progress = {};
        progress.totalFiles = static_cast<uint64_t>(numEntries);
        progress.totalBytes = 0;
        for (zip_int64_t i = 0; i < numEntries; ++i) {
            const char* name = zip_get_name(zip, i, 0);
            if (!name) continue;

            std::string fileName(name);
            std::wstring fileNameW(fileName.begin(), fileName.end());
            std::wstring fullPath = outputPath + L"\\" + fileNameW;

            if (callback) {
                progress.processedFiles = static_cast<uint64_t>(i);
                progress.currentFile = fileNameW;
                callback(progress);
            }

            if (fs::exists(fullPath)) {
                if (overwrite == OverwriteMode::No) continue;
                if (overwrite == OverwriteMode::Ask) {
                    if (!PromptUserOverwrite(fileNameW)) {
                        continue;
                    }
                }
            }

            zip_file_t* zipFile = zip_fopen_index(zip, i, 0);
            if (!zipFile) continue;

            std::ofstream outFile(fullPath, std::ios::binary);
            if (!outFile.is_open()) {
                zip_fclose(zipFile);
                continue;
            }

            char buffer[8192];
            zip_int64_t bytesRead;
            while ((bytesRead = zip_fread(zipFile, buffer, sizeof(buffer))) > 0) {
                outFile.write(buffer, bytesRead);
                progress.processedBytes += static_cast<uint64_t>(bytesRead);
            }

            outFile.close();
            zip_fclose(zipFile);
        }

        zip_close(zip);

        if (callback) {
            progress.processedFiles = static_cast<uint64_t>(numEntries);
            progress.currentFile = L"Complete";
            callback(progress);
        }

        return true;
    }
}