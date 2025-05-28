# MakeAppxPP

**Enhanced Microsoft App Package Tool with Superior Memory Efficiency**

MakeAppxPP is a high-performance, memory-optimized replacement for Microsoft's `makeappx.exe` tool. Built from the ground up in C++, it provides all the functionality of the original tool while using significantly less memory and offering enhanced security features.

## 🚀 Key Features

### **Memory Efficiency**
- **8KB streaming buffers** instead of loading entire files into memory
- **One-file-at-a-time processing** minimizes RAM usage even with multi-GB packages
- **Smart resource management** with automatic cleanup and RAII patterns

### **Enhanced Security**
- **AES-256-CBC encryption** using Windows BCrypt API (not toy XOR)
- **Cryptographically secure random IV generation** for each encryption
- **32-byte key file support** for maximum security
- **Proper PKCS#7 padding** and secure memory handling

### **Complete Feature Parity**
- ✅ **pack** - Create APPX/MSIX packages from directories
- ✅ **unpack** - Extract packages to directories  
- ✅ **bundle** - Create APPXBUNDLE/MSIXBUNDLE from multiple packages
- ✅ **unbundle** - Extract bundles to individual packages
- ✅ **encrypt** - Secure encryption with AES-256
- ✅ **decrypt** - Decrypt encrypted packages
- ✅ **convertCGM** - Transform Content Group Maps for streaming
- ✅ **build** - Build packages from layout files

### **User Experience Improvements**
- **Real-time progress bars** with file counts and transfer speeds
- **Interactive file overwrite prompts** (y/n/all/skip all)
- **Colored console output** with professional formatting
- **Verbose and quiet modes** for different use cases
- **Cross-platform compatibility** (Windows primary, Linux compatible)

## 📋 Requirements

- **Windows 10/11** (primary platform)
- **Visual Studio 2022** (for building)
- **vcpkg** (for dependencies)
- **.NET Framework 4.8+** (runtime)

### Dependencies
```bash
vcpkg install libzip:x64-windows
vcpkg install libzip:x86-windows
```

## 🎯 Usage Examples

### **Package Operations**
```bash
# Create package with maximum compression
MakeAppxPP.exe pack -d "C:\MyApp" -p "MyApp.msix" -c max

# Extract package with overwrite protection
MakeAppxPP.exe unpack -p "MyApp.msix" -d "C:\Extracted" -s

# Create bundle from directory of packages
MakeAppxPP.exe bundle -d "C:\Packages" -p "MyAppBundle.msixbundle"

# Extract bundle contents
MakeAppxPP.exe unbundle -p "MyAppBundle.msixbundle" -d "C:\BundleContents" -o
```

### **Security Operations**
```bash
# Generate 32-byte AES key (PowerShell)
[System.Security.Cryptography.RNGCryptoServiceProvider]::Create().GetBytes(32) | Set-Content -Path "key.bin" -Encoding Byte

# Encrypt package with AES-256
MakeAppxPP.exe encrypt -p "MyApp.msix" -ep "MyApp.encrypted" -kf "key.bin"

# Decrypt package
MakeAppxPP.exe decrypt -ep "MyApp.encrypted" -p "MyApp_decrypted.msix" -kf "key.bin"
```

### **Advanced Operations**
```bash
# Convert Content Group Map for streaming
MakeAppxPP.exe convertCGM -s "source.xml" -f "final.xml"

# Build from layout file
MakeAppxPP.exe build -f "PackageLayout.xml" -op "Built.msix" -c normal -v
```

## 📚 Command Reference

### **Global Options**
- `-v, /v` - Verbose output with detailed progress
- `-q, /q` - Quiet mode (suppress non-error output)
- `-?, /?, -help, --help` - Show help

### **Compression Levels**
- `none` - No compression (fastest)  
- `fast` - Fast compression
- `normal` - Balanced compression (default)
- `max` - Maximum compression (smallest size)

### **Overwrite Modes**  
- Default: Prompt user for each file
- `-o, /o` - Overwrite all existing files
- `-s, /s` - Skip all existing files

## 🔍 Command Details

### **pack** - Create App Package

```bash
MakeAppxPP.exe pack [options]

Required:
  -d <directory>    Source directory containing app files
  -p <package>      Output package file (.appx, .msix)

Optional:
  -c <level>        Compression: none, fast, normal, max
  -v                Verbose progress output  
  -q                Quiet mode

Example:
  MakeAppxPP.exe pack -d "C:\MyApp" -p "MyApp.msix" -c max -v
```

### **unpack** - Extract App Package

