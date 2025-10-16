/*
  ==============================================================================

    uniQuE-ui Library Manager
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

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <sqlite3.h>
#include <memory>
#include <vector>

//==============================================================================
/**
    DatabaseManager handles all SQLite database operations for the Library Manager.
    It manages the database file, creates tables, and provides CRUD operations
    for Tracks, VirtualFolders, Folder_Tracks_Link, and Jobs tables.
*/
class DatabaseManager
{
public:
    //==============================================================================
    // Data structures for each table
    
    struct Track
    {
        int64_t id = 0;
        juce::String filePath;
        juce::String title;
        juce::String artist;
        juce::String album;
        juce::String genre;
        int bpm = 0;
        juce::String key;
        double duration = 0.0;
        int64_t fileSize = 0;
        juce::String fileHash;
        juce::String acoustidFingerprint;
        juce::Time dateAdded;
        juce::Time lastModified;
    };
    
    struct VirtualFolder
    {
        int64_t id = 0;
        juce::String name;
        juce::String description;
        juce::Time dateCreated;
        bool isSmartPlaylist = false;
        juce::String smartCriteria;  // Semicolon-delimited string with filter criteria (key:value pairs)
    };
    
    struct FolderTrackLink
    {
        int64_t id = 0;
        int64_t folderId = 0;
        int64_t trackId = 0;
        int displayOrder = 0;
        juce::Time dateAdded;
    };
    
    struct Job
    {
        int64_t id = 0;
        juce::String jobType;
        juce::String status;
        juce::String parameters;
        juce::Time dateCreated;
        juce::Time dateStarted;
        juce::Time dateCompleted;
        juce::String errorMessage;
        int progress = 0;
    };
    
    struct CuePoint
    {
        int64_t id = 0;
        int64_t trackId = 0;
        double position = 0.0;  // Position in seconds
        juce::String name;
        int type = 0;  // 0=Memory Cue, 1=Hot Cue, 2=Loop In, 3=Loop Out
        int hotCueNumber = -1;  // Hot cue number (0-7), -1 for non-hot cues
        juce::String color;  // Hex color code
        juce::Time dateCreated;
    };

    //==============================================================================
    DatabaseManager();
    ~DatabaseManager();
    
    // Initialize the database
    bool initialize(const juce::File& databaseFile);
    
    // Close the database
    void close();
    
    // Check if database is open
    bool isOpen() const;
    
    //==============================================================================
    // CRUD operations for Tracks
    
    bool addTrack(const Track& track, int64_t& outId);
    bool updateTrack(const Track& track);
    bool deleteTrack(int64_t trackId);
    Track getTrack(int64_t trackId) const;
    std::vector<Track> getAllTracks() const;
    std::vector<Track> searchTracks(const juce::String& searchTerm) const;
    
    /**
     * Find tracks with the same AcoustID fingerprint (potential duplicates).
     * @param fingerprint The AcoustID fingerprint to search for
     * @return Vector of tracks with matching fingerprint
     */
    std::vector<Track> findTracksByFingerprint(const juce::String& fingerprint) const;
    
    //==============================================================================
    // CRUD operations for VirtualFolders
    
    bool addVirtualFolder(const VirtualFolder& folder, int64_t& outId);
    bool updateVirtualFolder(const VirtualFolder& folder);
    bool deleteVirtualFolder(int64_t folderId);
    VirtualFolder getVirtualFolder(int64_t folderId) const;
    std::vector<VirtualFolder> getAllVirtualFolders() const;
    
    /**
     * Evaluate smart playlist criteria and return matching tracks.
     * Smart criteria format: Semicolon-delimited key:value pairs like "artist:value;genre:value;bpmMin:120;bpmMax:140"
     */
    std::vector<Track> evaluateSmartPlaylist(const VirtualFolder& folder) const;
    
    //==============================================================================
    // CRUD operations for Folder_Tracks_Link
    
    bool addFolderTrackLink(const FolderTrackLink& link, int64_t& outId);
    bool updateFolderTrackLink(const FolderTrackLink& link);
    bool deleteFolderTrackLink(int64_t linkId);
    bool removeTrackFromFolder(int64_t folderId, int64_t trackId);
    std::vector<Track> getTracksInFolder(int64_t folderId) const;
    std::vector<VirtualFolder> getFoldersForTrack(int64_t trackId) const;
    
    //==============================================================================
    // CRUD operations for Jobs
    
    bool addJob(const Job& job, int64_t& outId);
    bool updateJob(const Job& job);
    bool deleteJob(int64_t jobId);
    Job getJob(int64_t jobId) const;
    std::vector<Job> getAllJobs() const;
    std::vector<Job> getJobsByStatus(const juce::String& status) const;
    
    //==============================================================================
    // CRUD operations for CuePoints
    
    bool addCuePoint(const CuePoint& cuePoint, int64_t& outId);
    bool updateCuePoint(const CuePoint& cuePoint);
    bool deleteCuePoint(int64_t cuePointId);
    CuePoint getCuePoint(int64_t cuePointId) const;
    std::vector<CuePoint> getCuePointsForTrack(int64_t trackId) const;
    bool deleteAllCuePointsForTrack(int64_t trackId);
    
    //==============================================================================
    // Transaction support
    
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // Get last error message
    juce::String getLastError() const;

private:
    //==============================================================================
    sqlite3* db = nullptr;
    juce::String lastError;
    mutable juce::CriticalSection dbMutex;  // Thread safety for database operations
    
    // Helper methods
    bool createTables();
    bool executeSQL(const juce::String& sql);
    bool checkTableExists(const juce::String& tableName) const;
    
    // Helper for converting JUCE Time to SQLite timestamp
    static juce::String timeToString(const juce::Time& time);
    static juce::Time stringToTime(const juce::String& timeStr);
    
    // Logging helper
    void logError(const juce::String& context, const juce::String& error);
    void logInfo(const juce::String& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DatabaseManager)
};
