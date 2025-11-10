# Secure Virtual File System (SVFS) - Project Report

## Project Overview

**Project Name:** Secure Virtual File System (SVFS)  
**Version:** 2.0  
**Development Platform:** Windows 10/11  
**Programming Language:** C++17  
**GUI Framework:** Qt 6.9.2  
**Database:** SQLite 3  
**Build System:** CMake 3.30.5  
**Compiler:** MinGW GCC 13.1.0  

---

## Abstract

The Secure Virtual File System (SVFS) is a desktop application that provides a secure, encrypted storage solution for sensitive files. The system implements a virtual file system stored in a single SQLite database file, offering strong encryption, compression, and user authentication features. Files are stored as BLOBs (Binary Large Objects) in the database with support for multiple encryption algorithms and compression methods.

---

## 1. System Architecture

### 1.1 Architecture Pattern
The project follows a **layered architecture** with clear separation of concerns:

- **Presentation Layer:** Qt Widgets-based GUI (LoginDialog, MainWindow)
- **Business Logic Layer:** VFS Manager, Encryption Manager, Compression Manager
- **Data Access Layer:** Database Manager (SQLite interface)
- **Storage Layer:** SQLite database file on disk

### 1.2 Core Components

#### 1.2.1 VFSManager (Virtual File System Manager)
- **Purpose:** Central coordinator for all file operations
- **Design Pattern:** Singleton pattern
- **Key Responsibilities:**
  - File creation, deletion, and modification
  - Directory management
  - Content encryption/decryption pipeline
  - File import/export operations
  - User authentication management
  - Signal emission for UI updates

#### 1.2.2 DatabaseManager
- **Purpose:** SQLite database abstraction layer
- **Design Pattern:** Singleton pattern
- **Key Responsibilities:**
  - Database initialization and connection management
  - CRUD operations for users, files, and directories
  - SQL query execution and error handling
  - Transaction management

#### 1.2.3 EncryptionManager
- **Purpose:** Cryptographic operations handler
- **Design Pattern:** Singleton pattern
- **Key Responsibilities:**
  - Symmetric encryption/decryption
  - Key derivation from passwords
  - Checksum calculation
  - Support for multiple encryption algorithms

#### 1.2.4 CompressionManager
- **Purpose:** Data compression handler
- **Design Pattern:** Singleton pattern
- **Key Responsibilities:**
  - Data compression/decompression
  - Support for multiple compression algorithms
  - Compression level management

---

## 2. Database Schema

### 2.1 Users Table
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    created_at TEXT NOT NULL,
    last_login TEXT
);
```

**Purpose:** Store user credentials with salted password hashes for secure authentication.

### 2.2 Files Table
```sql
CREATE TABLE files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    filename TEXT NOT NULL,
    path TEXT NOT NULL,
    size INTEGER NOT NULL,
    mime_type TEXT,
    content BLOB,
    encrypted_content BLOB,
    is_encrypted INTEGER DEFAULT 0,
    is_compressed INTEGER DEFAULT 0,
    checksum TEXT,
    user_id INTEGER NOT NULL,
    created_at TEXT NOT NULL,
    modified_at TEXT NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

**Key Features:**
- Dual storage columns: `content` for plain/compressed data, `encrypted_content` for encrypted data
- Boolean flags for encryption and compression status
- SHA-256 checksum for integrity verification
- MIME type detection for file categorization

### 2.3 Directories Table
```sql
CREATE TABLE directories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    path TEXT NOT NULL,
    parent_id INTEGER,
    user_id INTEGER NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (parent_id) REFERENCES directories(id)
);
```

**Purpose:** Hierarchical directory structure with parent-child relationships.

---

## 3. Security Features

### 3.1 Encryption Algorithms

#### 3.1.1 AES-256-GCM (Default)
- **Algorithm:** Advanced Encryption Standard
- **Key Size:** 256 bits
- **Mode:** Galois/Counter Mode (authenticated encryption)
- **IV Size:** 12 bytes (randomly generated per encryption)
- **Authentication Tag:** 16 bytes (AEAD mode)
- **Library:** OpenSSL 3.6.0 EVP API
- **Advantages:**
  - Authenticated encryption (integrity + confidentiality)
  - Parallel processing capable
  - Industry standard for modern encryption

