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
#include "DatabaseManager.h"
#include <functional>

//==============================================================================
/**
    SeratoExporter exports library data to Serato DJ format.
    
    Supports:
    - Track metadata export
    - Crate (playlist) export
    - Cue point export
    - BPM and key information
*/
class SeratoExporter
{
public:
    SeratoExporter(DatabaseManager& dbManager);
    ~SeratoExporter();
    
    /**
     * Export entire library to Serato format.
     * @param outputDirectory Directory where Serato files will be created
     * @param progressCallback Optional callback for progress updates (0.0 to 1.0)
     * @return true if export was successful
     */
    bool exportLibrary(const juce::File& outputDirectory,
                      std::function<void(float)> progressCallback = nullptr);
    
    /**
     * Export specific playlist/crate to Serato format.
     * @param folderId The playlist/crate to export
     * @param outputDirectory Directory where crate file will be created
     * @return true if export was successful
     */
    bool exportPlaylist(int64_t folderId, const juce::File& outputDirectory);
    
    /**
     * Get the last error message.
     */
    juce::String getLastError() const { return lastError; }
    
private:
    DatabaseManager& databaseManager;
    juce::String lastError;
    
    // Helper methods
    bool createDatabaseFile(const juce::File& dbFile, const std::vector<DatabaseManager::Track>& tracks);
    bool createCrateFile(const juce::File& crateFile, const DatabaseManager::VirtualFolder& folder,
                        const std::vector<DatabaseManager::Track>& tracks);
    juce::String trackToSeratoPath(const DatabaseManager::Track& track) const;
    void writeSeratoString(juce::OutputStream& stream, const juce::String& str);
    void writeSeratoInt32(juce::OutputStream& stream, int32_t value);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeratoExporter)
};
