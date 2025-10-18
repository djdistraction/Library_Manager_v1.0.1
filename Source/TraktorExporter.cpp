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

#include "TraktorExporter.h"

//==============================================================================
TraktorExporter::TraktorExporter(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
}

TraktorExporter::~TraktorExporter()
{
}

//==============================================================================
bool TraktorExporter::exportLibrary(const juce::File& outputFile,
                                   std::function<void(float)> progressCallback)
{
    // Get all tracks
    auto tracks = databaseManager.getAllTracks();
    
    if (tracks.empty())
    {
        lastError = "No tracks to export";
        DBG("[TraktorExporter] " << lastError);
        return false;
    }
    
    // Create XML structure
    juce::XmlElement root("NML");
    root.setAttribute("VERSION", "19");
    
    // Add header
    writeNMLHeader(root);
    
    if (progressCallback)
        progressCallback(0.1f);
    
    // Add collection
    auto* collection = root.createNewChildElement("COLLECTION");
    collection->setAttribute("ENTRIES", static_cast<int>(tracks.size()));
    
    writeTrackCollection(*collection, tracks);
    
    if (progressCallback)
        progressCallback(0.6f);
    
    // Add playlists
    auto* playlists = root.createNewChildElement("PLAYLISTS");
    writePlaylists(*playlists);
    
    if (progressCallback)
        progressCallback(0.9f);
    
    // Write to file
    if (!root.writeTo(outputFile))
    {
        lastError = "Failed to write NML file: " + outputFile.getFullPathName();
        DBG("[TraktorExporter] " << lastError);
        return false;
    }
    
    if (progressCallback)
        progressCallback(1.0f);
    
    DBG("[TraktorExporter] Export completed successfully: " << outputFile.getFullPathName());
    return true;
}

bool TraktorExporter::exportPlaylist(int64_t folderId, const juce::File& outputFile)
{
    auto folder = databaseManager.getVirtualFolder(folderId);
    auto tracks = databaseManager.getTracksInFolder(folderId);
    
    if (tracks.empty())
    {
        lastError = "Playlist is empty";
        return false;
    }
    
    // Create XML structure
    juce::XmlElement root("NML");
    root.setAttribute("VERSION", "19");
    
    writeNMLHeader(root);
    
    // Add collection with only the tracks in this playlist
    auto* collection = root.createNewChildElement("COLLECTION");
    collection->setAttribute("ENTRIES", static_cast<int>(tracks.size()));
    writeTrackCollection(*collection, tracks);
    
    // Add single playlist
    auto* playlists = root.createNewChildElement("PLAYLISTS");
    auto* node = playlists->createNewChildElement("NODE");
    node->setAttribute("TYPE", "FOLDER");
    node->setAttribute("NAME", "$ROOT");
    
    writePlaylistNode(*node, folder);
    
    // Write to file
    if (!root.writeTo(outputFile))
    {
        lastError = "Failed to write NML file";
        return false;
    }
    
    DBG("[TraktorExporter] Playlist export completed: " << outputFile.getFullPathName());
    return true;
}

//==============================================================================
void TraktorExporter::writeNMLHeader(juce::XmlElement& root)
{
    auto* head = root.createNewChildElement("HEAD");
    head->setAttribute("COMPANY", "uniQuE-ui");
    head->setAttribute("PROGRAM", "Library Manager");
    head->setAttribute("VERSION", "1.0.1");
}

void TraktorExporter::writeTrackCollection(juce::XmlElement& collection,
                                          const std::vector<DatabaseManager::Track>& tracks)
{
    for (const auto& track : tracks)
    {
        writeTrackEntry(collection, track);
    }
}