#### 3.1.2 AES-256-CBC
- **Algorithm:** Advanced Encryption Standard
- **Key Size:** 256 bits
- **Mode:** Cipher Block Chaining
- **IV Size:** 16 bytes (randomly generated)
- **Library:** OpenSSL 3.6.0 EVP API
- **Use Case:** Alternative mode for compatibility

#### 3.1.3 ChaCha20-Poly1305
- **Algorithm:** ChaCha20 stream cipher with Poly1305 MAC
- **Key Size:** 256 bits
- **Nonce Size:** 12 bytes
- **Library:** OpenSSL 3.6.0 EVP API
- **Advantages:**
  - Superior performance on devices without AES hardware acceleration
  - Constant-time implementation (resistant to timing attacks)

### 3.2 Key Derivation

**Algorithm:** PBKDF2-HMAC-SHA256
- **Iterations:** 100,000
- **Salt Size:** 32 bytes (randomly generated per user)
- **Output Key Size:** 32 bytes (256 bits)
- **Purpose:** Derive encryption keys from user passwords with protection against brute-force attacks

### 3.3 Password Security

- **Hashing Algorithm:** PBKDF2-HMAC-SHA256
- **Salt:** Unique 32-byte random salt per user
- **Iterations:** 100,000 (computationally expensive to slow down brute-force)
- **Storage:** Only salted hash stored in database (plaintext passwords never stored)

### 3.4 Custom Encrypted File Format

**Binary Structure:**
```
[MAGIC: 4 bytes] "SVFS"
[VERSION: 1 byte] Format version
[ALGORITHM: 1 byte] Encryption algorithm ID
[FLAGS: 1 byte] Bit flags (compressed, algorithm details)
[IV_LENGTH: 1 byte] Initialization vector length
[IV: variable] Initialization vector
[TAG_LENGTH: 1 byte] Authentication tag length (for AEAD modes)
[TAG: variable] Authentication tag
[PAYLOAD: variable] Encrypted data
```

**Flags Byte:**
- Bit 0: Compressed (1 = compressed before encryption)
- Bits 2-3: Compression algorithm (00=ZLIB, 01=LZ4, 10=ZSTD)
- Bits 4-7: Reserved for future use

---

## 4. Compression Features

### 4.1 Implemented Algorithms

#### 4.1.1 ZLIB (Default)
- **Library:** Qt built-in qCompress/qUncompress
- **Algorithm:** DEFLATE (LZ77 + Huffman coding)
- **Compression Levels:** 1-9 (default: 6)
- **Use Case:** General-purpose compression with good balance
- **Status:** Fully implemented and tested

### 4.2 Conditional Compilation Support

#### 4.2.2 LZ4 (Optional)
- **Library:** lz4 (external dependency)
- **Compilation Flag:** HAVE_LZ4
- **Features:**
  - Extremely fast compression/decompression
  - Lower compression ratio than ZLIB
  - Ideal for real-time applications
- **Status:** Code implemented, requires library installation

#### 4.2.3 ZSTD (Optional)
- **Library:** zstd (external dependency)
- **Compilation Flag:** HAVE_ZSTD
- **Features:**
  - Better compression ratio than ZLIB
  - Tunable compression levels (1-22)
  - Fast decompression
- **Status:** Code implemented, requires library installation

### 4.3 Compression Pipeline

**Process Flow:**
1. Plain data → Compression (if enabled)
2. Compressed data → Encryption (if enabled)
3. Encrypted data → Database storage

**Decompression Flow:**
1. Database → Encrypted data
2. Decryption → Compressed data
3. Decompression → Plain data

---

## 5. User Interface

### 5.1 Login Dialog
- **Features:**
  - Dark theme with modern styling
  - Username and password fields
  - "Remember me" option (stores credentials in QSettings)
  - "Show password" toggle
  - Create new account functionality
  - Forgot password link (placeholder)

### 5.2 Main Window

#### 5.2.1 Menu Bar
- **File Menu:**
  - New VFS / Open VFS
  - New File / New Folder
  - Import / Export
  - Open / Delete / Rename
  - Refresh
  - Exit

- **Edit Menu:**
  - Copy / Cut / Paste
  - Properties
  - Settings

- **Tools Menu:**
  - Scan Drive (read-only system file browser)
  - Encrypt File / Decrypt File
  - Compress File / Decompress File

- **View Menu:**
  - Toggle Preview Panel
  - Theme Selection (System/Light/Dark/High Contrast)

- **Help Menu:**
  - Show Encryption Proof (debug tool)
  - About

