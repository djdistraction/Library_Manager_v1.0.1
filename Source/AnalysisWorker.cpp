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

#include "AnalysisWorker.h"
#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
AnalysisWorker::AnalysisWorker(DatabaseManager& dbManager)
    : Thread("AnalysisWorker"),
      databaseManager(dbManager)
{
}

AnalysisWorker::~AnalysisWorker()
{
    stopWorker();
}

//==============================================================================
void AnalysisWorker::startWorker()
{
    if (isThreadRunning())
        return;
    
    DBG("[AnalysisWorker] Starting worker thread");
    startThread(); // Start with default priority
}

void AnalysisWorker::stopWorker()
{
    if (!isThreadRunning())
        return;
    
    DBG("[AnalysisWorker] Stopping worker thread");
    signalThreadShouldExit();
    notify();
    waitForThreadToExit(5000);
}

//==============================================================================
void AnalysisWorker::run()
{
    DBG("[AnalysisWorker] Worker thread started");
    
    while (!threadShouldExit())
    {
        // Get pending jobs
        auto pendingJobs = databaseManager.getJobsByStatus("pending");
        
        if (pendingJobs.empty())
        {
            // No jobs to process, wait for notification or timeout
            wait(1000); // Check every second
            continue;
        }
        
        // Process the first pending job
        auto job = pendingJobs[0];
        
        DBG("[AnalysisWorker] Processing job " << job.id << " (" << job.jobType << ")");
        
        // Update job status to running
        job.status = "running";
        job.dateStarted = juce::Time::getCurrentTime();
        job.progress = 0;
        databaseManager.updateJob(job);
        
        // Process the job
        bool success = processJob(job);
        
        // Update job status to completed or failed
        job.status = success ? "completed" : "failed";
        job.dateCompleted = juce::Time::getCurrentTime();
        job.progress = success ? 100 : job.progress;
        databaseManager.updateJob(job);
        
        if (success)
        {
            DBG("[AnalysisWorker] Job " << job.id << " completed successfully");
        }
        else
        {
            DBG("[AnalysisWorker] Job " << job.id << " failed: " << job.errorMessage);
        }
        
        isCurrentlyProcessing = false;
    }
    
    DBG("[AnalysisWorker] Worker thread stopped");
}

//==============================================================================
bool AnalysisWorker::processJob(const DatabaseManager::Job& job)
{
    if (threadShouldExit())
        return false;
    
    isCurrentlyProcessing = true;
    
    // Parse job parameters
    auto params = juce::JSON::parse(job.parameters);
    if (params.isVoid())
    {
        DBG("[AnalysisWorker] Error: Failed to parse job parameters");
        return false;
    }
    
    auto* paramsObj = params.getDynamicObject();
    if (paramsObj == nullptr)
    {
        DBG("[AnalysisWorker] Error: Job parameters is not an object");
        return false;
    }
    
    // Update current job info
    {
        const juce::ScopedLock lock(jobInfoLock);
        currentJobInfo.jobId = job.id;
        currentJobInfo.jobType = job.jobType;
        currentJobInfo.filePath = paramsObj->getProperty("file_path").toString();
        currentJobInfo.progress = 0;
        currentJobInfo.status = "running";
        currentJobInfo.errorMessage = "";
    }
    
    notifyProgress(currentJobInfo);
    
    // Route to appropriate handler based on job type
    if (job.jobType == "analyze_audio")
    {
        return processAudioAnalysis(job);
    }
    else
    {
        DBG("[AnalysisWorker] Error: Unknown job type: " << job.jobType);
        return false;
    }
}

