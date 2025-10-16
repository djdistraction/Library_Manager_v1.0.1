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

#include <JuceHeader.h>
#include "../Source/DatabaseManager.h"
#include <iostream>
#include <cassert>

// Simple test program to verify DatabaseManager functionality
int main()
{
    std::cout << "=== DatabaseManager Test Program ===" << std::endl;
    
    // Create a temporary database file
    juce::File tempDb = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("test_library.db");
    
    if (tempDb.existsAsFile())
        tempDb.deleteFile();
    
    std::cout << "Using test database: " << tempDb.getFullPathName() << std::endl;
    
    // Test 1: Initialize database
    std::cout << "\nTest 1: Initialize database..." << std::endl;
    DatabaseManager dbManager;
    assert(dbManager.initialize(tempDb));
    assert(dbManager.isOpen());
    std::cout << "✓ Database initialized successfully" << std::endl;
    
    // Test 2: Add a track
    std::cout << "\nTest 2: Add a track..." << std::endl;
    DatabaseManager::Track track;
    track.filePath = "/home/user/music/test_track.mp3";
    track.title = "Test Track";
    track.artist = "Test Artist";
    track.album = "Test Album";
    track.genre = "Electronic";
    track.bpm = 128;
    track.key = "Am";
    track.duration = 240.5;
    track.fileSize = 5242880;
    track.fileHash = "abc123def456";
    track.dateAdded = juce::Time::getCurrentTime();
    track.lastModified = juce::Time::getCurrentTime();
    
    int64_t trackId = 0;
    assert(dbManager.addTrack(track, trackId));
    assert(trackId > 0);
    std::cout << "✓ Track added with ID: " << trackId << std::endl;
    
    // Test 3: Retrieve the track
    std::cout << "\nTest 3: Retrieve track..." << std::endl;
    auto retrievedTrack = dbManager.getTrack(trackId);
    assert(retrievedTrack.id == trackId);
    assert(retrievedTrack.title == "Test Track");
    assert(retrievedTrack.artist == "Test Artist");
    assert(retrievedTrack.bpm == 128);
    std::cout << "✓ Track retrieved successfully" << std::endl;
    std::cout << "  Title: " << retrievedTrack.title << std::endl;
    std::cout << "  Artist: " << retrievedTrack.artist << std::endl;
    std::cout << "  BPM: " << retrievedTrack.bpm << std::endl;
    
    // Test 4: Update the track
    std::cout << "\nTest 4: Update track..." << std::endl;
    retrievedTrack.bpm = 140;
    retrievedTrack.artist = "Updated Artist";
    assert(dbManager.updateTrack(retrievedTrack));
    auto updatedTrack = dbManager.getTrack(trackId);
    assert(updatedTrack.bpm == 140);
    assert(updatedTrack.artist == "Updated Artist");
    std::cout << "✓ Track updated successfully" << std::endl;
    
    // Test 5: Add a virtual folder
    std::cout << "\nTest 5: Add a virtual folder..." << std::endl;
    DatabaseManager::VirtualFolder folder;
    folder.name = "My Favorites";
    folder.description = "Collection of favorite tracks";
    folder.dateCreated = juce::Time::getCurrentTime();
    
    int64_t folderId = 0;
    assert(dbManager.addVirtualFolder(folder, folderId));
    assert(folderId > 0);
    std::cout << "✓ Virtual folder added with ID: " << folderId << std::endl;
    
    // Test 6: Add track to folder
    std::cout << "\nTest 6: Add track to folder..." << std::endl;
    DatabaseManager::FolderTrackLink link;
    link.folderId = folderId;
    link.trackId = trackId;
    link.displayOrder = 1;
    link.dateAdded = juce::Time::getCurrentTime();
    
    int64_t linkId = 0;
    assert(dbManager.addFolderTrackLink(link, linkId));
    assert(linkId > 0);
    std::cout << "✓ Track added to folder with link ID: " << linkId << std::endl;
    
    // Test 7: Get tracks in folder
    std::cout << "\nTest 7: Get tracks in folder..." << std::endl;
    auto tracksInFolder = dbManager.getTracksInFolder(folderId);
    assert(tracksInFolder.size() == 1);
    assert(tracksInFolder[0].id == trackId);
    std::cout << "✓ Retrieved " << tracksInFolder.size() << " track(s) from folder" << std::endl;
    
    // Test 8: Add a job
    std::cout << "\nTest 8: Add a job..." << std::endl;
    DatabaseManager::Job job;
    job.jobType = "scan_library";
    job.status = "pending";
    job.parameters = "{\"path\": \"/home/user/music\"}";
    job.dateCreated = juce::Time::getCurrentTime();
    job.progress = 0;
    
    int64_t jobId = 0;
    assert(dbManager.addJob(job, jobId));
    assert(jobId > 0);
    std::cout << "✓ Job added with ID: " << jobId << std::endl;
    
    // Test 9: Update job
    std::cout << "\nTest 9: Update job..." << std::endl;
    auto retrievedJob = dbManager.getJob(jobId);
    retrievedJob.status = "running";
    retrievedJob.progress = 50;
    retrievedJob.dateStarted = juce::Time::getCurrentTime();
    assert(dbManager.updateJob(retrievedJob));
    auto updatedJob = dbManager.getJob(jobId);
    assert(updatedJob.status == "running");
    assert(updatedJob.progress == 50);
    std::cout << "✓ Job updated successfully" << std::endl;
    
    // Test 10: Search tracks
    std::cout << "\nTest 10: Search tracks..." << std::endl;
    auto searchResults = dbManager.searchTracks("Updated");
    assert(searchResults.size() == 1);
    assert(searchResults[0].artist == "Updated Artist");
    std::cout << "✓ Found " << searchResults.size() << " track(s) matching search" << std::endl;
    
    // Test 11: Transaction test
    std::cout << "\nTest 11: Transaction test..." << std::endl;
    assert(dbManager.beginTransaction());
    
    DatabaseManager::Track track2;
    track2.filePath = "/home/user/music/test_track2.mp3";
    track2.title = "Test Track 2";
    track2.artist = "Test Artist 2";
    track2.dateAdded = juce::Time::getCurrentTime();
    track2.lastModified = juce::Time::getCurrentTime();
    
    int64_t trackId2 = 0;
    assert(dbManager.addTrack(track2, trackId2));
    assert(dbManager.commitTransaction());
    
    auto allTracks = dbManager.getAllTracks();
    assert(allTracks.size() == 2);
    std::cout << "✓ Transaction committed successfully, total tracks: " << allTracks.size() << std::endl;
    
    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    dbManager.close();
    assert(!dbManager.isOpen());
    tempDb.deleteFile();
    std::cout << "✓ Database closed and test file deleted" << std::endl;
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