#### 5.2.2 Toolbar
- Quick access buttons: New File, New Folder, Import, Export, Open, Delete, Refresh
- Encryption algorithm selector: AES-256-GCM, AES-256-CBC, ChaCha20-Poly1305
- Compression algorithm selector: ZLIB, LZ4, ZSTD
- Theme selector
- Toggle Preview button (right-aligned)

#### 5.2.3 File Tree View
- **Columns:** Name, Size, Date, Type
- **Features:**
  - Hierarchical directory structure
  - Color-coded encrypted files (red background)
  - Color-coded compressed files (blue text)
  - Context menu (right-click) with operations
  - Double-click to open file
  - Drag-and-drop support (future)

#### 5.2.4 Preview Panel (Tabbed)
- **Text Preview Tab:**
  - Displays decrypted text content
  - Banner showing encryption/compression status
  - Syntax highlighting for code files (future)

- **Image Preview Tab:**
  - Display images (PNG, JPG, etc.)
  - Placeholder for current version

- **Properties Tab:**
  - File name, path, size
  - MIME type
  - Creation and modification timestamps
  - Encryption status (Yes/No)
  - Compression status (Yes/No)

### 5.3 Context Menu Operations
- Open
- Edit (built-in text editor)
- Encrypt / Decrypt
- Compress / Decompress
- Rename
- Delete
- Copy / Cut / Paste
- Export
- Properties

---

## 6. Core Functionality

### 6.1 File Operations

#### 6.1.1 Import File
- Select local file via file dialog
- Optionally encrypt and/or compress during import
- Calculate checksum for integrity verification
- Store in database with metadata
- Add to file tree view

#### 6.1.2 Export File
- **Decrypted/Readable Export:** Decrypts and decompresses file before saving
- **Raw Export:** Saves encrypted/compressed binary data as-is
- User chooses export mode via dialog for encrypted files

#### 6.1.3 Create New File
- Built-in text editor with syntax highlighting support
- Optionally encrypt and/or compress on save
- Auto-save functionality

#### 6.1.4 Edit File
- Opens file in built-in editor
- Loads decrypted content
- Saves changes with encryption/compression preserved

#### 6.1.5 Delete File
- Confirmation dialog to prevent accidental deletion
- Removes file record and content from database
- Updates file tree

### 6.2 Encryption Operations

#### 6.2.1 Encrypt Existing File
1. User selects unencrypted file
2. Right-click → Encrypt (or Tools menu)
3. System retrieves plaintext content
4. Applies selected encryption algorithm
5. Updates database with encrypted content
6. Updates file tree to show "Encrypted" status

#### 6.2.2 Decrypt Existing File
1. User selects encrypted file
2. Right-click → Decrypt
3. System decrypts content using stored key
4. Updates database with plaintext content
5. Updates file tree to show "Normal" status

### 6.3 Compression Operations

#### 6.3.1 Compress Existing File
1. User selects uncompressed file
2. Right-click → Compress
3. System applies selected compression algorithm
4. Updates database with compressed content
5. Updates file tree to show "Compressed" status

#### 6.3.2 Decompress Existing File
1. User selects compressed file
2. Right-click → Decompress
3. System decompresses content
4. Updates database with uncompressed content
5. Updates file tree to show "Normal" status

### 6.4 Directory Operations
- Create nested directories
- Navigate directory hierarchy
- Delete empty directories
- Move files between directories (future)

---

## 7. Advanced Features

### 7.1 System Scan Mode
- **Purpose:** Browse local file system in read-only mode
- **Features:**
  - Scan entire drive or specific folder
  - Real-time progress display
  - Cancel scan operation
  - Import selected files to VFS with encryption/compression

### 7.2 Batch Operations
- Select multiple files
- Apply encryption/compression to all
- Batch import/export (partial implementation)

### 7.3 Search Functionality
- Search files by name
- Filter by file type (future)
- Search by content (future)

### 7.4 Theme System
- **System Theme:** Follows OS settings
- **Light Theme:** Light background, dark text
- **Dark Theme:** Dark background, light text
- **High Contrast Theme:** Maximum readability

### 7.5 Encryption Proof Tool
- **Purpose:** Demonstrate that encryption is working
- **Features:**
  - Shows raw database storage (hex dump of encrypted bytes)
  - Shows decrypted content side-by-side
  - Proves data is secure at rest
  - Educational tool for presentations

---

## 8. Technical Implementation Details

### 8.1 Encryption Pipeline

