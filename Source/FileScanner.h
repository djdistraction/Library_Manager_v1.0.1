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
#include <vector>
#include <functional>

//==============================================================================
/**
    FileScanner recursively scans directories for supported audio file types
    and creates pending job entries in the database for each found file.
*/
class FileScanner
{
public:
    //==============================================================================
    FileScanner(DatabaseManager& dbManager);
    ~FileScanner();
    
    //==============================================================================
    // Supported audio file extensions
    static const juce::StringArray& getSupportedExtensions();
    
    /**
     * Recursively scan a directory for audio files.
     * @param directory The directory to scan
     * @param recursive If true, scan subdirectories recursively
     * @return Number of files found and added to the job queue
     */
    int scanDirectory(const juce::File& directory, bool recursive = true);
    
    /**
     * Check if a file is a supported audio file.
     * @param file The file to check
     * @return True if the file is a supported audio type
     */
    static bool isSupportedAudioFile(const juce::File& file);
    
    /**
     * Set a progress callback to be notified during scanning.
     * The callback receives: filesScanned, totalFiles (estimated)
     */
    void setProgressCallback(std::function<void(int, int)> callback);
    
    /**
     * Cancel the current scan operation.
     */
    void cancelScan();
    
private:
    //==============================================================================
    DatabaseManager& databaseManager;
    std::function<void(int, int)> progressCallback;
    std::atomic<bool> shouldCancel{false};
    
    // Helper method to recursively scan
    void scanDirectoryInternal(const juce::File& directory, bool recursive, 
                              std::vector<juce::File>& foundFiles);
    
    // Create a pending job for a file
    bool createJobForFile(const juce::File& audioFile);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileScanner)
};
