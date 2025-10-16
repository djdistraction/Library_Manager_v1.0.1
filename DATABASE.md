# Database Layer Implementation

## Overview

The Library Manager application uses SQLite as its embedded database for storing music library information. The database is managed by the `DatabaseManager` C++ class, which provides a clean interface for all database operations.

## Database Location

The database file is automatically created at:
- **Linux/macOS**: `~/.config/LibraryManager/library.db` or equivalent application data directory
- **Windows**: `%APPDATA%\LibraryManager\library.db`

The database file is created automatically on first run if it doesn't exist.

## Schema

The database consists of four main tables:

### 1. Tracks Table

Stores information about individual music tracks.

```sql
CREATE TABLE Tracks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT NOT NULL UNIQUE,
    title TEXT,
    artist TEXT,
    album TEXT,
    genre TEXT,
    bpm INTEGER DEFAULT 0,
    key TEXT,
    duration REAL DEFAULT 0.0,
    file_size INTEGER DEFAULT 0,
    file_hash TEXT,
    date_added TEXT NOT NULL,
    last_modified TEXT NOT NULL
);
```

**Indices:**
- `idx_tracks_artist` on `artist`
- `idx_tracks_album` on `album`
- `idx_tracks_genre` on `genre`
- `idx_tracks_bpm` on `bpm`
- `idx_tracks_key` on `key`

### 2. VirtualFolders Table

Stores user-created virtual folders for organizing tracks.

```sql
CREATE TABLE VirtualFolders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT,
    date_created TEXT NOT NULL
);
```

### 3. Folder_Tracks_Link Table

Many-to-many relationship table linking tracks to virtual folders.

```sql
CREATE TABLE Folder_Tracks_Link (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    folder_id INTEGER NOT NULL,
    track_id INTEGER NOT NULL,
    display_order INTEGER DEFAULT 0,
    date_added TEXT NOT NULL,
    FOREIGN KEY (folder_id) REFERENCES VirtualFolders(id) ON DELETE CASCADE,
    FOREIGN KEY (track_id) REFERENCES Tracks(id) ON DELETE CASCADE,
    UNIQUE(folder_id, track_id)
);
```

**Indices:**
- `idx_folder_tracks_folder` on `folder_id`
- `idx_folder_tracks_track` on `track_id`

**Constraints:**
- Unique constraint prevents duplicate track-folder associations
- Cascade delete ensures orphaned links are removed

### 4. Jobs Table

Stores background job information for tasks like library scanning.

```sql
CREATE TABLE Jobs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    job_type TEXT NOT NULL,
    status TEXT NOT NULL,
    parameters TEXT,
    date_created TEXT NOT NULL,
    date_started TEXT,
    date_completed TEXT,
    error_message TEXT,
    progress INTEGER DEFAULT 0
);
```

**Indices:**
- `idx_jobs_status` on `status`
- `idx_jobs_type` on `job_type`

## DatabaseManager Class

### Key Features

1. **Automatic Database Creation**: Creates the database and all tables on first run
2. **CRUD Operations**: Complete Create, Read, Update, Delete support for all tables
3. **Transaction Support**: Begin, commit, and rollback transactions
4. **Error Handling**: Comprehensive error messages with logging
5. **Search Functionality**: Full-text search across track metadata
6. **Relationship Queries**: Get tracks in a folder, get folders for a track

### Public API

#### Initialization
```cpp
bool initialize(const juce::File& databaseFile);
void close();
bool isOpen() const;
```

#### Tracks Operations
```cpp
bool addTrack(const Track& track, int64_t& outId);
bool updateTrack(const Track& track);
bool deleteTrack(int64_t trackId);
Track getTrack(int64_t trackId) const;
std::vector<Track> getAllTracks() const;
std::vector<Track> searchTracks(const juce::String& searchTerm) const;
```

