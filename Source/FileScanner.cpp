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

#include "FileScanner.h"

//==============================================================================
FileScanner::FileScanner(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
}

FileScanner::~FileScanner()
{
}

//==============================================================================
const juce::StringArray& FileScanner::getSupportedExtensions()
{
    static const juce::StringArray extensions {
        ".mp3", ".wav", ".flac", ".aac", ".ogg", ".m4a", 
        ".aiff", ".aif", ".wma", ".opus", ".alac"
    };
    return extensions;
}

bool FileScanner::isSupportedAudioFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;
    
    auto extension = file.getFileExtension().toLowerCase();
    return getSupportedExtensions().contains(extension);
}

//==============================================================================
int FileScanner::scanDirectory(const juce::File& directory, bool recursive)
{
    if (!directory.isDirectory())
    {
        DBG("[FileScanner] Error: Not a valid directory: " << directory.getFullPathName());
        return 0;
    }
    
    DBG("[FileScanner] Starting scan of: " << directory.getFullPathName());
    shouldCancel = false;
    
    std::vector<juce::File> foundFiles;
    scanDirectoryInternal(directory, recursive, foundFiles);
    
    if (shouldCancel)
    {
        DBG("[FileScanner] Scan cancelled");
        return 0;
    }
    
    DBG("[FileScanner] Found " << foundFiles.size() << " audio files");
    
    // Create jobs in a transaction for better performance
    int jobsCreated = 0;
    
    if (!databaseManager.beginTransaction())
    {
        DBG("[FileScanner] Error: Failed to begin transaction");
        return 0;
    }
    
    for (size_t i = 0; i < foundFiles.size(); ++i)
    {
        if (shouldCancel)
            break;
            
        if (createJobForFile(foundFiles[i]))
        {
            jobsCreated++;
        }
        
        if (progressCallback)
        {
            progressCallback(static_cast<int>(i + 1), static_cast<int>(foundFiles.size()));
        }
    }
    
    if (shouldCancel)
    {
        databaseManager.rollbackTransaction();
        DBG("[FileScanner] Scan cancelled, rolled back transaction");
        return 0;
    }
    
    if (!databaseManager.commitTransaction())
    {
        DBG("[FileScanner] Error: Failed to commit transaction");
        return 0;
    }
    
    DBG("[FileScanner] Created " << jobsCreated << " pending jobs");
    return jobsCreated;
}

//==============================================================================
void FileScanner::scanDirectoryInternal(const juce::File& directory, bool recursive,
                                       std::vector<juce::File>& foundFiles)
{
    if (shouldCancel)
        return;
    
    // Get all files in this directory
    juce::Array<juce::File> filesInDir;
    directory.findChildFiles(filesInDir, 
                            juce::File::findFiles, 
                            false, // don't search subdirectories here
                            "*");
    
    // Check each file
    for (const auto& file : filesInDir)
    {
        if (shouldCancel)
            return;
            
        if (isSupportedAudioFile(file))
        {
            foundFiles.push_back(file);
        }
    }
    
    // Recursively scan subdirectories if requested
    if (recursive)
    {
        juce::Array<juce::File> subdirs;
        directory.findChildFiles(subdirs, 
                                juce::File::findDirectories, 
                                false, 
                                "*");
        
        for (const auto& subdir : subdirs)
        {
            if (shouldCancel)
                return;
                
            scanDirectoryInternal(subdir, recursive, foundFiles);
        }
    }
}

bool FileScanner::createJobForFile(const juce::File& audioFile)
{
    DatabaseManager::Job job;
    job.jobType = "analyze_audio";
    job.status = "pending";
    
    // Create JSON parameters
    juce::var paramsObj = new juce::DynamicObject();
    paramsObj.getDynamicObject()->setProperty("file_path", audioFile.getFullPathName());
    paramsObj.getDynamicObject()->setProperty("file_size", static_cast<juce::int64>(audioFile.getSize()));
    paramsObj.getDynamicObject()->setProperty("date_added", juce::Time::getCurrentTime().toISO8601(true));
    
    job.parameters = juce::JSON::toString(paramsObj);
    job.dateCreated = juce::Time::getCurrentTime();
    job.progress = 0;
    
    int64_t jobId = 0;
    if (!databaseManager.addJob(job, jobId))
    {
        DBG("[FileScanner] Error: Failed to create job for: " << audioFile.getFullPathName());
        return false;
    }
    
    return true;
}

//==============================================================================
void FileScanner::setProgressCallback(std::function<void(int, int)> callback)
{
    progressCallback = callback;
}

void FileScanner::cancelScan()
{
    shouldCancel = true;
}
