/*
  ==============================================================================

    Library Manager v1.0.1
    Copyright (C) 2025 uniQuE-ui

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#include "DatabaseManager.h"

//==============================================================================
DatabaseManager::DatabaseManager()
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

//==============================================================================
bool DatabaseManager::initialize(const juce::File& databaseFile)
{
    const juce::ScopedLock lock(dbMutex);
    
    // Close any existing connection
    if (db != nullptr)
    {
        sqlite3_close(db);
        db = nullptr;
    }
    
    // Check if database file exists
    bool databaseExists = databaseFile.existsAsFile();
    
    // Open or create the database
    int result = sqlite3_open(databaseFile.getFullPathName().toRawUTF8(), &db);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to open database: ") + sqlite3_errmsg(db);
        logError("initialize", lastError);
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    
    logInfo("Database opened: " + databaseFile.getFullPathName());
    
    // Enable foreign keys
    executeSQL("PRAGMA foreign_keys = ON");
    
    // If database didn't exist or tables don't exist, create them
    if (!databaseExists || !checkTableExists("Tracks"))
    {
        logInfo("Creating database tables...");
        if (!createTables())
        {
            logError("initialize", "Failed to create tables");
            close();
            return false;
        }
        logInfo("Database tables created successfully");
    }
    else
    {
        // Check if acoustid_fingerprint column exists and add it if not (migration)
        sqlite3_stmt* stmt = nullptr;
        const char* pragmaQuery = "PRAGMA table_info(Tracks)";
        
        if (sqlite3_prepare_v2(db, pragmaQuery, -1, &stmt, nullptr) == SQLITE_OK)
        {
            bool hasAcoustidColumn = false;
            
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char* columnName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                if (columnName && juce::String(columnName) == "acoustid_fingerprint")
                {
                    hasAcoustidColumn = true;
                    break;
                }
            }
            
            sqlite3_finalize(stmt);
            
            if (!hasAcoustidColumn)
            {
                logInfo("Adding acoustid_fingerprint column to Tracks table...");
                if (executeSQL("ALTER TABLE Tracks ADD COLUMN acoustid_fingerprint TEXT"))
                {
                    logInfo("Successfully added acoustid_fingerprint column");
                }
                else
                {
                    logError("initialize", "Failed to add acoustid_fingerprint column");
                }
            }
        }
        
        // Check if smart playlist columns exist in VirtualFolders and add if not
        stmt = nullptr;
        const char* folderPragmaQuery = "PRAGMA table_info(VirtualFolders)";
        
        if (sqlite3_prepare_v2(db, folderPragmaQuery, -1, &stmt, nullptr) == SQLITE_OK)
        {
            bool hasSmartColumns = false;
            
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char* columnName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                if (columnName && juce::String(columnName) == "is_smart_playlist")
                {
                    hasSmartColumns = true;
                    break;
                }
            }
            
            sqlite3_finalize(stmt);
            
            if (!hasSmartColumns)
            {
                logInfo("Adding smart playlist columns to VirtualFolders table...");
                if (executeSQL("ALTER TABLE VirtualFolders ADD COLUMN is_smart_playlist INTEGER DEFAULT 0") &&
                    executeSQL("ALTER TABLE VirtualFolders ADD COLUMN smart_criteria TEXT"))
                {
                    logInfo("Successfully added smart playlist columns");
                }
                else
                {
                    logError("initialize", "Failed to add smart playlist columns");
                }
            }
        }
    }
    
    return true;
}

void DatabaseManager::close()
{
    const juce::ScopedLock lock(dbMutex);
    
    if (db != nullptr)
    {
        sqlite3_close(db);
        db = nullptr;
        logInfo("Database closed");
    }
}

bool DatabaseManager::isOpen() const
{
    const juce::ScopedLock lock(dbMutex);
    return db != nullptr;
}

//==============================================================================
bool DatabaseManager::createTables()
{
    // Create Tracks table
    const char* createTracksTable = R"(
        CREATE TABLE IF NOT EXISTS Tracks (
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
            acoustid_fingerprint TEXT,
            date_added TEXT NOT NULL,
            last_modified TEXT NOT NULL
        )
    )";
    
    if (!executeSQL(createTracksTable))
        return false;
    
    // Create indices for Tracks table
    executeSQL("CREATE INDEX IF NOT EXISTS idx_tracks_artist ON Tracks(artist)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_tracks_album ON Tracks(album)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_tracks_genre ON Tracks(genre)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_tracks_bpm ON Tracks(bpm)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_tracks_key ON Tracks(key)");
    
    // Create VirtualFolders table
    const char* createVirtualFoldersTable = R"(
        CREATE TABLE IF NOT EXISTS VirtualFolders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            date_created TEXT NOT NULL
        )
    )";
    
    if (!executeSQL(createVirtualFoldersTable))
        return false;
    
    // Create Folder_Tracks_Link table
    const char* createFolderTracksLinkTable = R"(
        CREATE TABLE IF NOT EXISTS Folder_Tracks_Link (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_id INTEGER NOT NULL,
            track_id INTEGER NOT NULL,
            display_order INTEGER DEFAULT 0,
            date_added TEXT NOT NULL,
            FOREIGN KEY (folder_id) REFERENCES VirtualFolders(id) ON DELETE CASCADE,
            FOREIGN KEY (track_id) REFERENCES Tracks(id) ON DELETE CASCADE,
            UNIQUE(folder_id, track_id)
        )
    )";
    
    if (!executeSQL(createFolderTracksLinkTable))
        return false;
    
    executeSQL("CREATE INDEX IF NOT EXISTS idx_folder_tracks_folder ON Folder_Tracks_Link(folder_id)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_folder_tracks_track ON Folder_Tracks_Link(track_id)");
    
    // Create Jobs table
    const char* createJobsTable = R"(
        CREATE TABLE IF NOT EXISTS Jobs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            job_type TEXT NOT NULL,
            status TEXT NOT NULL,
            parameters TEXT,
            date_created TEXT NOT NULL,
            date_started TEXT,
            date_completed TEXT,
            error_message TEXT,
            progress INTEGER DEFAULT 0
        )
    )";
    
    if (!executeSQL(createJobsTable))
        return false;
    
    executeSQL("CREATE INDEX IF NOT EXISTS idx_jobs_status ON Jobs(status)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_jobs_type ON Jobs(job_type)");
    
    // Create CuePoints table
    const char* createCuePointsTable = R"(
        CREATE TABLE IF NOT EXISTS CuePoints (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            track_id INTEGER NOT NULL,
            position REAL NOT NULL,
            name TEXT,
            type INTEGER DEFAULT 0,
            hot_cue_number INTEGER DEFAULT -1,
            color TEXT,
            date_created TEXT NOT NULL,
            FOREIGN KEY (track_id) REFERENCES Tracks(id) ON DELETE CASCADE
        )
    )";
    
    if (!executeSQL(createCuePointsTable))
        return false;
    
    executeSQL("CREATE INDEX IF NOT EXISTS idx_cuepoints_track ON CuePoints(track_id)");
    
    return true;
}

bool DatabaseManager::executeSQL(const juce::String& sql)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    char* errorMsg = nullptr;
    int result = sqlite3_exec(db, sql.toRawUTF8(), nullptr, nullptr, &errorMsg);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("SQL execution failed: ") + juce::String(errorMsg);
        logError("executeSQL", lastError + "\nSQL: " + sql);
        sqlite3_free(errorMsg);
        return false;
    }
    
    return true;
}

bool DatabaseManager::checkTableExists(const juce::String& tableName) const
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
        return false;
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, tableName.toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    
    return exists;
}

//==============================================================================
// CRUD operations for Tracks

bool DatabaseManager::addTrack(const Track& track, int64_t& outId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO Tracks (file_path, title, artist, album, genre, bpm, key, 
                          duration, file_size, file_hash, acoustid_fingerprint, date_added, last_modified)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("addTrack", lastError);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, track.filePath.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, track.title.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, track.artist.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, track.album.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, track.genre.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, track.bpm);
    sqlite3_bind_text(stmt, 7, track.key.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 8, track.duration);
    sqlite3_bind_int64(stmt, 9, track.fileSize);
    sqlite3_bind_text(stmt, 10, track.fileHash.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, track.acoustidFingerprint.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, timeToString(track.dateAdded).toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, timeToString(track.lastModified).toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to insert track: ") + sqlite3_errmsg(db);
        logError("addTrack", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    outId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    
    logInfo("Track added with ID: " + juce::String(outId));
    return true;
}

bool DatabaseManager::updateTrack(const Track& track)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        UPDATE Tracks SET file_path=?, title=?, artist=?, album=?, genre=?, bpm=?, 
                         key=?, duration=?, file_size=?, file_hash=?, acoustid_fingerprint=?, last_modified=?
        WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("updateTrack", lastError);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, track.filePath.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, track.title.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, track.artist.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, track.album.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, track.genre.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, track.bpm);
    sqlite3_bind_text(stmt, 7, track.key.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 8, track.duration);
    sqlite3_bind_int64(stmt, 9, track.fileSize);
    sqlite3_bind_text(stmt, 10, track.fileHash.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, track.acoustidFingerprint.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, timeToString(track.lastModified).toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 13, track.id);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to update track: ") + sqlite3_errmsg(db);
        logError("updateTrack", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Track updated: " + juce::String(track.id));
    return true;
}

bool DatabaseManager::deleteTrack(int64_t trackId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM Tracks WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("deleteTrack", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, trackId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to delete track: ") + sqlite3_errmsg(db);
        logError("deleteTrack", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Track deleted: " + juce::String(trackId));
    return true;
}

DatabaseManager::Track DatabaseManager::getTrack(int64_t trackId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    Track track;
    
    if (!isOpen())
        return track;
    
    const char* sql = R"(
        SELECT id, file_path, title, artist, album, genre, bpm, key, 
               duration, file_size, file_hash, acoustid_fingerprint, date_added, last_modified
        FROM Tracks WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return track;
    
    sqlite3_bind_int64(stmt, 1, trackId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        track.id = sqlite3_column_int64(stmt, 0);
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 1);
            track.filePath = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 2);
            track.title = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 3);
            track.artist = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 4);
            track.album = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 5);
            track.genre = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        track.bpm = sqlite3_column_int(stmt, 6);
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 7);
            track.key = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        track.duration = sqlite3_column_double(stmt, 8);
        track.fileSize = sqlite3_column_int64(stmt, 9);
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 10);
            track.fileHash = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 11);
            track.acoustidFingerprint = val ? juce::CharPointer_UTF8(val) : juce::String();
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 12);
            track.dateAdded = stringToTime(val ? juce::CharPointer_UTF8(val) : juce::String());
        }
        {
            const char* val = (const char*)sqlite3_column_text(stmt, 13);
            track.lastModified = stringToTime(val ? juce::CharPointer_UTF8(val) : juce::String());
        }
    }
    
    sqlite3_finalize(stmt);
    return track;
}

std::vector<DatabaseManager::Track> DatabaseManager::getAllTracks() const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Track> tracks;
    
    if (!isOpen())
        return tracks;
    
    const char* sql = R"(
        SELECT id, file_path, title, artist, album, genre, bpm, key, 
               duration, file_size, file_hash, acoustid_fingerprint, date_added, last_modified
        FROM Tracks ORDER BY title
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return tracks;
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Track track;
        track.id = sqlite3_column_int64(stmt, 0);
        track.filePath = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        track.title = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        track.artist = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        track.album = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4));
        track.genre = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
        track.bpm = sqlite3_column_int(stmt, 6);
        track.key = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        track.duration = sqlite3_column_double(stmt, 8);
        track.fileSize = sqlite3_column_int64(stmt, 9);
        track.fileHash = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 10));
        track.acoustidFingerprint = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 11));
        track.dateAdded = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 12)));
        track.lastModified = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 13)));
        tracks.push_back(track);
    }
    
    sqlite3_finalize(stmt);
    return tracks;
}

std::vector<DatabaseManager::Track> DatabaseManager::searchTracks(const juce::String& searchTerm) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Track> tracks;
    
    if (!isOpen())
        return tracks;
    
    const char* sql = R"(
        SELECT id, file_path, title, artist, album, genre, bpm, key, 
               duration, file_size, file_hash, acoustid_fingerprint, date_added, last_modified
        FROM Tracks 
        WHERE title LIKE ? OR artist LIKE ? OR album LIKE ? OR genre LIKE ?
        ORDER BY title
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return tracks;
    
    juce::String searchPattern = "%" + searchTerm + "%";
    sqlite3_bind_text(stmt, 1, searchPattern.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, searchPattern.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, searchPattern.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, searchPattern.toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Track track;
        track.id = sqlite3_column_int64(stmt, 0);
        track.filePath = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        track.title = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        track.artist = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        track.album = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4));
        track.genre = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
        track.bpm = sqlite3_column_int(stmt, 6);
        track.key = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        track.duration = sqlite3_column_double(stmt, 8);
        track.fileSize = sqlite3_column_int64(stmt, 9);
        track.fileHash = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 10));
        track.acoustidFingerprint = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 11));
        track.dateAdded = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 12)));
        track.lastModified = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 13)));
        tracks.push_back(track);
    }
    
    sqlite3_finalize(stmt);
    return tracks;
}

std::vector<DatabaseManager::Track> DatabaseManager::findTracksByFingerprint(const juce::String& fingerprint) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Track> tracks;
    
    if (!isOpen() || fingerprint.isEmpty())
        return tracks;
    
    const char* sql = R"(
        SELECT id, file_path, title, artist, album, genre, bpm, key, 
               duration, file_size, file_hash, acoustid_fingerprint, date_added, last_modified
        FROM Tracks 
        WHERE acoustid_fingerprint = ?
        ORDER BY title
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return tracks;
    
    sqlite3_bind_text(stmt, 1, fingerprint.toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Track track;
        track.id = sqlite3_column_int64(stmt, 0);
        track.filePath = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        track.title = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        track.artist = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        track.album = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4));
        track.genre = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
        track.bpm = sqlite3_column_int(stmt, 6);
        track.key = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        track.duration = sqlite3_column_double(stmt, 8);
        track.fileSize = sqlite3_column_int64(stmt, 9);
        track.fileHash = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 10));
        track.acoustidFingerprint = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 11));
        track.dateAdded = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 12)));
        track.lastModified = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 13)));
        tracks.push_back(track);
    }
    
    sqlite3_finalize(stmt);
    return tracks;
}

//==============================================================================
// CRUD operations for VirtualFolders

bool DatabaseManager::addVirtualFolder(const VirtualFolder& folder, int64_t& outId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO VirtualFolders (name, description, date_created, is_smart_playlist, smart_criteria)
        VALUES (?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("addVirtualFolder", lastError);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, folder.name.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, folder.description.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, timeToString(folder.dateCreated).toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, folder.isSmartPlaylist ? 1 : 0);
    sqlite3_bind_text(stmt, 5, folder.smartCriteria.toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to insert virtual folder: ") + sqlite3_errmsg(db);
        logError("addVirtualFolder", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    outId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    
    logInfo("Virtual folder added with ID: " + juce::String(outId));
    return true;
}

bool DatabaseManager::updateVirtualFolder(const VirtualFolder& folder)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        UPDATE VirtualFolders SET name=?, description=?
        WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("updateVirtualFolder", lastError);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, folder.name.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, folder.description.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, folder.id);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to update virtual folder: ") + sqlite3_errmsg(db);
        logError("updateVirtualFolder", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Virtual folder updated: " + juce::String(folder.id));
    return true;
}

bool DatabaseManager::deleteVirtualFolder(int64_t folderId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM VirtualFolders WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("deleteVirtualFolder", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, folderId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to delete virtual folder: ") + sqlite3_errmsg(db);
        logError("deleteVirtualFolder", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Virtual folder deleted: " + juce::String(folderId));
    return true;
}

DatabaseManager::VirtualFolder DatabaseManager::getVirtualFolder(int64_t folderId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    VirtualFolder folder;
    
    if (!isOpen())
        return folder;
    
    const char* sql = R"(
        SELECT id, name, description, date_created, is_smart_playlist, smart_criteria
        FROM VirtualFolders WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return folder;
    
    sqlite3_bind_int64(stmt, 1, folderId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        folder.id = sqlite3_column_int64(stmt, 0);
        folder.name = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        folder.description = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        folder.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3)));
        folder.isSmartPlaylist = sqlite3_column_int(stmt, 4) != 0;
        folder.smartCriteria = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
    }
    
    sqlite3_finalize(stmt);
    return folder;
}

std::vector<DatabaseManager::VirtualFolder> DatabaseManager::getAllVirtualFolders() const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<VirtualFolder> folders;
    
    if (!isOpen())
        return folders;
    
    const char* sql = R"(
        SELECT id, name, description, date_created, is_smart_playlist, smart_criteria
        FROM VirtualFolders ORDER BY name
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return folders;
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        VirtualFolder folder;
        folder.id = sqlite3_column_int64(stmt, 0);
        folder.name = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        folder.description = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        folder.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3)));
        folder.isSmartPlaylist = sqlite3_column_int(stmt, 4) != 0;
        folder.smartCriteria = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
        folders.push_back(folder);
    }
    
    sqlite3_finalize(stmt);
    return folders;
}

//==============================================================================
// CRUD operations for Folder_Tracks_Link

bool DatabaseManager::addFolderTrackLink(const FolderTrackLink& link, int64_t& outId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO Folder_Tracks_Link (folder_id, track_id, display_order, date_added)
        VALUES (?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("addFolderTrackLink", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, link.folderId);
    sqlite3_bind_int64(stmt, 2, link.trackId);
    sqlite3_bind_int(stmt, 3, link.displayOrder);
    sqlite3_bind_text(stmt, 4, timeToString(link.dateAdded).toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to insert folder-track link: ") + sqlite3_errmsg(db);
        logError("addFolderTrackLink", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    outId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    
    logInfo("Folder-track link added with ID: " + juce::String(outId));
    return true;
}

bool DatabaseManager::updateFolderTrackLink(const FolderTrackLink& link)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        UPDATE Folder_Tracks_Link SET folder_id=?, track_id=?, display_order=?
        WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("updateFolderTrackLink", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, link.folderId);
    sqlite3_bind_int64(stmt, 2, link.trackId);
    sqlite3_bind_int(stmt, 3, link.displayOrder);
    sqlite3_bind_int64(stmt, 4, link.id);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to update folder-track link: ") + sqlite3_errmsg(db);
        logError("updateFolderTrackLink", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Folder-track link updated: " + juce::String(link.id));
    return true;
}

bool DatabaseManager::deleteFolderTrackLink(int64_t linkId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM Folder_Tracks_Link WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("deleteFolderTrackLink", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, linkId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to delete folder-track link: ") + sqlite3_errmsg(db);
        logError("deleteFolderTrackLink", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Folder-track link deleted: " + juce::String(linkId));
    return true;
}

bool DatabaseManager::removeTrackFromFolder(int64_t folderId, int64_t trackId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM Folder_Tracks_Link WHERE folder_id=? AND track_id=?";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("removeTrackFromFolder", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, folderId);
    sqlite3_bind_int64(stmt, 2, trackId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to remove track from folder: ") + sqlite3_errmsg(db);
        logError("removeTrackFromFolder", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Track removed from folder");
    return true;
}

std::vector<DatabaseManager::Track> DatabaseManager::getTracksInFolder(int64_t folderId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Track> tracks;
    
    if (!isOpen())
        return tracks;
    
    const char* sql = R"(
        SELECT t.id, t.file_path, t.title, t.artist, t.album, t.genre, t.bpm, t.key, 
               t.duration, t.file_size, t.file_hash, t.acoustid_fingerprint, t.date_added, t.last_modified
        FROM Tracks t
        INNER JOIN Folder_Tracks_Link ftl ON t.id = ftl.track_id
        WHERE ftl.folder_id = ?
        ORDER BY ftl.display_order, t.title
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return tracks;
    
    sqlite3_bind_int64(stmt, 1, folderId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Track track;
        track.id = sqlite3_column_int64(stmt, 0);
        track.filePath = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        track.title = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        track.artist = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        track.album = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4));
        track.genre = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
        track.bpm = sqlite3_column_int(stmt, 6);
        track.key = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        track.duration = sqlite3_column_double(stmt, 8);
        track.fileSize = sqlite3_column_int64(stmt, 9);
        track.fileHash = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 10));
        track.acoustidFingerprint = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 11));
        track.dateAdded = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 12)));
        track.lastModified = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 13)));
        tracks.push_back(track);
    }
    
    sqlite3_finalize(stmt);
    return tracks;
}

std::vector<DatabaseManager::VirtualFolder> DatabaseManager::getFoldersForTrack(int64_t trackId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<VirtualFolder> folders;
    
    if (!isOpen())
        return folders;
    
    const char* sql = R"(
        SELECT vf.id, vf.name, vf.description, vf.date_created
        FROM VirtualFolders vf
        INNER JOIN Folder_Tracks_Link ftl ON vf.id = ftl.folder_id
        WHERE ftl.track_id = ?
        ORDER BY vf.name
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return folders;
    
    sqlite3_bind_int64(stmt, 1, trackId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        VirtualFolder folder;
        folder.id = sqlite3_column_int64(stmt, 0);
        folder.name = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        folder.description = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        folder.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3)));
        folders.push_back(folder);
    }
    
    sqlite3_finalize(stmt);
    return folders;
}

//==============================================================================
// CRUD operations for Jobs

bool DatabaseManager::addJob(const Job& job, int64_t& outId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO Jobs (job_type, status, parameters, date_created, date_started, 
                         date_completed, error_message, progress)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("addJob", lastError);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, job.jobType.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, job.status.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, job.parameters.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, timeToString(job.dateCreated).toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    if (job.dateStarted != juce::Time())
        sqlite3_bind_text(stmt, 5, timeToString(job.dateStarted).toRawUTF8(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 5);
    
    if (job.dateCompleted != juce::Time())
        sqlite3_bind_text(stmt, 6, timeToString(job.dateCompleted).toRawUTF8(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 6);
    
    sqlite3_bind_text(stmt, 7, job.errorMessage.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, job.progress);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to insert job: ") + sqlite3_errmsg(db);
        logError("addJob", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    outId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    
    logInfo("Job added with ID: " + juce::String(outId));
    return true;
}

bool DatabaseManager::updateJob(const Job& job)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        UPDATE Jobs SET job_type=?, status=?, parameters=?, date_started=?, 
                       date_completed=?, error_message=?, progress=?
        WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("updateJob", lastError);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, job.jobType.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, job.status.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, job.parameters.toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    if (job.dateStarted != juce::Time())
        sqlite3_bind_text(stmt, 4, timeToString(job.dateStarted).toRawUTF8(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 4);
    
    if (job.dateCompleted != juce::Time())
        sqlite3_bind_text(stmt, 5, timeToString(job.dateCompleted).toRawUTF8(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 5);
    
    sqlite3_bind_text(stmt, 6, job.errorMessage.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, job.progress);
    sqlite3_bind_int64(stmt, 8, job.id);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to update job: ") + sqlite3_errmsg(db);
        logError("updateJob", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Job updated: " + juce::String(job.id));
    return true;
}

bool DatabaseManager::deleteJob(int64_t jobId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM Jobs WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("deleteJob", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, jobId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to delete job: ") + sqlite3_errmsg(db);
        logError("deleteJob", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Job deleted: " + juce::String(jobId));
    return true;
}

DatabaseManager::Job DatabaseManager::getJob(int64_t jobId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    Job job;
    
    if (!isOpen())
        return job;
    
    const char* sql = R"(
        SELECT id, job_type, status, parameters, date_created, date_started, 
               date_completed, error_message, progress
        FROM Jobs WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return job;
    
    sqlite3_bind_int64(stmt, 1, jobId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        job.id = sqlite3_column_int64(stmt, 0);
        {
            const char* jobTypeText = (const char*)sqlite3_column_text(stmt, 1);
            job.jobType = jobTypeText ? juce::CharPointer_UTF8(jobTypeText) : juce::String();
        }
        {
            const char* statusText = (const char*)sqlite3_column_text(stmt, 2);
            job.status = statusText ? juce::CharPointer_UTF8(statusText) : juce::String();
        }
        {
            const char* parametersText = (const char*)sqlite3_column_text(stmt, 3);
            job.parameters = parametersText ? juce::CharPointer_UTF8(parametersText) : juce::String();
        }
        {
            const char* dateCreatedText = (const char*)sqlite3_column_text(stmt, 4);
            job.dateCreated = dateCreatedText ? stringToTime(juce::CharPointer_UTF8(dateCreatedText)) : juce::Time();
        }
        
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL)
        {
            const char* dateStartedText = (const char*)sqlite3_column_text(stmt, 5);
            job.dateStarted = dateStartedText ? stringToTime(juce::CharPointer_UTF8(dateStartedText)) : juce::Time();
        }
        
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
        {
            const char* dateCompletedText = (const char*)sqlite3_column_text(stmt, 6);
            job.dateCompleted = dateCompletedText ? stringToTime(juce::CharPointer_UTF8(dateCompletedText)) : juce::Time();
        }
        
        {
            const char* errorMessageText = (const char*)sqlite3_column_text(stmt, 7);
            job.errorMessage = errorMessageText ? juce::CharPointer_UTF8(errorMessageText) : juce::String();
        }
        job.progress = sqlite3_column_int(stmt, 8);
    }
    
    sqlite3_finalize(stmt);
    return job;
}

std::vector<DatabaseManager::Job> DatabaseManager::getAllJobs() const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Job> jobs;
    
    if (!isOpen())
        return jobs;
    
    const char* sql = R"(
        SELECT id, job_type, status, parameters, date_created, date_started, 
               date_completed, error_message, progress
        FROM Jobs ORDER BY date_created DESC
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return jobs;
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Job job;
        job.id = sqlite3_column_int64(stmt, 0);
        job.jobType = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        job.status = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        job.parameters = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        job.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4)));
        
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL)
            job.dateStarted = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5)));
        
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
            job.dateCompleted = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 6)));
        
        job.errorMessage = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        job.progress = sqlite3_column_int(stmt, 8);
        jobs.push_back(job);
    }
    
    sqlite3_finalize(stmt);
    return jobs;
}

std::vector<DatabaseManager::Job> DatabaseManager::getJobsByStatus(const juce::String& status) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Job> jobs;
    
    if (!isOpen())
        return jobs;
    
    const char* sql = R"(
        SELECT id, job_type, status, parameters, date_created, date_started, 
               date_completed, error_message, progress
        FROM Jobs WHERE status=? ORDER BY date_created DESC
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return jobs;
    
    sqlite3_bind_text(stmt, 1, status.toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Job job;
        job.id = sqlite3_column_int64(stmt, 0);
        job.jobType = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        job.status = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        job.parameters = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        job.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4)));
        
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL)
            job.dateStarted = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5)));
        
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
            job.dateCompleted = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 6)));
        
        job.errorMessage = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        job.progress = sqlite3_column_int(stmt, 8);
        jobs.push_back(job);
    }
    
    sqlite3_finalize(stmt);
    return jobs;
}

//==============================================================================
// Transaction support

bool DatabaseManager::beginTransaction()
{
    const juce::ScopedLock lock(dbMutex);
    
    return executeSQL("BEGIN TRANSACTION");
}

bool DatabaseManager::commitTransaction()
{
    const juce::ScopedLock lock(dbMutex);
    
    return executeSQL("COMMIT");
}

bool DatabaseManager::rollbackTransaction()
{
    const juce::ScopedLock lock(dbMutex);
    
    return executeSQL("ROLLBACK");
}

//==============================================================================
// CRUD operations for CuePoints

bool DatabaseManager::addCuePoint(const CuePoint& cuePoint, int64_t& outId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO CuePoints (track_id, position, name, type, hot_cue_number, color, date_created)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("addCuePoint", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, cuePoint.trackId);
    sqlite3_bind_double(stmt, 2, cuePoint.position);
    sqlite3_bind_text(stmt, 3, cuePoint.name.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, cuePoint.type);
    sqlite3_bind_int(stmt, 5, cuePoint.hotCueNumber);
    sqlite3_bind_text(stmt, 6, cuePoint.color.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, timeToString(cuePoint.dateCreated).toRawUTF8(), -1, SQLITE_TRANSIENT);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to insert cue point: ") + sqlite3_errmsg(db);
        logError("addCuePoint", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    outId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    
    logInfo("Cue point added with ID: " + juce::String(outId));
    return true;
}

bool DatabaseManager::updateCuePoint(const CuePoint& cuePoint)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = R"(
        UPDATE CuePoints SET track_id=?, position=?, name=?, type=?, hot_cue_number=?, color=?
        WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("updateCuePoint", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, cuePoint.trackId);
    sqlite3_bind_double(stmt, 2, cuePoint.position);
    sqlite3_bind_text(stmt, 3, cuePoint.name.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, cuePoint.type);
    sqlite3_bind_int(stmt, 5, cuePoint.hotCueNumber);
    sqlite3_bind_text(stmt, 6, cuePoint.color.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, cuePoint.id);
    
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to update cue point: ") + sqlite3_errmsg(db);
        logError("updateCuePoint", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Cue point updated: " + juce::String(cuePoint.id));
    return true;
}

bool DatabaseManager::deleteCuePoint(int64_t cuePointId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM CuePoints WHERE id=?";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("deleteCuePoint", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, cuePointId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to delete cue point: ") + sqlite3_errmsg(db);
        logError("deleteCuePoint", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("Cue point deleted: " + juce::String(cuePointId));
    return true;
}

DatabaseManager::CuePoint DatabaseManager::getCuePoint(int64_t cuePointId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    CuePoint cuePoint;
    
    if (!isOpen())
        return cuePoint;
    
    const char* sql = R"(
        SELECT id, track_id, position, name, type, hot_cue_number, color, date_created
        FROM CuePoints WHERE id=?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return cuePoint;
    
    sqlite3_bind_int64(stmt, 1, cuePointId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        cuePoint.id = sqlite3_column_int64(stmt, 0);
        cuePoint.trackId = sqlite3_column_int64(stmt, 1);
        cuePoint.position = sqlite3_column_double(stmt, 2);
        cuePoint.name = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        cuePoint.type = sqlite3_column_int(stmt, 4);
        cuePoint.hotCueNumber = sqlite3_column_int(stmt, 5);
        cuePoint.color = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 6));
        cuePoint.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7)));
    }
    
    sqlite3_finalize(stmt);
    return cuePoint;
}

std::vector<DatabaseManager::CuePoint> DatabaseManager::getCuePointsForTrack(int64_t trackId) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<CuePoint> cuePoints;
    
    if (!isOpen())
        return cuePoints;
    
    const char* sql = R"(
        SELECT id, track_id, position, name, type, hot_cue_number, color, date_created
        FROM CuePoints WHERE track_id=? ORDER BY position
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return cuePoints;
    
    sqlite3_bind_int64(stmt, 1, trackId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        CuePoint cuePoint;
        cuePoint.id = sqlite3_column_int64(stmt, 0);
        cuePoint.trackId = sqlite3_column_int64(stmt, 1);
        cuePoint.position = sqlite3_column_double(stmt, 2);
        cuePoint.name = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        cuePoint.type = sqlite3_column_int(stmt, 4);
        cuePoint.hotCueNumber = sqlite3_column_int(stmt, 5);
        cuePoint.color = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 6));
        cuePoint.dateCreated = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7)));
        cuePoints.push_back(cuePoint);
    }
    
    sqlite3_finalize(stmt);
    return cuePoints;
}

bool DatabaseManager::deleteAllCuePointsForTrack(int64_t trackId)
{
    const juce::ScopedLock lock(dbMutex);
    
    if (!isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    const char* sql = "DELETE FROM CuePoints WHERE track_id=?";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
    {
        lastError = juce::String("Failed to prepare statement: ") + sqlite3_errmsg(db);
        logError("deleteAllCuePointsForTrack", lastError);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, trackId);
    result = sqlite3_step(stmt);
    
    if (result != SQLITE_DONE)
    {
        lastError = juce::String("Failed to delete cue points: ") + sqlite3_errmsg(db);
        logError("deleteAllCuePointsForTrack", lastError);
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    logInfo("All cue points deleted for track: " + juce::String(trackId));
    return true;
}

//==============================================================================
juce::String DatabaseManager::getLastError() const
{
    return lastError;
}

//==============================================================================
// Helper methods

juce::String DatabaseManager::timeToString(const juce::Time& time)
{
    if (time == juce::Time())
        return juce::String();
    
    return time.toISO8601(true);
}

juce::Time DatabaseManager::stringToTime(const juce::String& timeStr)
{
    if (timeStr.isEmpty())
        return juce::Time();
    
    return juce::Time::fromISO8601(timeStr);
}

void DatabaseManager::logError(const juce::String& context, const juce::String& error)
{
    DBG("[DatabaseManager ERROR] " + context + ": " + error);
}

void DatabaseManager::logInfo(const juce::String& message)
{
    DBG("[DatabaseManager] " + message);
}

//==============================================================================
// Smart Playlist Evaluation

std::vector<DatabaseManager::Track> DatabaseManager::evaluateSmartPlaylist(const VirtualFolder& folder) const
{
    const juce::ScopedLock lock(dbMutex);
    
    std::vector<Track> tracks;
    
    if (!isOpen() || !folder.isSmartPlaylist || folder.smartCriteria.isEmpty())
        return tracks;
    
    // Parse smart criteria (simplified JSON-like format)
    // Format: "artist:value;genre:value;bpmMin:120;bpmMax:140"
    juce::StringArray criteria;
    criteria.addTokens(folder.smartCriteria, ";", "");
    
    juce::String whereClause = "WHERE 1=1";
    
    for (const auto& criterion : criteria)
    {
        auto parts = juce::StringArray::fromTokens(criterion, ":", "");
        if (parts.size() != 2)
            continue;
        
        auto key = parts[0].trim();
        auto value = parts[1].trim();
        
        if (key == "artist")
            whereClause += " AND artist LIKE '%" + value + "%'";
        else if (key == "album")
            whereClause += " AND album LIKE '%" + value + "%'";
        else if (key == "genre")
            whereClause += " AND genre LIKE '%" + value + "%'";
        else if (key == "key")
            whereClause += " AND key = '" + value + "'";
        else if (key == "bpmMin")
            whereClause += " AND bpm >= " + value;
        else if (key == "bpmMax")
            whereClause += " AND bpm <= " + value;
    }
    
    juce::String sql = R"(
        SELECT id, file_path, title, artist, album, genre, bpm, key, 
               duration, file_size, file_hash, acoustid_fingerprint, date_added, last_modified
        FROM Tracks )" + whereClause + " ORDER BY title";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql.toRawUTF8(), -1, &stmt, nullptr);
    
    if (result != SQLITE_OK)
        return tracks;
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Track track;
        track.id = sqlite3_column_int64(stmt, 0);
        track.filePath = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 1));
        track.title = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 2));
        track.artist = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 3));
        track.album = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 4));
        track.genre = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 5));
        track.bpm = sqlite3_column_int(stmt, 6);
        track.key = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 7));
        track.duration = sqlite3_column_double(stmt, 8);
        track.fileSize = sqlite3_column_int64(stmt, 9);
        track.fileHash = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 10));
        track.acoustidFingerprint = juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 11));
        track.dateAdded = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 12)));
        track.lastModified = stringToTime(juce::CharPointer_UTF8((const char*)sqlite3_column_text(stmt, 13)));
        tracks.push_back(track);
    }
    
    sqlite3_finalize(stmt);
    
    DBG("[DatabaseManager] Smart playlist '" << folder.name << "' evaluated: " << tracks.size() << " tracks found");
    return tracks;
}