#### Virtual Folders Operations
```cpp
bool addVirtualFolder(const VirtualFolder& folder, int64_t& outId);
bool updateVirtualFolder(const VirtualFolder& folder);
bool deleteVirtualFolder(int64_t folderId);
VirtualFolder getVirtualFolder(int64_t folderId) const;
std::vector<VirtualFolder> getAllVirtualFolders() const;
```

#### Folder-Track Link Operations
```cpp
bool addFolderTrackLink(const FolderTrackLink& link, int64_t& outId);
bool updateFolderTrackLink(const FolderTrackLink& link);
bool deleteFolderTrackLink(int64_t linkId);
bool removeTrackFromFolder(int64_t folderId, int64_t trackId);
std::vector<Track> getTracksInFolder(int64_t folderId) const;
std::vector<VirtualFolder> getFoldersForTrack(int64_t trackId) const;
```

#### Jobs Operations
```cpp
bool addJob(const Job& job, int64_t& outId);
bool updateJob(const Job& job);
bool deleteJob(int64_t jobId);
Job getJob(int64_t jobId) const;
std::vector<Job> getAllJobs() const;
std::vector<Job> getJobsByStatus(const juce::String& status) const;
```

#### Transactions
```cpp
bool beginTransaction();
bool commitTransaction();
bool rollbackTransaction();
```

## Usage Example

```cpp
#include "DatabaseManager.h"

// Initialize database
DatabaseManager dbManager;
auto dbFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("LibraryManager")
                .getChildFile("library.db");

if (!dbManager.initialize(dbFile))
{
    DBG("Failed to initialize database: " + dbManager.getLastError());
    return;
}

// Add a track
DatabaseManager::Track track;
track.filePath = "/path/to/music.mp3";
track.title = "My Song";
track.artist = "Artist Name";
track.bpm = 128;
track.dateAdded = juce::Time::getCurrentTime();
track.lastModified = juce::Time::getCurrentTime();

int64_t trackId = 0;
if (dbManager.addTrack(track, trackId))
{
    DBG("Track added with ID: " + juce::String(trackId));
}

// Search for tracks
auto results = dbManager.searchTracks("Artist");
for (const auto& result : results)
{
    DBG("Found: " + result.title + " by " + result.artist);
}

// Create a virtual folder
DatabaseManager::VirtualFolder folder;
folder.name = "My Playlist";
folder.description = "Custom playlist";
folder.dateCreated = juce::Time::getCurrentTime();

int64_t folderId = 0;
if (dbManager.addVirtualFolder(folder, folderId))
{
    // Add track to folder
    DatabaseManager::FolderTrackLink link;
    link.folderId = folderId;
    link.trackId = trackId;
    link.displayOrder = 1;
    link.dateAdded = juce::Time::getCurrentTime();
    
    int64_t linkId = 0;
    dbManager.addFolderTrackLink(link, linkId);
}

// Get all tracks in folder
auto tracks = dbManager.getTracksInFolder(folderId);
```

## Error Handling

All database operations return `bool` to indicate success or failure. When an operation fails:

1. The method returns `false`
2. An error message is logged using JUCE's `DBG()` macro
3. The error message is stored and can be retrieved via `getLastError()`

Example:
```cpp
if (!dbManager.addTrack(track, trackId))
{
    juce::String error = dbManager.getLastError();
    // Handle error...
}
```

## Performance Considerations

1. **Indices**: Created on commonly queried fields (artist, album, genre, BPM, key)
2. **Prepared Statements**: All queries use prepared statements to prevent SQL injection and improve performance
3. **Transactions**: Use transactions for batch operations to improve performance
4. **Foreign Key Constraints**: Enabled to maintain data integrity

## Thread Safety

The DatabaseManager is **not thread-safe**. If you need to access the database from multiple threads:

1. Use a single DatabaseManager instance per thread, or
2. Implement appropriate locking mechanisms, or
3. Queue database operations to a single thread

## Future Enhancements

Potential improvements for future versions:

1. Connection pooling for multi-threaded access
2. Full-text search using SQLite FTS5
3. Caching layer for frequently accessed data
4. Database migration system for schema updates
5. Backup and restore functionality
6. Export/import capabilities
