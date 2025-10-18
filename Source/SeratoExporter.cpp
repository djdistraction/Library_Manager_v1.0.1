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

#include "SeratoExporter.h"

//==============================================================================
SeratoExporter::SeratoExporter(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
}

SeratoExporter::~SeratoExporter()
{
}

//==============================================================================
bool SeratoExporter::exportLibrary(const juce::File& outputDirectory,
                                   std::function<void(float)> progressCallback)
{
    if (!outputDirectory.exists())
    {
        if (!outputDirectory.createDirectory())
        {
            lastError = "Failed to create output directory: " + outputDirectory.getFullPathName();
            DBG("[SeratoExporter] " << lastError);
            return false;
        }
    }
    
    // Get all tracks
    auto tracks = databaseManager.getAllTracks();
    
    if (tracks.empty())
    {
        lastError = "No tracks to export";
        DBG("[SeratoExporter] " << lastError);
        return false;
    }
    
    // Create Serato database file
    juce::File dbFile = outputDirectory.getChildFile("database V2");
    if (!createDatabaseFile(dbFile, tracks))
    {
        return false;
    }
    
    if (progressCallback)
        progressCallback(0.5f);
    
    // Export all playlists as crates
    auto folders = databaseManager.getAllVirtualFolders();
    
    // Create Subcrates directory
    juce::File subcratesDir = outputDirectory.getChildFile("Subcrates");
    if (!subcratesDir.exists())
        subcratesDir.createDirectory();
    
    for (size_t i = 0; i < folders.size(); ++i)
    {
        const auto& folder = folders[i];
        auto folderTracks = databaseManager.getTracksInFolder(folder.id);
        
        juce::File crateFile = subcratesDir.getChildFile(folder.name + ".crate");
        createCrateFile(crateFile, folder, folderTracks);
        
        if (progressCallback)
        {
            float progress = 0.5f + (0.5f * (static_cast<float>(i + 1) / folders.size()));
            progressCallback(progress);
        }
    }
    
    DBG("[SeratoExporter] Export completed successfully to: " << outputDirectory.getFullPathName());
    return true;
}

bool SeratoExporter::exportPlaylist(int64_t folderId, const juce::File& outputDirectory)
{
    if (!outputDirectory.exists())
    {
        if (!outputDirectory.createDirectory())
        {
            lastError = "Failed to create output directory";
            return false;
        }
    }
    
    auto folder = databaseManager.getVirtualFolder(folderId);
    auto tracks = databaseManager.getTracksInFolder(folderId);
    
    juce::File crateFile = outputDirectory.getChildFile(folder.name + ".crate");
    return createCrateFile(crateFile, folder, tracks);
}

//==============================================================================
bool SeratoExporter::createDatabaseFile(const juce::File& dbFile,
                                       const std::vector<DatabaseManager::Track>& tracks)
{
    // Serato database format is proprietary binary format
    // This is a simplified version that creates a basic structure
    
    auto stream = dbFile.createOutputStream();
    
    if (stream == nullptr)
    {
        lastError = "Failed to create database file";
        DBG("[SeratoExporter] " << lastError);
        return false;
    }
    
    // Write header (simplified Serato format)
    stream->writeString("vrsn");
    writeSeratoInt32(*stream, 0x202); // Version 2.2
    
    // Write track entries
    for (const auto& track : tracks)
    {
        // Track entry marker
        stream->writeString("otrk");
        
        // File path
        stream->writeString("pfil");
        writeSeratoString(*stream, trackToSeratoPath(track));
        
        // Track title
        stream->writeString("tsng");
        writeSeratoString(*stream, track.title);
        
        // Artist
        stream->writeString("tart");
        writeSeratoString(*stream, track.artist);
        
        // Album
        stream->writeString("talb");
        writeSeratoString(*stream, track.album);
        
        // Genre
        stream->writeString("tgen");
        writeSeratoString(*stream, track.genre);
        
        // BPM
        if (track.bpm > 0)
        {
            stream->writeString("tbpm");
            writeSeratoString(*stream, juce::String(track.bpm));
        }
        
        // Key
        if (track.key.isNotEmpty())
        {
            stream->writeString("tkey");
            writeSeratoString(*stream, track.key);
        }
    }
    
    stream->flush();
    DBG("[SeratoExporter] Created database file with " << tracks.size() << " tracks");
    return true;
}

bool SeratoExporter::createCrateFile(const juce::File& crateFile,
                                    const DatabaseManager::VirtualFolder& folder,
                                    const std::vector<DatabaseManager::Track>& tracks)
{
    auto stream = crateFile.createOutputStream();
    
    if (stream == nullptr)
    {
        lastError = "Failed to create crate file: " + crateFile.getFullPathName();
        DBG("[SeratoExporter] " << lastError);
        return false;
    }
    
    // Write crate header
    stream->writeString("vrsn");
    writeSeratoInt32(*stream, 0x10); // Crate version 1.0
    
    // Write crate name
    stream->writeString("tvcn");
    writeSeratoString(*stream, folder.name);
    
    // Write track paths
    for (const auto& track : tracks)
    {
        stream->writeString("otrk");
        stream->writeString("ptrk");
        writeSeratoString(*stream, trackToSeratoPath(track));
    }
    
    stream->flush();
    DBG("[SeratoExporter] Created crate file: " << crateFile.getFileName() << " with " << tracks.size() << " tracks");
    return true;
}

juce::String SeratoExporter::trackToSeratoPath(const DatabaseManager::Track& track) const
{
    // Convert Windows path to Serato format
    // Serato uses forward slashes
    juce::String path = track.filePath;
    return path.replace("\\", "/");
}

void SeratoExporter::writeSeratoString(juce::OutputStream& stream, const juce::String& str)
{
    // Write string length as 4-byte big-endian integer
    const int32_t length = str.length();
    writeSeratoInt32(stream, length);
    
    // Write UTF-16 encoded string
    const juce::String utf16 = str;
    stream.write(utf16.toWideCharPointer(), static_cast<size_t>(length * 2));
}

void SeratoExporter::writeSeratoInt32(juce::OutputStream& stream, int32_t value)
{
    // Write as big-endian
    stream.writeByte(static_cast<char>((value >> 24) & 0xFF));
    stream.writeByte(static_cast<char>((value >> 16) & 0xFF));
    stream.writeByte(static_cast<char>((value >> 8) & 0xFF));
    stream.writeByte(static_cast<char>(value & 0xFF));
}
