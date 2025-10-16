/*
  ==============================================================================

    uniQuE-ui Library Manager - Rekordbox Export Test
    Copyright (C) 2025 uniQuE-ui

    This test validates the Rekordbox XML export functionality.

  ==============================================================================
*/

#include "../Source/RekordboxExporter.h"
#include "../Source/DatabaseManager.h"
#include <iostream>

void printTestResult(const juce::String& testName, bool passed)
{
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName.toStdString() << std::endl;
}

bool testBasicExport()
{
    // Create test database
    auto dbFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("test_export_" + juce::String(juce::Time::getCurrentTime().toMilliseconds()) + ".db");
    
    DatabaseManager db;
    if (!db.initialize(dbFile))
    {
        std::cerr << "Failed to initialize database: " << db.getLastError().toStdString() << std::endl;
        return false;
    }
    
    // Add test track
    DatabaseManager::Track track;
    track.filePath = "/path/to/music/test_song.mp3";
    track.title = "Test Song";
    track.artist = "Test Artist";
    track.album = "Test Album";
    track.genre = "Electronic";
    track.bpm = 128;
    track.key = "Am";
    track.duration = 240.0;
    track.fileSize = 10485760;
    track.fileHash = "abc123";
    track.dateAdded = juce::Time::getCurrentTime();
    track.lastModified = juce::Time::getCurrentTime();
    
    int64_t trackId;
    if (!db.addTrack(track, trackId))
    {
        std::cerr << "Failed to add track" << std::endl;
        dbFile.deleteFile();
        return false;
    }
    
    // Add test playlist
    DatabaseManager::VirtualFolder playlist;
    playlist.name = "Test Playlist";
    playlist.description = "Test playlist for export";
    playlist.dateCreated = juce::Time::getCurrentTime();
    
    int64_t playlistId;
    if (!db.addVirtualFolder(playlist, playlistId))
    {
        std::cerr << "Failed to add playlist" << std::endl;
        dbFile.deleteFile();
        return false;
    }
    
    // Add track to playlist
    DatabaseManager::FolderTrackLink link;
    link.folderId = playlistId;
    link.trackId = trackId;
    link.displayOrder = 0;
    link.dateAdded = juce::Time::getCurrentTime();
    
    int64_t linkId;
    if (!db.addFolderTrackLink(link, linkId))
    {
        std::cerr << "Failed to link track to playlist" << std::endl;
        dbFile.deleteFile();
        return false;
    }
    
    // Export to XML
    auto xmlFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile("test_export.xml");
    
    RekordboxExporter exporter(db);
    bool exportSuccess = exporter.exportToXML(xmlFile);
    
    if (!exportSuccess)
    {
        std::cerr << "Export failed: " << exporter.getLastError().toStdString() << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Verify XML file exists and has content
    if (!xmlFile.existsAsFile())
    {
        std::cerr << "XML file was not created" << std::endl;
        dbFile.deleteFile();
        return false;
    }
    
    // Parse and validate XML
    auto xml = juce::parseXML(xmlFile);
    if (xml == nullptr)
    {
        std::cerr << "Failed to parse XML" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Validate root element
    if (xml->getTagName() != "DJ_PLAYLISTS")
    {
        std::cerr << "Invalid root element: " << xml->getTagName().toStdString() << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Validate PRODUCT element
    auto* product = xml->getChildByName("PRODUCT");
    if (product == nullptr || product->getStringAttribute("Name") != "Library Manager")
    {
        std::cerr << "Invalid PRODUCT element" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Validate COLLECTION element
    auto* collection = xml->getChildByName("COLLECTION");
    if (collection == nullptr || collection->getIntAttribute("Entries") != 1)
    {
        std::cerr << "Invalid COLLECTION element" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Validate TRACK element
    auto* trackElement = collection->getChildByName("TRACK");
    if (trackElement == nullptr)
    {
        std::cerr << "No TRACK element found" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    if (trackElement->getStringAttribute("Name") != "Test Song" ||
        trackElement->getStringAttribute("Artist") != "Test Artist" ||
        trackElement->getIntAttribute("AverageBpm") != 128)
    {
        std::cerr << "Invalid track metadata in XML" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Validate PLAYLISTS element
    auto* playlists = xml->getChildByName("PLAYLISTS");
    if (playlists == nullptr)
    {
        std::cerr << "No PLAYLISTS element found" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    auto* rootNode = playlists->getChildByName("NODE");
    if (rootNode == nullptr || rootNode->getIntAttribute("Count") != 1)
    {
        std::cerr << "Invalid root NODE element" << std::endl;
        dbFile.deleteFile();
        xmlFile.deleteFile();
        return false;
    }
    
    // Clean up
    dbFile.deleteFile();
    xmlFile.deleteFile();
    
    std::cout << "✓ XML structure validated successfully" << std::endl;
    std::cout << "✓ Track metadata exported correctly" << std::endl;
    std::cout << "✓ Playlist structure exported correctly" << std::endl;
    
    return true;
}

int main()
{
    std::cout << "=== Rekordbox Export Test Suite ===" << std::endl << std::endl;
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    bool allPassed = true;
    
    allPassed &= testBasicExport();
    printTestResult("Basic Export", allPassed);
    
    std::cout << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Overall: " << (allPassed ? "PASSED" : "FAILED") << std::endl;
    
    return allPassed ? 0 : 1;
}
