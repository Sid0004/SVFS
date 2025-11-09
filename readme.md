#
<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/57e57737-ae77-49a8-a556-d28d90f240f2" />


# SVFS - Secure Virtual File System                             
A robust, secure file management system with built-in encryption, compression, and user authentication. Files are stored entirely within a secure SQLite database, providing a true virtual file system experience.

<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/542d35e8-f442-439e-ab7c-2ec1c444a027" />


## 🚀 Quick Start

Use MinGW Qt toolchain (matching provided binaries) + OpenSSL.

```cmd
:: Configure (MinGW + Qt + OpenSSL)
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.9.2\mingw_64" -DOPENSSL_ROOT_DIR=C:/msys64/mingw64

:: Build
cmake --build build -- -j %NUMBER_OF_PROCESSORS%

:: (Optional) Bypass Qt license check spam for open-source build
set QTFRAMEWORK_BYPASS_LICENSE_CHECK=1

:: Deploy Qt runtime (DLLs, plugins) next to exe
cd build
"C:\Qt\6.9.2\mingw_64\bin\windeployqt.exe" --no-translations --release SecureVFS.exe

:: Run
SecureVFS.exe
```

If the app exits immediately, create a user first (New VFS) or add logging. A default test user is NOT auto-created.

## 🔑 Key Features

### Virtual File System
- ✅ All files stored IN the VFS database (not external links)
- ✅ Built-in text editor - create files directly in VFS
- ✅ Import files FROM your computer INTO VFS
- ✅ Export files FROM VFS to your computer
- ✅ No external file dependencies

### Security & Encryption
- 🔒 **Modern ciphers** – AES‑256‑GCM (default), AES‑256‑CBC, ChaCha20‑Poly1305
- 🔁 **Algorithm selector** – switch encryption and compression from the toolbar or Settings
- 🔓 **Decrypt on demand** – remove encryption when needed
- 📦 **Compression** – ZLIB built-in; optional LZ4/Zstd when available
- 📂 **Decompress instantly** – auto-detected during decrypt via header flags
- 🔐 **User Authentication** - Secure login with salted passwords
- 👤 **Multi-User Support** - Each user has isolated vault

### File Operations
- 📝 Create/Edit text files with built-in editor
- 📥 Import files from local disk
- 📤 Export files to local disk
- 🗂️ Create folders and organize files
- 🔍 Search across all files
- ❌ Delete, rename, copy files
- ℹ️ View detailed file properties

### Advanced Features
- **Context Menu**: Right-click for all operations
- **Dual-Pane View**: File tree + preview/editor (toggle preview panel)
- **Real-time Stats**: See file count, sizes, encryption status
- **Multiple VFS**: Create and switch between different vaults
- **Settings & Toolbar**: Customize and switch encryption/compression algorithms
- **Themes**: System/Light/Dark/High Contrast with persistence
- **File Properties**: Detailed info incl. detected encryption algorithm and compression flag

## 📖 How It Works

### The VFS Concept
Unlike traditional file systems, SVFS stores **ALL file content inside an SQLite database**:
- No external file references
- Files exist only in the `.svfsdb` file
- Complete portability - move one file, move entire vault
- Encryption/compression managed transparently

### Typical Workflow

1. **Create VFS**: File → New VFS → Save as `my_vault.svfsdb`
2. **Create Account**: Set username/password
3. **Add Files**:
   - **Import**: Bring existing files INTO the VFS
   - **Create New**: Write text files directly in built-in editor
4. **Secure Files**:
   - Right-click → 🔒 Encrypt (AES-256)
   - Right-click → 📦 Compress (save space)
5. **Edit Anytime**: Right-click → Edit (even encrypted files)
6. **Export When Needed**: Right-click → Export (extract to disk)

## 🛠️ Building

### Windows Qt MinGW (Recommended)
```cmd
# Open Qt 6 MinGW Command Prompt
cd /d C:\Users\siddh\OneDrive\Documents\Desktop\SVFS
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.9.2\mingw_64" -DOPENSSL_ROOT_DIR=C:/msys64/mingw64
cmake --build build -- -j %NUMBER_OF_PROCESSORS%
set QTFRAMEWORK_BYPASS_LICENSE_CHECK=1
cd build
"C:\Qt\6.9.2\mingw_64\bin\windeployqt.exe" --no-translations --release SecureVFS.exe
SecureVFS.exe
```
Optional: LZ4/Zstd support (detected automatically if installed)
```cmd
:: In MSYS2 shell (for MinGW toolchain), you can install:
pacman -S --needed mingw-w64-x86_64-lz4 mingw-w64-x86_64-zstd
```

### Windows Visual Studio
```cmd
# Open Developer Command Prompt
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.9.2\msvc2019_64" -DOPENSSL_ROOT_DIR=C:/OpenSSL
cmake --build build --config Release -- /m
cd build/Release
"C:\Qt\6.9.2\msvc2019_64\bin\windeployqt.exe" --no-translations --release SecureVFS.exe
SecureVFS.exe
```