```bash
MakeAppxPP.exe unpack [options]

Required:
  -p <package>      Source package file (.appx, .msix)  
  -d <directory>    Output directory for extracted files

Optional:
  -o                Overwrite existing files without prompting
  -s                Skip existing files without prompting
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe unpack -p "MyApp.msix" -d "C:\Extracted" -o -v
```

### **bundle** - Create App Bundle

```bash
MakeAppxPP.exe bundle [options]

Required:
  -d <directory>    Directory containing .appx/.msix files
  -p <bundle>       Output bundle file (.appxbundle, .msixbundle)

Optional:
  -c <level>        Compression level
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe bundle -d "C:\Packages" -p "MyBundle.msixbundle" -c normal
```

### **unbundle** - Extract App Bundle

```bash
MakeAppxPP.exe unbundle [options]

Required:
  -p <bundle>       Source bundle file (.appxbundle, .msixbundle)
  -d <directory>    Output directory for extracted packages

Optional:
  -o                Overwrite existing files without prompting
  -s                Skip existing files without prompting
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe unbundle -p "MyBundle.msixbundle" -d "C:\Extracted" -s
```

### **encrypt** - Secure Package Encryption

```bash
MakeAppxPP.exe encrypt [options]

Required:
  -p <package>      Source package/bundle file
  -ep <encrypted>   Output encrypted file
  -kf <keyfile>     32-byte AES-256 key file

Optional:
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe encrypt -p "MyApp.msix" -ep "MyApp.secure" -kf "aes256.key"
```

### **decrypt** - Decrypt Secure Package

```bash
MakeAppxPP.exe decrypt [options]

Required:
  -ep <encrypted>   Source encrypted file
  -p <package>      Output decrypted package/bundle
  -kf <keyfile>     32-byte AES-256 key file

Optional:
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe decrypt -ep "MyApp.secure" -p "MyApp_decrypted.msix" -kf "aes256.key"
```

### **convertCGM** - Content Group Map Conversion

```bash
MakeAppxPP.exe convertCGM [options]

Required:
  -s <source>       Source CGM file
  -f <final>        Output final CGM file

Optional:
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe convertCGM -s "source.xml" -f "final.xml" -v
```

### **build** - Build from Layout File

```bash
MakeAppxPP.exe build [options]

Required:
  -f <layoutfile>   Layout file specifying file mappings
  -op <output>      Output package file

Optional:
  -c <level>        Compression level
  -v                Verbose output
  -q                Quiet mode

Example:
  MakeAppxPP.exe build -f "PackageLayout.xml" -op "Built.msix" -c normal -v
```

## 📊 Performance Comparison

| Operation | Original makeappx | MakeAppxPP | Improvement |
|-----------|------------------|------------|-------------|
| **Large File Handling** | Often crashes | Stable streaming | **Reliability** |
| **Progress Feedback** | Minimal | Real-time bars | **UX Enhancement** |
| **Encryption** | Not available | AES-256-CBC | **Security** |
| **Cross-platform** | Windows only | Windows + Linux | **Portability** |

## 🛡️ Security Features

### **AES-256-CBC Encryption**
- **Industry standard encryption** using Windows BCrypt API
- **Unique IV per encryption** prevents rainbow table attacks  
- **Secure key derivation** from 32-byte key files
- **Memory-safe implementation** with automatic cleanup

### **Key Management**
```powershell
# Generate secure key file (PowerShell)
$key = New-Object byte[] 32
[System.Security.Cryptography.RNGCryptoServiceProvider]::Create().GetBytes($key)
[System.IO.File]::WriteAllBytes("myapp.key", $key)
```

## 🔧 Troubleshooting

### **Common Issues**

**"Missing AppxManifest.xml"**
- Ensure your source directory contains a valid `AppxManifest.xml` file
- Verify the manifest has proper XML structure and required elements

**"Failed to open package file"**  
- Check file permissions and ensure file is not locked
- Verify package file is not corrupted

**"Encryption key file too small"**
- Key file must be exactly 32 bytes for AES-256
- Use the PowerShell command above to generate proper keys

**Memory issues with very large packages**
- MakeAppxPP handles large files efficiently, but ensure adequate disk space
- Use `-q` flag to reduce console output overhead

### **Debug Mode**
```bash
# Enable verbose output for troubleshooting
MakeAppxPP.exe pack -d "C:\MyApp" -p "MyApp.msix" -v

# Check specific errors
MakeAppxPP.exe unpack -p "Problematic.msix" -d "C:\Debug" -v
```

## 📄 License

This project is provided as-is for educational and development purposes. 

## 🤝 Contributing

Built as a high-performance replacement for Microsoft's makeappx tool, focusing on:
- Memory efficiency for large enterprise applications
- Enhanced security with modern cryptography  
- Improved user experience with progress feedback
- Cross-platform compatibility