bool AnalysisWorker::processAudioAnalysis(const DatabaseManager::Job& job)
{
    auto params = juce::JSON::parse(job.parameters);
    auto* paramsObj = params.getDynamicObject();
    
    juce::String filePath = paramsObj->getProperty("file_path").toString();
    juce::File audioFile(filePath);
    
    if (!audioFile.existsAsFile())
    {
        DBG("[AnalysisWorker] Error: File not found: " << filePath);
        auto updatedJob = job;
        updatedJob.errorMessage = "File not found";
        databaseManager.updateJob(updatedJob);
        return false;
    }
    
    DBG("[AnalysisWorker] Analyzing: " << audioFile.getFileName());
    
    // Create track record
    DatabaseManager::Track track;
    track.filePath = audioFile.getFullPathName();
    track.fileSize = audioFile.getSize();
    track.dateAdded = juce::Time::getCurrentTime();
    track.lastModified = juce::Time(audioFile.getLastModificationTime());
    
    // Extract basic metadata
    if (!extractBasicMetadata(audioFile, track))
    {
        DBG("[AnalysisWorker] Warning: Failed to extract metadata, using defaults");
    }
    
    // Update progress
    {
        const juce::ScopedLock lock(jobInfoLock);
        currentJobInfo.progress = 50;
    }
    notifyProgress(currentJobInfo);
    
    // Check if track already exists
    auto existingTracks = databaseManager.searchTracks(track.filePath);
    bool trackExists = false;
    for (const auto& existing : existingTracks)
    {
        if (existing.filePath == track.filePath)
        {
            // Update existing track
            track.id = existing.id;
            trackExists = true;
            break;
        }
    }
    
    // Add or update track in database
    int64_t trackId = 0;
    bool dbSuccess = false;
    
    if (trackExists)
    {
        dbSuccess = databaseManager.updateTrack(track);
    }
    else
    {
        dbSuccess = databaseManager.addTrack(track, trackId);
    }
    
    if (!dbSuccess)
    {
        DBG("[AnalysisWorker] Error: Failed to save track to database");
        auto updatedJob = job;
        updatedJob.errorMessage = "Failed to save to database";
        databaseManager.updateJob(updatedJob);
        return false;
    }
    
    // Update progress to complete
    {
        const juce::ScopedLock lock(jobInfoLock);
        currentJobInfo.progress = 100;
        currentJobInfo.status = "completed";
    }
    notifyProgress(currentJobInfo);
    
    return true;
}

bool AnalysisWorker::extractBasicMetadata(const juce::File& audioFile, DatabaseManager::Track& track)
{
    // Create audio format manager and register formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // Try to read the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    
    if (reader == nullptr)
    {
        DBG("[AnalysisWorker] Warning: Could not create audio reader for: " << audioFile.getFileName());
        // Set defaults
        track.title = audioFile.getFileNameWithoutExtension();
        track.duration = 0.0;
        return false;
    }
    
    // Calculate duration
    track.duration = reader->lengthInSamples / reader->sampleRate;
    
    // Extract metadata from the reader
    auto metadata = reader->metadataValues;
    
    // Extract title
    if (metadata.containsKey("title"))
        track.title = metadata["title"];
    else if (metadata.containsKey("TITLE"))
        track.title = metadata["TITLE"];
    else
        track.title = audioFile.getFileNameWithoutExtension();
    
    // Extract artist
    if (metadata.containsKey("artist"))
        track.artist = metadata["artist"];
    else if (metadata.containsKey("ARTIST"))
        track.artist = metadata["ARTIST"];
    
    // Extract album
    if (metadata.containsKey("album"))
        track.album = metadata["album"];
    else if (metadata.containsKey("ALBUM"))
        track.album = metadata["ALBUM"];
    
    // Extract genre
    if (metadata.containsKey("genre"))
        track.genre = metadata["genre"];
    else if (metadata.containsKey("GENRE"))
        track.genre = metadata["GENRE"];
    
    DBG("[AnalysisWorker] Extracted metadata - Title: " << track.title 
        << ", Artist: " << track.artist 
        << ", Duration: " << track.duration << "s");
    
    return true;
}

//==============================================================================
void AnalysisWorker::setProgressCallback(std::function<void(const ProgressInfo&)> callback)
{
    const juce::ScopedLock lock(callbackLock);
    progressCallback = callback;
}

void AnalysisWorker::notifyProgress(const ProgressInfo& info)
{
    const juce::ScopedLock lock(callbackLock);
    if (progressCallback)
    {
        progressCallback(info);
    }
}

int AnalysisWorker::getPendingJobCount() const
{
    auto pendingJobs = databaseManager.getJobsByStatus("pending");
    return static_cast<int>(pendingJobs.size());
}

AnalysisWorker::ProgressInfo AnalysisWorker::getCurrentJob() const
{
    const juce::ScopedLock lock(jobInfoLock);
    return currentJobInfo;
}

bool AnalysisWorker::isProcessing() const
{
    return isCurrentlyProcessing;
}