**Code Flow (VFSManager::processContent):**
```cpp
QByteArray processContent(const QByteArray &content, bool encrypt, bool compress) {
    QByteArray processed = content;
    
    // Step 1: Compress if requested
    if (compress) {
        processed = CompressionManager::compress(processed, algorithm, level);
    }
    
    // Step 2: Encrypt if requested (with compression flag embedded)
    if (encrypt) {
        unsigned char flags = compress ? 0x01 : 0x00;
        // Encode compression algorithm in flags
        processed = EncryptionManager::encryptWithFlags(processed, algorithm, flags);
    }
    
    return processed;
}
```

**Decryption Pipeline:**
```cpp
QByteArray unprocessContent(const QByteArray &processed, bool decrypt, bool decompress) {
    QByteArray plain = processed;
    
    // Step 1: Decrypt if encrypted (extracts compression flag)
    if (decrypt) {
        unsigned char flags = 0;
        plain = EncryptionManager::decryptAndGetFlags(plain, algorithm, flags);
        // Check if data was also compressed
        bool wasCompressed = (flags & 0x01) != 0;
    }
    
    // Step 2: Decompress if compressed
    if (decompress || wasCompressed) {
        plain = CompressionManager::decompress(plain);
    }
    
    return plain;
}
```

### 8.2 File Reprocessing

**Dynamic Encryption/Compression Toggle:**
```cpp
bool reprocessFile(int fileId, bool encrypt, bool compress) {
    // 1. Get current plaintext (auto-decrypt/decompress)
    QByteArray plaintext = getFileContent(fileId);
    
    // 2. Apply new transformations
    QByteArray processed = processContent(plaintext, encrypt, compress);
    
    // 3. Update database with new state
    updateFileContent(fileId, processed, encrypt, compress);
    
    return true;
}
```

### 8.3 Checksum Verification

**Integrity Check:**
- SHA-256 hash calculated on plaintext
- Stored in database with file metadata
- Verified on file retrieval
- Detects corruption or tampering

---

## 9. Build Instructions

### 9.1 Prerequisites
- **Qt 6.9.2** (mingw_64 or MSVC build)
- **CMake 3.20+**
- **MinGW GCC 13.1.0** or **MSVC 2019/2022**
- **OpenSSL 3.6.0** (MSYS2 mingw64 package)

### 9.2 Build Steps (Windows MinGW)

```bash
# 1. Configure CMake
cmake -B build_clean -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=C:/Qt/6.9.2/mingw_64 ^
    -DOPENSSL_ROOT_DIR=C:/msys64/mingw64

# 2. Build
cmake --build build_clean -- -j 8

# 3. Deploy Qt dependencies
set QTFRAMEWORK_BYPASS_LICENSE_CHECK=1
C:\Qt\6.9.2\mingw_64\bin\windeployqt.exe ^
    --no-translations --release build_clean\SecureVFS.exe

# 4. Run
cd build_clean
.\SecureVFS.exe
```

### 9.3 CMake Configuration

**Key CMake Settings:**
```cmake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenSSL REQUIRED)

target_link_libraries(SecureVFS
    Qt6::Widgets
    Qt6::Sql
    OpenSSL::SSL
    OpenSSL::Crypto
)
```

---

## 10. Testing & Validation

### 10.1 Manual Testing Performed

#### 10.1.1 Encryption Testing
- **Test 1:** Create text file, encrypt, verify encryption proof tool shows encrypted hex
- **Test 2:** Export encrypted file as raw, verify unreadable in text editor
- **Test 3:** Decrypt file, verify readable content restored
- **Test 4:** Test all three encryption algorithms (AES-GCM, AES-CBC, ChaCha20)

#### 10.1.2 Compression Testing
- **Test 1:** Compress text file, verify size reduction in properties
- **Test 2:** Decompress file, verify content integrity
- **Test 3:** Compress then encrypt, verify both transformations applied

#### 10.1.3 Authentication Testing
- **Test 1:** Create user, logout, login again
- **Test 2:** Test incorrect password rejection
- **Test 3:** Test "Remember me" functionality
- **Test 4:** Test multiple user isolation

#### 10.1.4 File Operations Testing
- **Test 1:** Import various file types (text, binary, images)
- **Test 2:** Edit files and save changes
- **Test 3:** Delete files and verify removal
- **Test 4:** Copy/paste files within VFS

### 10.2 Known Issues
- None critical identified
- Large file handling (>100MB) not extensively tested
- Unicode filename support varies by platform

