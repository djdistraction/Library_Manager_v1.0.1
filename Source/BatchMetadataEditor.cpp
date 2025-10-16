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

#include "BatchMetadataEditor.h"

//==============================================================================
BatchMetadataEditor::BatchMetadataEditor(DatabaseManager& dbManager, const std::vector<int64_t>& trackIds)
    : databaseManager(dbManager), selectedTrackIds(trackIds)
{
    setSize(500, 400);
    
    // Instruction label
    instructionLabel.setText("Edit metadata for " + juce::String(trackIds.size()) + " selected track(s). Check boxes to enable editing.", juce::dontSendNotification);
    instructionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    instructionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(instructionLabel);
    
    // Artist field
    artistLabel.setText("Artist:", juce::dontSendNotification);
    artistLabel.attachToComponent(&artistEditor, true);
    addAndMakeVisible(artistLabel);
    
    artistEditor.setMultiLine(false);
    addAndMakeVisible(artistEditor);
    
    artistCheckbox.setButtonText("Update");
    addAndMakeVisible(artistCheckbox);
    
    // Album field
    albumLabel.setText("Album:", juce::dontSendNotification);
    albumLabel.attachToComponent(&albumEditor, true);
    addAndMakeVisible(albumLabel);
    
    albumEditor.setMultiLine(false);
    addAndMakeVisible(albumEditor);
    
    albumCheckbox.setButtonText("Update");
    addAndMakeVisible(albumCheckbox);
    
    // Genre field
    genreLabel.setText("Genre:", juce::dontSendNotification);
    genreLabel.attachToComponent(&genreEditor, true);
    addAndMakeVisible(genreLabel);
    
    genreEditor.setMultiLine(false);
    addAndMakeVisible(genreEditor);
    
    genreCheckbox.setButtonText("Update");
    addAndMakeVisible(genreCheckbox);
    
    // BPM field
    bpmLabel.setText("BPM:", juce::dontSendNotification);
    bpmLabel.attachToComponent(&bpmEditor, true);
    addAndMakeVisible(bpmLabel);
    
    bpmEditor.setMultiLine(false);
    bpmEditor.setInputRestrictions(3, "0123456789");
    addAndMakeVisible(bpmEditor);
    
    bpmCheckbox.setButtonText("Update");
    addAndMakeVisible(bpmCheckbox);
    
    // Key field
    keyLabel.setText("Key:", juce::dontSendNotification);
    keyLabel.attachToComponent(&keyEditor, true);
    addAndMakeVisible(keyLabel);
    
    keyEditor.setMultiLine(false);
    addAndMakeVisible(keyEditor);
    
    keyCheckbox.setButtonText("Update");
    addAndMakeVisible(keyCheckbox);
    
    // Buttons
    applyButton.setButtonText("Apply Changes");
    applyButton.onClick = [this] { onApplyClicked(); };
    addAndMakeVisible(applyButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.onClick = [this] { onCancelClicked(); };
    addAndMakeVisible(cancelButton);
}

BatchMetadataEditor::~BatchMetadataEditor()
{
}

void BatchMetadataEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void BatchMetadataEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    instructionLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    const int rowHeight = 30;
    const int labelWidth = 80;
    const int checkboxWidth = 70;
    const int spacing = 10;
    
    // Artist
    auto artistRow = bounds.removeFromTop(rowHeight);
    artistRow.removeFromLeft(labelWidth);
    artistCheckbox.setBounds(artistRow.removeFromRight(checkboxWidth));
    artistRow.removeFromRight(spacing);
    artistEditor.setBounds(artistRow);
    bounds.removeFromTop(spacing);
    
    // Album
    auto albumRow = bounds.removeFromTop(rowHeight);
    albumRow.removeFromLeft(labelWidth);
    albumCheckbox.setBounds(albumRow.removeFromRight(checkboxWidth));
    albumRow.removeFromRight(spacing);
    albumEditor.setBounds(albumRow);
    bounds.removeFromTop(spacing);
    
    // Genre
    auto genreRow = bounds.removeFromTop(rowHeight);
    genreRow.removeFromLeft(labelWidth);
    genreCheckbox.setBounds(genreRow.removeFromRight(checkboxWidth));
    genreRow.removeFromRight(spacing);
    genreEditor.setBounds(genreRow);
    bounds.removeFromTop(spacing);
    
    // BPM
    auto bpmRow = bounds.removeFromTop(rowHeight);
    bpmRow.removeFromLeft(labelWidth);
    bpmCheckbox.setBounds(bpmRow.removeFromRight(checkboxWidth));
    bpmRow.removeFromRight(spacing);
    bpmEditor.setBounds(bpmRow);
    bounds.removeFromTop(spacing);
    
    // Key
    auto keyRow = bounds.removeFromTop(rowHeight);
    keyRow.removeFromLeft(labelWidth);
    keyCheckbox.setBounds(keyRow.removeFromRight(checkboxWidth));
    keyRow.removeFromRight(spacing);
    keyEditor.setBounds(keyRow);
    bounds.removeFromTop(spacing * 2);
    
    // Buttons
    auto buttonRow = bounds.removeFromTop(40);
    const int buttonWidth = 120;
    cancelButton.setBounds(buttonRow.removeFromRight(buttonWidth));
    buttonRow.removeFromRight(spacing);
    applyButton.setBounds(buttonRow.removeFromRight(buttonWidth));
}

bool BatchMetadataEditor::showModal()
{
    // Show as a modal component
    enterModalState(true, nullptr, true);
    
    return applyChanges;
}

void BatchMetadataEditor::onApplyClicked()
{
    // Apply changes to all selected tracks
    int successCount = 0;
    
    for (auto trackId : selectedTrackIds)
    {
        auto track = databaseManager.getTrack(trackId);
        
        if (track.id == 0)
            continue;
        
        bool modified = false;
        
        // Update fields based on checkboxes
        if (artistCheckbox.getToggleState() && artistEditor.getText().isNotEmpty())
        {
            track.artist = artistEditor.getText();
            modified = true;
        }
        
        if (albumCheckbox.getToggleState() && albumEditor.getText().isNotEmpty())
        {
            track.album = albumEditor.getText();
            modified = true;
        }
        
        if (genreCheckbox.getToggleState() && genreEditor.getText().isNotEmpty())
        {
            track.genre = genreEditor.getText();
            modified = true;
        }
        
        if (bpmCheckbox.getToggleState() && bpmEditor.getText().isNotEmpty())
        {
            track.bpm = bpmEditor.getText().getIntValue();
            modified = true;
        }
        
        if (keyCheckbox.getToggleState() && keyEditor.getText().isNotEmpty())
        {
            track.key = keyEditor.getText();
            modified = true;
        }
        
        if (modified)
        {
            track.lastModified = juce::Time::getCurrentTime();
            if (databaseManager.updateTrack(track))
                successCount++;
        }
    }
    
    DBG("[BatchMetadataEditor] Updated " << successCount << " track(s)");
    
    // Show confirmation
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                           "Batch Update Complete",
                                           "Successfully updated " + juce::String(successCount) + " track(s).");
    
    applyChanges = (successCount > 0);
    
    exitModalState(0);
}

void BatchMetadataEditor::onCancelClicked()
{
    applyChanges = false;
    
    exitModalState(0);
}
