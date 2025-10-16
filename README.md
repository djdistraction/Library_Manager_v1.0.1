# Library_Manager

[![CI](https://github.com/djdistraction/Library_Manager/actions/workflows/ci.yml/badge.svg)](https://github.com/djdistraction/Library_Manager/actions/workflows/ci.yml)

The uniQuE-ui Library Manager is a standalone desktop application for Windows, designed to be the definitive music library management tool for DJs, music enthusiasts, and other entertainment professionals.

## Features

### 🎵 Music Library Management
- **Smart Table View**: Browse your music library in a comprehensive table with columns for Title, Artist, Album, Genre, BPM, Key, and Duration
- **Real-time Search**: Instantly filter your library with the built-in search functionality
- **Automatic Scanning**: Recursively scan directories to find and index all audio files
- **Metadata Extraction**: Automatically extracts and displays track metadata from audio files
- **Background Processing**: Non-blocking analysis of tracks with progress tracking

### 📁 Playlist Management
- **Virtual Folders**: Create and organize playlists (virtual folders) to categorize your music
- **Tree View**: Hierarchical display of all playlists with track counts
- **Verified Folders**: Visual indicators show which playlists contain tracks (green checkmark icon)
- **Drag-and-Drop** (Coming Soon): Easy track organization between playlists

### 🎧 DJ Software Integration
- **Rekordbox Export**: Export your entire library or specific playlists to Rekordbox-compatible XML format
- **Complete Metadata**: Exports all track information including BPM, key, duration, and file locations
- **Playlist Preservation**: Maintains playlist structure when exporting
- **Cue Point Support**: Basic tempo markers included in export

### 🚀 Onboarding Experience
- **Sandbox Compatibility Test**: New users are guided through a safe test of the application's audio processing
- **Before/After Comparison**: View metadata before and after processing to ensure data integrity
- **No File Modification**: All testing happens in a temporary directory - your original files are never touched

### 💾 Database Backend
- **SQLite Database**: Fast, reliable embedded database for storing library information
- **Efficient Indexing**: Optimized queries for fast searching and filtering
- **CRUD Operations**: Complete Create, Read, Update, Delete support for all data
- **Transaction Support**: Atomic operations for data consistency

## Screenshots

(Screenshots will be added after UI testing on a system with display)

## Building from Source

### Prerequisites
- CMake 3.22 or higher
- Visual Studio 2019 or higher with C++20 support
- JUCE Framework (included as submodule)
- vcpkg (for dependency management)

### Optional Dependencies
- Chromaprint (for audio fingerprinting - will be disabled if not found)

### Build Instructions

#### Windows
```powershell
# Clone repository and submodules
git clone https://github.com/djdistraction/Library_Manager.git
cd Library_Manager
git submodule update --init JUCE

# Install dependencies using vcpkg
vcpkg install sqlite3:x64-windows

# Build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Run
.\bin\Release\Library Manager.exe
```

**Note**: If you haven't installed vcpkg yet, follow the [vcpkg installation guide](https://github.com/microsoft/vcpkg#quick-start-windows).

## System Requirements

### Minimum Requirements
- **Operating System**: Windows 10 (64-bit) or later
- **Processor**: Intel Core i3 or AMD equivalent
- **Memory**: 4 GB RAM
- **Storage**: 100 MB available space (plus space for your music library database)
- **Graphics**: DirectX 11 compatible graphics card

### Recommended Requirements
- **Operating System**: Windows 11 (64-bit)
- **Processor**: Intel Core i5 or AMD Ryzen 5
- **Memory**: 8 GB RAM or more
- **Storage**: SSD with at least 500 MB available space
- **Graphics**: DirectX 12 compatible graphics card

## Usage Guide

### First Launch
1. On first launch, you'll see the **Onboarding** screen
2. Click **"Select Test Files"** and choose 2-5 audio files
3. Click **"Start Compatibility Test"** to verify audio processing
4. Review the before/after metadata comparison
5. Click **"Complete Setup"** to proceed to the main interface

### Scanning Your Library
1. Click the **"Scan Library"** button in the top toolbar
2. Select the root directory of your music collection
3. Wait for the scan to complete (progress shown in status bar)
4. Tracks will appear in the main table as they're processed

### Searching Your Library
1. Use the **search box** at the top of the window
2. Type any text to filter tracks by title, artist, album, or genre
3. Results update in real-time as you type

### Creating Playlists
1. Click **"New Playlist"** button
2. A new playlist is created with a timestamp
3. The playlist appears in the left sidebar
4. (Track assignment coming in future update)

### Exporting to Rekordbox
1. Click **"Export to Rekordbox"** button
2. Choose a location to save the XML file
3. Monitor export progress in the status bar
4. Import the XML file into Rekordbox DJ software

## File Structure

```
Library_Manager/
├── Source/
│   ├── Main.cpp                    # Application entry point
│   ├── MainComponent.*             # Main UI component
│   ├── DatabaseManager.*           # SQLite database interface
│   ├── FileScanner.*               # Directory scanning
│   ├── AnalysisWorker.*            # Background audio analysis
│   ├── LibraryTableComponent.*     # Track table view
│   ├── PlaylistTreeComponent.*     # Playlist tree view
│   ├── OnboardingComponent.*       # First-run experience
│   ├── RekordboxExporter.*         # Rekordbox XML export
│   └── AcoustIDFingerprinter.*     # Audio fingerprinting (optional)
├── JUCE/                           # JUCE Framework (submodule)
├── CMakeLists.txt                  # Build configuration
├── DATABASE.md                     # Database schema documentation
├── UI_COMPONENTS.md                # UI components documentation
└── README.md                       # This file
```

## Documentation

- [DATABASE.md](DATABASE.md) - Complete database schema and API reference
- [UI_COMPONENTS.md](UI_COMPONENTS.md) - UI components documentation
- [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) - Implementation details

## Database Location

The application stores its database at:
- **Windows**: `%APPDATA%\LibraryManager\library.db`

This typically resolves to: `C:\Users\<YourUsername>\AppData\Roaming\LibraryManager\library.db`

## Supported Audio Formats

- MP3 (.mp3)
- WAV (.wav)
- FLAC (.flac)
- M4A (.m4a)
- AAC (.aac)
- OGG (.ogg)

## Known Limitations

1. **Chromaprint**: Fingerprinting features disabled if library not available
2. **Playlist Editing**: Drag-and-drop not yet implemented
3. **Cue Points**: Only basic tempo markers in Rekordbox export
4. **Thread Safety**: DatabaseManager not thread-safe (use mutex if accessing from multiple threads)

## Future Roadmap

- [ ] Advanced cue point editing
- [ ] Waveform visualization
- [ ] Drag-and-drop track organization
- [ ] Batch metadata editing
- [ ] Additional export formats (Serato, Traktor, etc.)
- [ ] Audio previewing
- [ ] Smart playlists with auto-filtering
- [ ] Cloud backup integration

## Contributing

This project is currently in active development. Contributions, bug reports, and feature requests are welcome!

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See [LICENSE](LICENSE) for full details.

## Credits

- **Framework**: [JUCE](https://juce.com/) - Cross-platform C++ framework
- **Database**: [SQLite](https://www.sqlite.org/) - Embedded SQL database
- **Fingerprinting**: [Chromaprint](https://acoustid.org/chromaprint) (optional)

## Contact

For support or questions, please open an issue on GitHub.

---

**uniQuE-ui Library Manager** - Professional music library management for DJs and audio professionals.
