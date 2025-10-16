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
#include <functional>
#include <atomic>

//==============================================================================
/**
    AnalysisWorker is a background thread that processes pending jobs from the
    database queue. It processes one job at a time and provides progress callbacks
    to keep the UI updated.
*/
class AnalysisWorker : public juce::Thread
{
public:
    //==============================================================================
    struct ProgressInfo
    {
        int64_t jobId;
        juce::String jobType;
        juce::String filePath;
        int progress;  // 0-100
        juce::String status;
        juce::String errorMessage;
    };
    
    //==============================================================================
    AnalysisWorker(DatabaseManager& dbManager);
    ~AnalysisWorker() override;
    
    /**
     * Set a progress callback to be notified when job status changes.
     * The callback will be called from the worker thread, so use MessageManager
     * if you need to update the UI.
     */
    void setProgressCallback(std::function<void(const ProgressInfo&)> callback);
    
    /**
     * Start the worker thread.
     */
    void startWorker();
    
    /**
     * Stop the worker thread gracefully.
     */
    void stopWorker();
    
    /**
     * Get the current number of pending jobs in the queue.
     */
    int getPendingJobCount() const;
    
    /**
     * Get the current job being processed (if any).
     */
    ProgressInfo getCurrentJob() const;
    
    /**
     * Check if the worker is currently processing a job.
     */
    bool isProcessing() const;

private:
    //==============================================================================
    void run() override;
    
    // Process a single job
    bool processJob(const DatabaseManager::Job& job);
    
    // Process an audio analysis job
    bool processAudioAnalysis(const DatabaseManager::Job& job);
    
    // Extract basic metadata from an audio file
    bool extractBasicMetadata(const juce::File& audioFile, DatabaseManager::Track& track);
    
    // Notify progress callback
    void notifyProgress(const ProgressInfo& info);
    
    //==============================================================================
    DatabaseManager& databaseManager;
    std::function<void(const ProgressInfo&)> progressCallback;
    std::atomic<bool> isCurrentlyProcessing{false};
    ProgressInfo currentJobInfo;
    mutable juce::CriticalSection callbackLock;
    mutable juce::CriticalSection jobInfoLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalysisWorker)
};