### Linux/macOS
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./SecureVFS
```

## 📚 Usage Examples

### Creating a Secure Document Vault

```
1. Launch SVFS
2. File → New VFS → "documents.svfsdb"
3. Create account: "admin" / "secure_password"
4. Right-click tree → New File
5. Enter content in editor
6. Check "Encrypt" before saving
7. File saved encrypted in VFS!
```

### Importing and Securing Existing Files

```
1. File → Import File
2. Select "confidential.pdf"
3. Check "Encrypt" + "Compress"
4. File now secure inside VFS
5. Original can be deleted from disk
```

### Working with Encrypted Files

```
1. Double-click encrypted file → View in preview
2. Right-click → Edit → Makes changes
3. Save → Automatically re-encrypted
4. Right-click → 🔓 Decrypt → Remove encryption
5. Right-click → 🔒 Encrypt → Re-encrypt anytime
```

### Exporting for Sharing

```
1. Right-click file → Export
2. Choose destination
3. File extracted from VFS to disk
4. Share/use normally
5. Original stays secure in VFS
```

## 🏗️ Architecture

### Core Components

- **DatabaseManager**: SQLite operations, schema, queries
- **VFSManager**: High-level file ops, user sessions, default algorithm prefs
- **EncryptionManager**: OpenSSL EVP (AES‑GCM/CBC, ChaCha20‑Poly1305), PBKDF2‑HMAC‑SHA256
- **CompressionManager**: ZLIB built‑in; optional LZ4/Zstd
- **MainWindow**: Qt6 GUI, themes, selectors, file tree, editor, menus
- **LoginDialog**: Authentication, account creation

### Database Schema

```sql
users: id, username, password_hash, salt, created_at, last_login
files: id, filename, path, content, encrypted_content, mime_type, 
       size, user_id, is_encrypted, is_compressed, checksum
directories: id, name, path, parent_id, user_id, created_at
```

## 🔒 Security Notes

### ✅ What's Implemented
- AES‑256‑GCM (default), AES‑256‑CBC, ChaCha20‑Poly1305
- PBKDF2‑HMAC‑SHA256 key derivation (100K iterations)
- Per-user file isolation
- Header format with magic+version+algorithm+flags (compression embedded)
- SHA‑256 checksums for content verification

### ⚠️ Production Recommendations
For real-world use, enhance with:
- Argon2 for password hashing
- Hardware-backed key storage (OS keychain/TPM)
- Role-based access controls and audit logging
- Backup/recovery and key rotation
- 2FA authentication

## 🎯 Use Cases

- **Personal Document Vault**: Secure storage for sensitive documents
- **Password Manager**: Store encrypted text files with passwords
- **Secure Notes**: Keep private notes in encrypted VFS
- **Portable Security**: Carry vault on USB drive
- **Development**: Store API keys, certificates securely
- **Archive**: Compress old files, save space
- **Multi-User**: Family members with separate vaults

## 🤝 Contributing

Areas for improvement:
- WebDAV/cloud sync
- File versioning/history
- Drag-and-drop UI
- Thumbnail previews
- Batch operations
- Regex search
- Export as ZIP archive
- Mobile app version

## 📝 Requirements

- **Qt 6.x** (Core, Widgets, Sql)
- **CMake 3.16+**
- **C++17 compiler** (MSVC/GCC/Clang)
- **SQLite** (included with Qt)

## 📞 Support & Troubleshooting

**Q: I forgot my password!**  
A: Unfortunately, passwords cannot be recovered. Keep backups!

**Q: App builds but no window appears?**  
A: You likely closed the login dialog (Reject) or have no user. Create a new VFS, then a user. For diagnostics, add `qDebug()` in `main.cpp` before and after `login.exec()`. Ensure `windeployqt` ran so the platform plugin `qwindows.dll` is present in `build\platforms`.

**Q: License warnings during build?**  
A: Set `QTFRAMEWORK_BYPASS_LICENSE_CHECK=1` before building to suppress non-commercial licensing noise.

**Q: Can I access the same VFS from multiple computers?**  
A: Yes! Just copy the `.svfsdb` file. Log in with same credentials.

**Q: Are files really secure?**  
A: For personal use, yes. For critical data, enhance with production-grade crypto.

**Q: Can I have multiple users?**  
A: Yes! Each user sees only their files. Create accounts in login dialog.

**Q: How do I backup?**  
A: Simply copy the `.svfsdb` file. That's your entire vault!

## 🙏 Credits

- **Author**: Siddharth
- **Created**: August 31, 2025
- **Framework**: Qt6
- **Database**: SQLite
- **Version**: 2.0

---

**⚠️ Remember**: This is a demonstration/educational project. For production use with sensitive data, implement additional security measures and conduct a security audit.

**💡 Tip**: Keep regular backups of your `.svfsdb` files and store passwords securely!
