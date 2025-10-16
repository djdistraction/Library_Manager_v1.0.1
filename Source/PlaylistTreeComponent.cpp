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
                    databaseManager.deleteVirtualFolder(virtualFolder.id);
                    if (auto* parent = getParentItem())
                        parent->removeSubItem(getIndexInParent());
                    break;
                case 3: // View tracks
                    DBG("View tracks in: " << virtualFolder.name);
                    break;
            }
        });
    }
}