void TraktorExporter::writeTrackEntry(juce::XmlElement& collection,
                                     const DatabaseManager::Track& track)
{
    auto* entry = collection.createNewChildElement("ENTRY");
    
    // Basic attributes
    entry->setAttribute("MODIFIED_DATE", track.lastModified.toString(true, true));
    entry->setAttribute("MODIFIED_TIME", static_cast<int>(track.lastModified.toMilliseconds() / 1000));
    entry->setAttribute("AUDIO_ID", juce::String(track.id));
    entry->setAttribute("TITLE", track.title);
    entry->setAttribute("ARTIST", track.artist);
    
    // Location
    auto* location = entry->createNewChildElement("LOCATION");
    location->setAttribute("DIR", trackToTraktorPath(track));
    location->setAttribute("FILE", juce::File(track.filePath).getFileName());
    location->setAttribute("VOLUME", "");
    location->setAttribute("VOLUMEID", "");
    
    // Album
    auto* album = entry->createNewChildElement("ALBUM");
    album->setAttribute("TITLE", track.album);
    
    // Musical keys
    if (track.key.isNotEmpty())
    {
        auto* musicalKey = entry->createNewChildElement("MUSICAL_KEY");
        musicalKey->setAttribute("VALUE", convertKeyToTraktorFormat(track.key));
    }
    
    // Info (BPM, duration, etc.)
    auto* info = entry->createNewChildElement("INFO");
    if (track.bpm > 0)
    {
        info->setAttribute("BITRATE", 0);
        info->setAttribute("GENRE", track.genre);
        info->setAttribute("PLAYTIME", static_cast<int>(track.duration));
        info->setAttribute("PLAYTIME_FLOAT", juce::String(track.duration, 3));
        
        // Tempo
        auto* tempo = entry->createNewChildElement("TEMPO");
        tempo->setAttribute("BPM", juce::String(track.bpm, 2));
        tempo->setAttribute("BPM_QUALITY", "100");
    }
    
    // Cue points
    auto cuePoints = databaseManager.getCuePointsForTrack(track.id);
    if (!cuePoints.empty())
    {
        writeCuePoints(*entry, cuePoints);
    }
}

void TraktorExporter::writeCuePoints(juce::XmlElement& entry,
                                    const std::vector<DatabaseManager::CuePoint>& cues)
{
    for (const auto& cue : cues)
    {
        auto* cueV2 = entry.createNewChildElement("CUE_V2");
        cueV2->setAttribute("NAME", cue.name);
        cueV2->setAttribute("TYPE", cue.type);
        cueV2->setAttribute("START", juce::String(cue.position, 3));
        
        // Traktor hotcue number (0-7)
        if (cue.hotCueNumber >= 0 && cue.hotCueNumber < 8)
        {
            cueV2->setAttribute("HOTCUE", cue.hotCueNumber);
        }
        
        // Color in RGB format
        if (cue.color.isNotEmpty())
        {
            cueV2->setAttribute("COLOR", cue.color);
        }
    }
}

void TraktorExporter::writePlaylists(juce::XmlElement& playlists)
{
    // Create root node
    auto* root = playlists.createNewChildElement("NODE");
    root->setAttribute("TYPE", "FOLDER");
    root->setAttribute("NAME", "$ROOT");
    
    // Get all folders
    auto folders = databaseManager.getAllVirtualFolders();
    
    for (const auto& folder : folders)
    {
        writePlaylistNode(*root, folder);
    }
}

void TraktorExporter::writePlaylistNode(juce::XmlElement& parent,
                                       const DatabaseManager::VirtualFolder& folder)
{
    auto* node = parent.createNewChildElement("NODE");
    node->setAttribute("TYPE", "PLAYLIST");
    node->setAttribute("NAME", folder.name);
    
    // Get tracks in this folder
    auto tracks = databaseManager.getTracksInFolder(folder.id);
    node->setAttribute("ENTRIES", static_cast<int>(tracks.size()));
    
    // Add playlist entries
    auto* playlist = node->createNewChildElement("PLAYLIST");
    for (const auto& track : tracks)
    {
        auto* entry = playlist->createNewChildElement("ENTRY");
        auto* primaryKey = entry->createNewChildElement("PRIMARYKEY");
        primaryKey->setAttribute("TYPE", "TRACK");
        primaryKey->setAttribute("KEY", juce::String(track.id));
    }
}

juce::String TraktorExporter::convertKeyToTraktorFormat(const juce::String& key) const
{
    // Convert OpenKey/Camelot notation to Traktor format if needed
    // For now, pass through as-is
    return key;
}

juce::String TraktorExporter::trackToTraktorPath(const DatabaseManager::Track& track) const
{
    // Traktor uses colon-separated path format on Windows
    // e.g., "C:/Music/Track.mp3" becomes "/:C:/Music/"
    juce::File file(track.filePath);
    juce::String path = file.getParentDirectory().getFullPathName();
    path = path.replace("\\", "/");
    
    if (path.length() > 2 && path[1] == ':')
    {
        // Windows drive letter
        return "/:" + path + "/";
    }
    
    return path + "/";
}
