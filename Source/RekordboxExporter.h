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

#pragma once

#include <juce_core/juce_core.h>
#include "DatabaseManager.h"

//==============================================================================
/**
    RekordboxExporter generates Rekordbox-compatible XML files from the indexed library.
    
    Supports:
    - Track metadata export
    - Playlist export
    - Basic cue point support
*/
class RekordboxExporter
{
public:
    RekordboxExporter(DatabaseManager& dbManager);
    ~RekordboxExporter();
    
    /**
     * Export the entire library to a Rekordbox XML file.
     * @param outputFile The destination XML file
     * @return true if export was successful
     */
    bool exportToXML(const juce::File& outputFile);
    
    /**
     * Export specific playlists to a Rekordbox XML file.
     * @param outputFile The destination XML file
     * @param playlistIds Vector of playlist IDs to export
     * @return true if export was successful
     */
    bool exportPlaylistsToXML(const juce::File& outputFile, const std::vector<int64_t>& playlistIds);
    
    /**
     * Get the last error message.
     */
    juce::String getLastError() const { return lastError; }
    
    /**
     * Set progress callback for monitoring export progress.
     */
    void setProgressCallback(std::function<void(double progress, const juce::String& status)> callback);

private:
    DatabaseManager& databaseManager;
    juce::String lastError;
    std::function<void(double, const juce::String&)> progressCallback;
    
    // XML generation methods
    juce::XmlElement createRootElement();
    juce::XmlElement* createProductElement();
    juce::XmlElement* createCollectionElement(const std::vector<DatabaseManager::Track>& tracks);
    juce::XmlElement* createPlaylistsElement(const std::vector<DatabaseManager::VirtualFolder>& playlists);
    juce::XmlElement* createTrackElement(const DatabaseManager::Track& track, int trackId);
    juce::XmlElement* createPlaylistElement(const DatabaseManager::VirtualFolder& playlist, int playlistId);
    
    // Helper methods
    juce::String convertKeyToRekordbox(const juce::String& key);
    juce::String generateTrackLocation(const juce::String& filePath);
    void reportProgress(double progress, const juce::String& status);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RekordboxExporter)
};
