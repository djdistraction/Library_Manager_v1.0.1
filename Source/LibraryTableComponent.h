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
    LibraryTableComponent displays tracks in a table format with search and filter capabilities.
*/
class LibraryTableComponent : public juce::Component,
                               public juce::TableListBoxModel,
                               private juce::Timer
{
public:
    LibraryTableComponent(DatabaseManager& dbManager);
    ~LibraryTableComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // TableListBoxModel implementation
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    
    // Drag and drop support
    juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override;
    
    // Context menu and batch operations
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;
    
    void refreshTableContent();
    void setSearchFilter(const juce::String& searchText);
    
    const std::vector<DatabaseManager::Track>& getTracks() const { return tracks; }

private:
    DatabaseManager& databaseManager;
    juce::TableListBox table;
    std::vector<DatabaseManager::Track> tracks;
    juce::String currentSearchFilter;
    
    void timerCallback() override;
    void loadTracks();

    enum ColumnIds
    {
        Title = 1,
        Artist = 2,
        Album = 3,
        Genre = 4,
        BPM = 5,
        Key = 6,
        Duration = 7
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryTableComponent)
};