---

## 11. Security Considerations

### 11.1 Implemented Security Features
- **Encryption at Rest:** All sensitive files encrypted in database
- **Salted Password Hashing:** PBKDF2 with unique salts per user
- **Key Derivation:** 100,000 iterations to resist brute-force
- **Authenticated Encryption:** GCM mode provides integrity verification
- **Checksum Validation:** Detect file corruption

### 11.2 Security Limitations (Disclaimer)
- **Master Password:** If user password is compromised, all data accessible
- **Key Storage:** Encryption key derived from password (key = password strength)
- **No Key Rotation:** Re-encryption required if password changed
- **Memory Security:** Keys temporarily in memory during operation
- **No Hardware Security Module (HSM):** Software-only encryption

### 11.3 Production Recommendations (Future)
- Implement two-factor authentication (2FA)
- Add key rotation mechanism
- Use hardware-backed key storage (TPM on Windows)
- Implement secure key deletion (memory zeroing)
- Add file access audit logs
- Implement automatic session timeout
- Add file version history with encryption

---

## 12. Future Enhancements

### 12.1 Planned Features
- **Cloud Sync:** Sync encrypted VFS to cloud storage
- **File Versioning:** Keep history of file changes
- **File Sharing:** Share encrypted files with other users
- **Mobile App:** Android/iOS clients
- **Command-Line Interface:** Scriptable operations
- **Backup/Restore:** Automated VFS backups
- **Quota Management:** Storage limits per user

### 12.2 Performance Optimizations
- **Lazy Loading:** Load file content on-demand
- **Caching:** Cache frequently accessed files
- **Parallel Processing:** Multi-threaded encryption
- **Database Indexing:** Optimize queries with indexes

### 12.3 UI Improvements
- **Drag-and-Drop:** Drag files into VFS
- **Thumbnails:** Image previews in file list
- **Syntax Highlighting:** Code editor with highlighting
- **Split View:** View multiple files simultaneously

---

## 13. Libraries and Dependencies

### 13.1 Core Dependencies
| Library | Version | Purpose |
|---------|---------|---------|
| Qt Widgets | 6.9.2 | GUI framework |
| Qt SQL | 6.9.2 | SQLite database interface |
| Qt Core | 6.9.2 | Core utilities (file I/O, strings, etc.) |
| OpenSSL | 3.6.0 | Cryptographic operations |
| SQLite | 3.x | Database engine |

### 13.2 Build Tools
| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.30.5 | Build system generator |
| MinGW GCC | 13.1.0 | C++ compiler |
| windeployqt | 6.9.2 | Qt deployment utility |

### 13.3 Optional Dependencies
| Library | Status | Purpose |
|---------|--------|---------|
| LZ4 | Not installed | Fast compression (optional) |
| ZSTD | Not installed | High-ratio compression (optional) |

---

## 14. Project Statistics

### 14.1 Code Metrics
- **Total Lines of Code:** ~5,000+
- **Header Files:** 8
- **Source Files:** 8
- **Languages:** C++ (primary), CMake
- **Classes:** 7 major classes

### 14.2 File Structure
```
SVFS/
├── CMakeLists.txt
├── main.cpp
├── include/
│   ├── core/
│   │   ├── VFSManager.h
│   │   ├── DatabaseManager.h
│   │   ├── EncryptionManager.h
│   │   └── CompressionManager.h
│   ├── ui/
│   │   ├── MainWindow.h
│   │   └── LoginDialog.h
│   └── scan/
│       └── FileSystemScanner.h
├── src/
│   ├── core/
│   │   ├── VFSManager.cpp
│   │   ├── DatabaseManager.cpp
│   │   ├── EncryptionManager.cpp
│   │   └── CompressionManager.cpp
│   ├── ui/
│   │   ├── MainWindow.cpp
│   │   └── LoginDialog.cpp
│   └── scan/
│       └── FileSystemScanner.cpp
├── build_clean/          (Build artifacts)
└── svfs.db              (Database file)
```

---

## 15. Conclusion

The Secure Virtual File System (SVFS) successfully demonstrates a complete implementation of a secure file storage solution with the following achievements:

### 15.1 Accomplishments
- **Functional VFS:** Complete virtual file system with directory hierarchy
- **Strong Encryption:** Industry-standard AES-256 and ChaCha20 encryption
- **Flexible Compression:** ZLIB compression with optional LZ4/ZSTD support
- **User Authentication:** Secure password hashing and multi-user support
- **Modern UI:** Intuitive Qt-based interface with dark theme
- **Database Integration:** Efficient SQLite storage with BLOB support
- **Cross-Platform Potential:** Qt framework enables Windows/Linux/macOS support

