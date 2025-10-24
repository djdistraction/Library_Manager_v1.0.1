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

#include "PlaylistTreeComponent.h"

//==============================================================================
PlaylistTreeComponent::PlaylistTreeComponent(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
    addAndMakeVisible(treeView);
    treeView.setColour(juce::TreeView::backgroundColourId, juce::Colour(0xff2d2d2d));
    treeView.setDefaultOpenness(true);
    
    loadPlaylists();
    
    // Start timer for periodic refresh (every 5 seconds)
    startTimer(5000);
}

PlaylistTreeComponent::~PlaylistTreeComponent()
{
    stopTimer();
    treeView.setRootItem(nullptr);
}

void PlaylistTreeComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void PlaylistTreeComponent::resized()
{
    treeView.setBounds(getLocalBounds());
}

void PlaylistTreeComponent::refreshTree()
{
    loadPlaylists();
}

void PlaylistTreeComponent::timerCallback()
{
    // Periodic refresh of playlists
    refreshTree();
}

void PlaylistTreeComponent::loadPlaylists()
{
    if (!databaseManager.isOpen())
        return;
    
    // Create a simple root item class
    class RootItem : public juce::TreeViewItem
    {
    public:
        bool mightContainSubItems() override { return true; }
        void paintItem(juce::Graphics&, int, int) override {}
    };
    
    // Create root item
    auto newRootItem = std::make_unique<RootItem>();
    newRootItem->setOpen(true);
    
    // Load all virtual folders
    auto folders = databaseManager.getAllVirtualFolders();
    
    for (const auto& folder : folders)
    {
        newRootItem->addSubItem(new PlaylistItem(folder, databaseManager));
    }
    
    // Update tree view
    treeView.setRootItem(nullptr);
    rootItem = std::move(newRootItem);
    treeView.setRootItem(rootItem.get());
}

//==============================================================================
PlaylistTreeComponent::PlaylistItem::PlaylistItem(const DatabaseManager::VirtualFolder& folder, 
                                                    DatabaseManager& dbManager)
    : virtualFolder(folder), databaseManager(dbManager)
{
    // Check if folder is verified (has tracks)
    auto tracks = databaseManager.getTracksInFolder(folder.id);
    isVerified = !tracks.empty();
}

void PlaylistTreeComponent::PlaylistItem::paintItem(juce::Graphics& g, int width, int height)
{
    // Set background color based on selection
    if (isSelected())
        g.fillAll(juce::Colours::lightblue);
    
    // Draw folder icon (verified folders get a checkmark)
    g.setColour(isVerified ? juce::Colours::green : juce::Colours::lightgrey);
    g.fillEllipse(4, height / 2 - 4, 8, 8);
    
    if (isVerified)
    {
        // Draw checkmark
        g.setColour(juce::Colours::white);
        g.drawLine(6, height / 2, 8, height / 2 + 2, 2.0f);
        g.drawLine(8, height / 2 + 2, 12, height / 2 - 2, 2.0f);
    }
    
    // Draw folder name
    g.setColour(juce::Colours::white);
    g.drawText(virtualFolder.name, 20, 0, width - 20, height, juce::Justification::centredLeft, true);
    
    // Draw track count
    auto tracks = databaseManager.getTracksInFolder(virtualFolder.id);
    juce::String countText = "(" + juce::String(tracks.size()) + ")";
    g.setColour(juce::Colours::grey);
    g.drawText(countText, width - 60, 0, 50, height, juce::Justification::centredRight, true);
}

void PlaylistTreeComponent::PlaylistItem::itemClicked(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        // Show context menu for playlist operations
        juce::PopupMenu menu;
        menu.addItem(1, "Rename");
        menu.addItem(2, "Delete");
        menu.addSeparator();
        menu.addItem(3, "View Tracks");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result)
        {
            switch (result)
            {
                case 1: // Rename
                    DBG("Rename playlist: " << virtualFolder.name);
                    break;
                case 2: // Delete
                    // Show confirmation dialog before deleting
                    juce::AlertWindow::showOkCancelBox(
                        juce::AlertWindow::WarningIcon,
                        "Delete Playlist",
                        "Are you sure you want to delete the playlist '" + virtualFolder.name + "'?\n\n"
                        "This action cannot be undone. The tracks will remain in your library.",
                        "Delete",
                        "Cancel",
                        nullptr,
                        juce::ModalCallbackFunction::create([this](int result) {
                            if (result == 1) // User clicked Delete
                            {
                                databaseManager.deleteVirtualFolder(virtualFolder.id);
                                if (auto* parent = getParentItem())
                                    parent->removeSubItem(getIndexInParent());
                            }
                        })
                    );
                    break;
                case 3: // View tracks
                    DBG("View tracks in: " << virtualFolder.name);
                    break;
            }
        });
    }
}

bool PlaylistTreeComponent::PlaylistItem::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    // Accept drops if the source contains track IDs (from LibraryTableComponent)
    return dragSourceDetails.description.isArray();
}

void PlaylistTreeComponent::PlaylistItem::itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails, int insertIndex)
{
    juce::ignoreUnused(insertIndex);
    
    // Get the array of track IDs from the drag source
    if (dragSourceDetails.description.isArray())
    {
        juce::Array<juce::var>* trackIds = dragSourceDetails.description.getArray();
        
        if (trackIds != nullptr)
        {
            int successCount = 0;
            int skipCount = 0;
            
            // Add each track to this playlist
            for (const auto& trackIdVar : *trackIds)
            {
                int64_t trackId = static_cast<int64_t>(static_cast<int>(trackIdVar));
                
                // Check if track is already in this folder
                auto existingTracks = databaseManager.getTracksInFolder(virtualFolder.id);
                bool alreadyInFolder = false;
                
                for (const auto& track : existingTracks)
                {
                    if (track.id == trackId)
                    {
                        alreadyInFolder = true;
                        break;
                    }
                }
                
                if (!alreadyInFolder)
                {
                    // Create link between track and folder
                    DatabaseManager::FolderTrackLink link;
                    link.folderId = virtualFolder.id;
                    link.trackId = trackId;
                    link.displayOrder = static_cast<int>(existingTracks.size());
                    link.dateAdded = juce::Time::getCurrentTime();
                    
                    int64_t linkId = 0;
                    if (databaseManager.addFolderTrackLink(link, linkId))
                    {
                        successCount++;
                    }
                }
                else
                {
                    skipCount++;
                }
            }
            
            DBG("[PlaylistTreeComponent] Added " << successCount << " track(s) to playlist '" 
                << virtualFolder.name << "' (skipped " << skipCount << " already in playlist)");
            
            // Update verification status
            auto tracks = databaseManager.getTracksInFolder(virtualFolder.id);
            isVerified = !tracks.empty();
            
            // Trigger repaint
            repaintItem();
        }
    }
}
