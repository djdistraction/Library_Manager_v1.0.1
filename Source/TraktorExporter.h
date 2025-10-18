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
#include "DatabaseManager.h"
#include <functional>

//==============================================================================
/**
    TraktorExporter exports library data to Traktor Pro NML format.
    
    Supports:
    - Track collection export
    - Playlist export
    - Cue point export (including colors)
    - BPM and key information
*/
class TraktorExporter
{
public:
    TraktorExporter(DatabaseManager& dbManager);
    ~TraktorExporter();
    
    /**
     * Export entire library to Traktor NML format.
     * @param outputFile Output NML file path
     * @param progressCallback Optional callback for progress updates (0.0 to 1.0)
     * @return true if export was successful
     */
    bool exportLibrary(const juce::File& outputFile,
                      std::function<void(float)> progressCallback = nullptr);
    
    /**
     * Export specific playlist to Traktor NML format.
     * @param folderId The playlist to export
     * @param outputFile Output NML file path
     * @return true if export was successful
     */
    bool exportPlaylist(int64_t folderId, const juce::File& outputFile);
    
    /**
     * Get the last error message.
     */
    juce::String getLastError() const { return lastError; }
    
private:
    DatabaseManager& databaseManager;
    juce::String lastError;
    
    // Helper methods
    void writeNMLHeader(juce::XmlElement& root);
    void writeTrackCollection(juce::XmlElement& collection,
                             const std::vector<DatabaseManager::Track>& tracks);
    void writeTrackEntry(juce::XmlElement& collection, const DatabaseManager::Track& track);
    void writeCuePoints(juce::XmlElement& entry, const std::vector<DatabaseManager::CuePoint>& cues);
    void writePlaylists(juce::XmlElement& playlists);
    void writePlaylistNode(juce::XmlElement& parent, const DatabaseManager::VirtualFolder& folder);
    
    juce::String convertKeyToTraktorFormat(const juce::String& key) const;
    juce::String trackToTraktorPath(const DatabaseManager::Track& track) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TraktorExporter)
};
