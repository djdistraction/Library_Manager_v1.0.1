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

#include "LibraryTableComponent.h"

//==============================================================================
LibraryTableComponent::LibraryTableComponent(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
    // Setup table
    addAndMakeVisible(table);
    table.setModel(this);
    table.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
    table.setOutlineThickness(1);
    table.setMultipleSelectionEnabled(true);
    
    // Enable drag and drop
    table.getVerticalScrollBar().setAutoHide(false);
    
    // Add columns
    table.getHeader().addColumn("Title", ColumnIds::Title, 200, 50, 400, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Artist", ColumnIds::Artist, 150, 50, 300, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Album", ColumnIds::Album, 150, 50, 300, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Genre", ColumnIds::Genre, 100, 50, 200, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("BPM", ColumnIds::BPM, 60, 40, 100, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Key", ColumnIds::Key, 60, 40, 100, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Duration", ColumnIds::Duration, 80, 60, 120, juce::TableHeaderComponent::defaultFlags);
    
    // Load initial tracks
    loadTracks();
    
    // Start timer for periodic refresh (every 5 seconds)
    startTimer(5000);
}

LibraryTableComponent::~LibraryTableComponent()
{
    stopTimer();
}

void LibraryTableComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void LibraryTableComponent::resized()
{
    table.setBounds(getLocalBounds());
}

int LibraryTableComponent::getNumRows()
{
    return static_cast<int>(tracks.size());
}

void LibraryTableComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::lightblue);
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff2d2d2d));
    else
        g.fillAll(juce::Colour(0xff252525));
}

void LibraryTableComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= 0 && rowNumber < static_cast<int>(tracks.size()))
    {
        const auto& track = tracks[rowNumber];
        
        g.setColour(rowIsSelected ? juce::Colours::darkblue : juce::Colours::white);
        
        juce::String text;
        switch (columnId)
        {
            case ColumnIds::Title:
                text = track.title.isEmpty() ? track.filePath.fromLastOccurrenceOf("/", false, false) : track.title;
                break;
            case ColumnIds::Artist:
                text = track.artist;
                break;
            case ColumnIds::Album:
                text = track.album;
                break;
            case ColumnIds::Genre:
                text = track.genre;
                break;
            case ColumnIds::BPM:
                text = track.bpm > 0 ? juce::String(track.bpm) : "";
                break;
            case ColumnIds::Key:
                text = track.key;
                break;
            case ColumnIds::Duration:
                if (track.duration > 0)
                {
                    int minutes = static_cast<int>(track.duration) / 60;
                    int seconds = static_cast<int>(track.duration) % 60;
                    text = juce::String(minutes) + ":" + juce::String(seconds).paddedLeft('0', 2);
                }
                break;
        }
        
        g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

void LibraryTableComponent::refreshTableContent()
{
    loadTracks();
    table.updateContent();
}

void LibraryTableComponent::setSearchFilter(const juce::String& searchText)
{
    currentSearchFilter = searchText;
    loadTracks();
    table.updateContent();
}

void LibraryTableComponent::timerCallback()
{
    // Periodic refresh of tracks
    refreshTableContent();
}

void LibraryTableComponent::loadTracks()
{
    if (!databaseManager.isOpen())
        return;
    
    if (currentSearchFilter.isEmpty())
    {
        tracks = databaseManager.getAllTracks();
    }
    else
    {
        tracks = databaseManager.searchTracks(currentSearchFilter);
    }
}

juce::var LibraryTableComponent::getDragSourceDescription(const juce::SparseSet<int>& selectedRows)
{
    // Create an array of track IDs for the selected rows
    juce::Array<juce::var> trackIds;
    
    for (int i = 0; i < selectedRows.size(); ++i)
    {
        int rowNumber = selectedRows[i];
        if (rowNumber >= 0 && rowNumber < static_cast<int>(tracks.size()))
        {
            trackIds.add(static_cast<juce::int64>(tracks[rowNumber].id));
        }
    }
    
    // Return the array as a variant
    return juce::var(trackIds);
}