### 15.2 Learning Outcomes
- **Applied Cryptography:** Practical implementation of encryption algorithms
- **Database Design:** Schema design for file storage systems
- **Qt Framework Mastery:** GUI development with signals/slots, custom widgets
- **C++ Best Practices:** RAII, smart pointers, singleton pattern
- **Security Engineering:** Understanding of key derivation, salting, AEAD modes
- **Software Architecture:** Layered design with clear separation of concerns

### 15.3 Real-World Applications
This system demonstrates concepts used in:
- Password managers (1Password, LastPass)
- Cloud storage services (Dropbox, Google Drive)
- Document management systems
- Encrypted backup solutions
- Secure collaboration platforms

### 15.4 Educational Value
The project serves as a complete example of:
- Encryption implementation in production code
- Database-backed application development
- GUI application architecture
- Build system configuration
- Cross-platform development practices

---

## 16. References

### 16.1 Technical Documentation
- **Qt Documentation:** https://doc.qt.io/qt-6/
- **OpenSSL Documentation:** https://www.openssl.org/docs/
- **SQLite Documentation:** https://www.sqlite.org/docs.html
- **CMake Documentation:** https://cmake.org/documentation/

### 16.2 Cryptography Standards
- **NIST FIPS 197:** Advanced Encryption Standard (AES)
- **RFC 7539:** ChaCha20 and Poly1305 for IETF Protocols
- **RFC 8018:** PKCS #5: Password-Based Cryptography Specification Version 2.1
- **NIST SP 800-38D:** Recommendation for Block Cipher Modes of Operation: Galois/Counter Mode (GCM)

### 16.3 Security Best Practices
- **OWASP Cryptographic Storage Cheat Sheet**
- **NIST Cryptographic Standards and Guidelines**

---

## Appendix A: Installation Guide

### A.1 System Requirements
- **OS:** Windows 10/11 (64-bit)
- **RAM:** 4 GB minimum, 8 GB recommended
- **Disk Space:** 500 MB for application + variable for VFS database
- **Display:** 1280x720 minimum resolution

### A.2 Qt Installation
1. Download Qt Online Installer from https://www.qt.io/download-qt-installer
2. Install Qt 6.9.2 with MinGW 64-bit component
3. Add Qt bin directory to PATH: `C:\Qt\6.9.2\mingw_64\bin`

### A.3 OpenSSL Installation (MSYS2)
```bash
# Install MSYS2 from https://www.msys2.org/
# Open MSYS2 MinGW 64-bit terminal
pacman -S mingw-w64-x86_64-openssl
```

### A.4 CMake Installation
- Download from https://cmake.org/download/
- Install and add to PATH during installation

---

## Appendix B: Usage Examples

### B.1 Creating Your First VFS
1. Launch SecureVFS.exe
2. Click "Create Account" and enter credentials
3. File → New VFS (creates svfs.db)
4. You're now in the VFS!

### B.2 Importing and Encrypting a File
1. File → Import or click "Import" button
2. Select a file from your computer
3. Check "Encrypt" option in import dialog
4. Click "Import"
5. File is now stored encrypted in VFS

### B.3 Verifying Encryption
1. Right-click encrypted file
2. Select "Export"
3. Choose "Raw (Keep Encrypted)"
4. Save to desktop
5. Open exported file in Notepad → See encrypted gibberish!

### B.4 Compression Example
1. Select a text file in VFS
2. Right-click → Compress
3. View Properties tab → Size reduced
4. Open file → Content automatically decompressed

---

## Appendix C: Troubleshooting

### C.1 Application Won't Start
- **Issue:** Missing Qt DLLs
- **Solution:** Run `windeployqt SecureVFS.exe` in build directory

### C.2 Database Errors
- **Issue:** Database file locked
- **Solution:** Close any SQLite viewers, ensure single instance running

### C.3 Encryption Errors
- **Issue:** OpenSSL DLLs not found
- **Solution:** Add `C:\msys64\mingw64\bin` to system PATH

---

**Document Version:** 1.0  
**Last Updated:** November 10, 2025  
**Author:** Project Team  
**Course:** [Your Course Name]  
**Institution:** [Your Institution]
