# uniQuE-ui Library Manager - Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Application Entry Point                          │
│                              (Main.cpp)                                  │
└───────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           MainComponent                                  │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  Title Bar    [Search Box]   [Scan] [Export] [New Playlist]     │  │
│  ├──────────────┬───────────────────────────────────────────────────┤  │
│  │              │                                                    │  │
│  │  Playlists   │         Library Table                             │  │
│  │  TreeView    │         (TableListBox)                            │  │
│  │  (30%)       │         (70%)                                     │  │
│  │              │                                                    │  │
│  ├──────────────┴───────────────────────────────────────────────────┤  │
│  │  Status Bar                              Progress Info           │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└───────────────────┬───────────────────────┬────────────────────────────┘
                    │                       │
        ┌───────────┴──────────┐           │
        │                      │           │
        ▼                      ▼           ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│  Onboarding      │  │  LibraryTable    │  │  PlaylistTree    │
│  Component       │  │  Component       │  │  Component       │
│  (First Run)     │  │  (Track List)    │  │  (Folders)       │
└──────────────────┘  └──────────────────┘  └──────────────────┘
        │                      │                       │
        │                      │                       │
        ▼                      ▼                       ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         Backend Services                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │
│  │  Database    │  │  File        │  │  Analysis    │  │  Rekordbox │ │
│  │  Manager     │  │  Scanner     │  │  Worker      │  │  Exporter  │ │
│  │              │  │              │  │              │  │            │ │
│  │  • Tracks    │  │  • Scan dirs │  │  • Queue     │  │  • XML Gen │ │
│  │  • Playlists │  │  • Metadata  │  │  • Process   │  │  • Export  │ │
│  │  • Links     │  │  • Progress  │  │  • Callback  │  │  • Format  │ │
│  │  • Jobs      │  │              │  │              │  │            │ │
│  └──────────────┘  └──────────────┘  └──────────────┘  └────────────┘ │
└───────────────────────────────┬─────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         Data Layer                                       │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    SQLite Database                                │  │
│  │                                                                   │  │
│  │  • Tracks Table       (metadata, paths, hashes)                  │  │
│  │  • VirtualFolders     (playlists, collections)                   │  │
│  │  • Folder_Tracks_Link (many-to-many relationships)               │  │
│  │  • Jobs Table         (background task tracking)                 │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                        Component Interactions                            │
└─────────────────────────────────────────────────────────────────────────┘

User Action Flow:
=================

1. First Launch → Onboarding
   ┌──────────────┐
   │ Select Files │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐    ┌──────────────┐
   │ Process Test │───→│ Show Results │
   │  in TempDir  │    │ (Before/After)│
   └──────────────┘    └──────┬───────┘
                               │
                               ▼
                        ┌──────────────┐
                        │ Switch to    │
                        │ Main UI      │
                        └──────────────┘

2. Scan Library
   ┌──────────────┐
   │ Select Dir   │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
   │ FileScanner  │───→│ Add to Jobs  │───→│ Analysis     │
   │ Recursively  │    │ Queue        │    │ Worker       │
   └──────────────┘    └──────────────┘    └──────┬───────┘
                                                   │
                                                   ▼
                                            ┌──────────────┐
                                            │ Update DB &  │
                                            │ Refresh UI   │
                                            └──────────────┘

3. Search Library
   ┌──────────────┐
   │ Type in      │
   │ Search Box   │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐    ┌──────────────┐
   │ Filter SQL   │───→│ Update Table │
   │ Query        │    │ Display      │
   └──────────────┘    └──────────────┘

4. Export to Rekordbox
   ┌──────────────┐
   │ Click Export │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
   │ Select File  │───→│ Generate XML │───→│ Write File & │
   │ Location     │    │ (Background) │    │ Show Success │
   └──────────────┘    └──────────────┘    └──────────────┘

Data Flow:
==========

Audio File → FileScanner → Job Queue → AnalysisWorker → DatabaseManager → UI Update
                                                              ↓
                                                         SQLite DB
                                                              ↓
                                                         RekordboxExporter
                                                              ↓
                                                         XML File

Threading Model:
================

Main Thread (UI):
- MainComponent
- LibraryTableComponent
- PlaylistTreeComponent
- OnboardingComponent
- All user interactions

Background Threads:
- FileScanner (directory scanning)
- AnalysisWorker (audio processing)
- RekordboxExporter (XML generation)

Thread Safety:
- DatabaseManager: Not thread-safe (single-threaded access)
- Progress callbacks: Use MessageManager::callAsync for UI updates
- File operations: Async with callbacks to main thread

```

## Key Design Decisions

### 1. Component Separation
- **UI Components**: Independent, reusable
- **Backend Services**: Stateless, focused on single responsibility
- **Database Layer**: Centralized data access

### 2. Async Operations
- All long-running operations run in background threads
- Progress callbacks for UI updates
- Non-blocking user interface

### 3. First-Run Experience
- Automatic detection of empty library
- Safe testing environment (temporary directory)
- Smooth transition to main interface

### 4. Data Persistence
- SQLite for reliability and performance
- CRUD operations for all entities
- Transaction support for consistency

### 5. Export Compatibility
- Standard Rekordbox XML v1.0.0 format
- Complete metadata preservation
- File:// URL format for paths
- Basic cue point support

## Performance Characteristics

### Database Queries
- **Indexed**: Fast searches on artist, album, genre, BPM, key
- **Prepared Statements**: SQL injection prevention, performance
- **Batch Operations**: Transaction support for bulk inserts

### UI Responsiveness
- **Periodic Refresh**: 5-second intervals (configurable)
- **Search**: Real-time filtering without lag
- **Progress Updates**: Smooth progress bar updates

### Memory Usage
- **Smart Pointers**: Automatic memory management
- **RAII Pattern**: Resource cleanup guaranteed
- **Component Lifecycle**: Proper initialization and destruction

### File Operations
- **Async Scanning**: Non-blocking directory traversal
- **Stream Processing**: Efficient large file handling
- **Temp Directory**: Safe testing without disk space waste

## Extension Points

### Future Enhancements
1. **Drag-and-Drop**: Framework ready, needs event handlers
2. **Waveform Display**: Audio data available via AnalysisWorker
3. **Batch Edit**: Multi-selection already supported
4. **Custom Filters**: SQL query builder extensible
5. **Additional Exports**: Template pattern for new formats

### Plugin Architecture (Future)
- Export format plugins
- Audio analysis plugins
- UI theme plugins
- Database migration plugins
