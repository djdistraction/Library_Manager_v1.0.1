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

#include <juce_gui_basics/juce_gui_basics.h>
#include "DatabaseManager.h"

//==============================================================================
/**
    PlaylistTreeComponent displays virtual folders/playlists in a tree structure.
*/
class PlaylistTreeComponent : public juce::Component,
                               private juce::Timer
{
public:
    PlaylistTreeComponent(DatabaseManager& dbManager);
    ~PlaylistTreeComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    void refreshTree();
    
    class PlaylistItem;

private:
    DatabaseManager& databaseManager;
    juce::TreeView treeView;
    std::unique_ptr<juce::TreeViewItem> rootItem;
    
    void timerCallback() override;
    void loadPlaylists();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistTreeComponent)
};

//==============================================================================
/**
    PlaylistItem represents a single playlist/virtual folder in the tree.
*/
class PlaylistTreeComponent::PlaylistItem : public juce::TreeViewItem
{
public:
    PlaylistItem(const DatabaseManager::VirtualFolder& folder, DatabaseManager& dbManager);
    
    bool mightContainSubItems() override { return false; }
    void paintItem(juce::Graphics& g, int width, int height) override;
    void itemClicked(const juce::MouseEvent& e) override;
    
    const DatabaseManager::VirtualFolder& getFolder() const { return virtualFolder; }

private:
    DatabaseManager::VirtualFolder virtualFolder;
    DatabaseManager& databaseManager;
    bool isVerified = false;
};
