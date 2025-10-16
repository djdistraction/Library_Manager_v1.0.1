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

#include "RekordboxExporter.h"

//==============================================================================
RekordboxExporter::RekordboxExporter(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
}

RekordboxExporter::~RekordboxExporter()
{
}

bool RekordboxExporter::exportToXML(const juce::File& outputFile)
{
    if (!databaseManager.isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    reportProgress(0.0, "Starting export...");
    
    // Create root XML element
    auto root = createRootElement();
    
    // Add product information
    root.addChildElement(createProductElement());
    
    reportProgress(0.1, "Exporting tracks...");
    
    // Add collection (tracks)
    auto tracks = databaseManager.getAllTracks();
    root.addChildElement(createCollectionElement(tracks));
    
    reportProgress(0.6, "Exporting playlists...");
    
    // Add playlists
    auto playlists = databaseManager.getAllVirtualFolders();
    root.addChildElement(createPlaylistsElement(playlists));
    
    reportProgress(0.9, "Writing XML file...");
    
    // Write to file
    if (!root.writeTo(outputFile, juce::XmlElement::TextFormat().singleLine()))
    {
        lastError = "Failed to write XML file";
        return false;
    }
    
    reportProgress(1.0, "Export complete!");
    
    DBG("[RekordboxExporter] Successfully exported to: " << outputFile.getFullPathName());
    return true;
}

bool RekordboxExporter::exportPlaylistsToXML(const juce::File& outputFile, 
                                              const std::vector<int64_t>& playlistIds)
{
    if (!databaseManager.isOpen())
    {
        lastError = "Database is not open";
        return false;
    }
    
    reportProgress(0.0, "Starting playlist export...");
    
    // Create root XML element
    auto root = createRootElement();
    
    // Add product information
    root.addChildElement(createProductElement());
    
    reportProgress(0.2, "Collecting tracks from playlists...");
    
    // Collect all tracks from selected playlists
    std::vector<DatabaseManager::Track> allTracks;
    std::set<int64_t> trackIds;
    
    for (auto playlistId : playlistIds)
    {
        auto playlistTracks = databaseManager.getTracksInFolder(playlistId);
        for (const auto& track : playlistTracks)
        {
            if (trackIds.find(track.id) == trackIds.end())
            {
                allTracks.push_back(track);
                trackIds.insert(track.id);
            }
        }
    }
    
    reportProgress(0.5, "Exporting tracks...");
    
    // Add collection
    root.addChildElement(createCollectionElement(allTracks));
    
    reportProgress(0.7, "Exporting playlists...");
    
    // Add selected playlists
    std::vector<DatabaseManager::VirtualFolder> selectedPlaylists;
    for (auto playlistId : playlistIds)
    {
        auto playlist = databaseManager.getVirtualFolder(playlistId);
        if (playlist.id != 0)
            selectedPlaylists.push_back(playlist);
    }
    
    root.addChildElement(createPlaylistsElement(selectedPlaylists));
    
    reportProgress(0.9, "Writing XML file...");
    
    // Write to file
    if (!root.writeTo(outputFile, juce::XmlElement::TextFormat().singleLine()))
    {
        lastError = "Failed to write XML file";
        return false;
    }
    
    reportProgress(1.0, "Export complete!");
    
    DBG("[RekordboxExporter] Successfully exported playlists to: " << outputFile.getFullPathName());
    return true;
}

void RekordboxExporter::setProgressCallback(std::function<void(double, const juce::String&)> callback)
{
    progressCallback = callback;
}

//==============================================================================
// XML Generation Methods

juce::XmlElement RekordboxExporter::createRootElement()
{
    juce::XmlElement root("DJ_PLAYLISTS");
    root.setAttribute("Version", "1.0.0");
    return root;
}

juce::XmlElement* RekordboxExporter::createProductElement()
{
    auto* product = new juce::XmlElement("PRODUCT");
    product->setAttribute("Name", "Library Manager");
    product->setAttribute("Version", "1.0.1");
    product->setAttribute("Company", "uniQuE-ui");
    return product;
}

juce::XmlElement* RekordboxExporter::createCollectionElement(const std::vector<DatabaseManager::Track>& tracks)
{
    auto* collection = new juce::XmlElement("COLLECTION");
    collection->setAttribute("Entries", static_cast<int>(tracks.size()));
    
    int trackId = 0;
    for (const auto& track : tracks)
    {
        collection->addChildElement(createTrackElement(track, trackId++));
    }
    
    return collection;
}

juce::XmlElement* RekordboxExporter::createPlaylistsElement(const std::vector<DatabaseManager::VirtualFolder>& playlists)
{
    auto* playlists_element = new juce::XmlElement("PLAYLISTS");
    
    // Create root node for all playlists
    auto* rootNode = new juce::XmlElement("NODE");
    rootNode->setAttribute("Type", "0");
    rootNode->setAttribute("Name", "ROOT");
    rootNode->setAttribute("Count", static_cast<int>(playlists.size()));
    
    int playlistId = 0;
    for (const auto& playlist : playlists)
    {
        rootNode->addChildElement(createPlaylistElement(playlist, playlistId++));
    }
    
    playlists_element->addChildElement(rootNode);
    return playlists_element;
}

juce::XmlElement* RekordboxExporter::createTrackElement(const DatabaseManager::Track& track, int trackId)
{
    auto* trackElement = new juce::XmlElement("TRACK");
    
    trackElement->setAttribute("TrackID", trackId);
    trackElement->setAttribute("Name", track.title.isEmpty() ? 
                               juce::File(track.filePath).getFileNameWithoutExtension() : track.title);
    trackElement->setAttribute("Artist", track.artist.isEmpty() ? "Unknown Artist" : track.artist);
    trackElement->setAttribute("Album", track.album);
    trackElement->setAttribute("Genre", track.genre);
    trackElement->setAttribute("Kind", juce::File(track.filePath).getFileExtension().toUpperCase() + " File");
    
    if (track.bpm > 0)
        trackElement->setAttribute("AverageBpm", track.bpm);
    
    if (!track.key.isEmpty())
        trackElement->setAttribute("Tonality", convertKeyToRekordbox(track.key));
    
    // Duration in seconds
    if (track.duration > 0)
        trackElement->setAttribute("TotalTime", static_cast<int>(track.duration));
    
    // File location
    trackElement->setAttribute("Location", generateTrackLocation(track.filePath));
    
    // Basic cue point support - add a memory cue at the start
    auto* tempoElement = new juce::XmlElement("TEMPO");
    tempoElement->setAttribute("Inizio", "0.000");
    if (track.bpm > 0)
        tempoElement->setAttribute("Bpm", juce::String(static_cast<double>(track.bpm), 2));
    trackElement->addChildElement(tempoElement);
    
    return trackElement;
}

juce::XmlElement* RekordboxExporter::createPlaylistElement(const DatabaseManager::VirtualFolder& playlist, 
                                                            int playlistId)
{
    auto* playlistElement = new juce::XmlElement("NODE");
    
    playlistElement->setAttribute("Type", "1");
    playlistElement->setAttribute("Name", playlist.name);
    playlistElement->setAttribute("KeyType", "0");
    
    // Get tracks in this playlist
    auto tracks = databaseManager.getTracksInFolder(playlist.id);
    playlistElement->setAttribute("Entries", static_cast<int>(tracks.size()));
    
    // Add track references
    int trackIndex = 0;
    for (const auto& track : tracks)
    {
        auto* trackRef = new juce::XmlElement("TRACK");
        trackRef->setAttribute("Key", trackIndex++);
        playlistElement->addChildElement(trackRef);
    }
    
    return playlistElement;
}

//==============================================================================
// Helper Methods

juce::String RekordboxExporter::convertKeyToRekordbox(const juce::String& key)
{
    // Rekordbox uses Camelot notation or traditional key names
    // This is a simple pass-through; you might want to add conversion logic
    return key;
}

juce::String RekordboxExporter::generateTrackLocation(const juce::String& filePath)
{
    // Rekordbox expects file:// URLs
    juce::File file(filePath);
    return "file://localhost" + file.getFullPathName().replace("\\", "/");
}

void RekordboxExporter::reportProgress(double progress, const juce::String& status)
{
    if (progressCallback)
        progressCallback(progress, status);
    
    DBG("[RekordboxExporter] Progress: " << static_cast<int>(progress * 100) << "% - " << status);
}
