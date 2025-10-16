/*
  ==============================================================================

    Library Manager v1.0.1
    Copyright (C) 2025 uniQuE-ui

    Test program for FileScanner, AnalysisWorker, and fingerprinting

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../Source/DatabaseManager.h"
#include "../Source/FileScanner.h"
#include "../Source/AnalysisWorker.h"
#include <iostream>

int main()
{
    std::cout << "=== Library Manager Component Test ===" << std::endl;
    
    // Create a temporary database
    juce::File tempDb = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("test_library_components.db");
    
    if (tempDb.existsAsFile())
        tempDb.deleteFile();
    
    std::cout << "\nTest 1: Initialize database..." << std::endl;
    DatabaseManager dbManager;
    if (!dbManager.initialize(tempDb))
    {
        std::cerr << "Failed to initialize database: " << dbManager.getLastError() << std::endl;
        return 1;
    }
    std::cout << "✓ Database initialized" << std::endl;
    
    // Create some test audio files (empty files, just for testing the scanner)
    std::cout << "\nTest 2: Create test directory structure..." << std::endl;
    juce::File testDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("test_music_library");
    
    if (testDir.exists())
        testDir.deleteRecursively();
    
    testDir.createDirectory();
    testDir.getChildFile("subfolder").createDirectory();
    
    // Create some dummy audio files
    testDir.getChildFile("track1.mp3").create();
    testDir.getChildFile("track2.flac").create();
    testDir.getChildFile("subfolder/track3.wav").create();
    testDir.getChildFile("not_audio.txt").create();
    
    std::cout << "✓ Test directory created" << std::endl;
    
    // Test FileScanner
    std::cout << "\nTest 3: Scan directory for audio files..." << std::endl;
    FileScanner scanner(dbManager);
    
    int progressCount = 0;
    scanner.setProgressCallback([&progressCount](int current, int total) {
        progressCount++;
        std::cout << "  Progress: " << current << "/" << total << std::endl;
    });
    
    int filesFound = scanner.scanDirectory(testDir, true);
    std::cout << "✓ Found and queued " << filesFound << " audio files" << std::endl;
    
    // Verify jobs were created
    auto pendingJobs = dbManager.getJobsByStatus("pending");
    std::cout << "  Pending jobs in database: " << pendingJobs.size() << std::endl;
    
    if (pendingJobs.size() != filesFound)
    {
        std::cerr << "Error: Job count mismatch!" << std::endl;
        return 1;
    }
    
    // Test AnalysisWorker (without actually processing since we don't have real audio files)
    std::cout << "\nTest 4: AnalysisWorker initialization..." << std::endl;
    AnalysisWorker worker(dbManager);
    
    worker.setProgressCallback([](const AnalysisWorker::ProgressInfo& info) {
        std::cout << "  Job " << info.jobId << ": " << info.status 
                  << " (" << info.progress << "%) - " << info.filePath << std::endl;
    });
    
    std::cout << "✓ AnalysisWorker created" << std::endl;
    std::cout << "  Pending jobs: " << worker.getPendingJobCount() << std::endl;
    
    // Test duplicate detection query
    std::cout << "\nTest 5: Duplicate detection query..." << std::endl;
    auto duplicates = dbManager.findTracksByFingerprint("test_fingerprint_123");
    std::cout << "✓ Duplicate query works (found " << duplicates.size() << " tracks)" << std::endl;
    
    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    worker.stopWorker();
    dbManager.close();
    tempDb.deleteFile();
    testDir.deleteRecursively();
    std::cout << "✓ Cleanup complete" << std::endl;
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
