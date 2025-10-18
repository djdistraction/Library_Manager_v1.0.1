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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DatabaseManager.h"
#include "WaveformComponent.h"
#include "AudioPreviewComponent.h"

//==============================================================================
/**
    CuePointEditorComponent provides advanced cue point editing with visual feedback.
    
    Features:
    - Visual cue point placement on waveform
    - Drag to reposition cue points
    - Color picker for cue point colors
    - Hot cue assignment (0-7)
    - Type selection (Memory Cue, Hot Cue, Loop In/Out)
    - Real-time preview with audio playback
*/
class CuePointEditorComponent : public juce::Component,
                                public juce::TableListBoxModel
{
public:
    CuePointEditorComponent(DatabaseManager& dbManager);
    ~CuePointEditorComponent() override;

    /**
     * Load a track for cue point editing.
     * @param trackId Database ID of the track
     * @return true if loading was successful
     */
    bool loadTrack(int64_t trackId);
    
    /**
     * Save all changes to the database.
     * @return true if save was successful
     */
    bool saveChanges();
    
    /**
     * Discard all unsaved changes.
     */
    void discardChanges();

    void paint(juce::Graphics&) override;
    void resized() override;
    
    // TableListBoxModel methods
    int getNumRows() override;
    void paintRowBackground(juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    
private:
    DatabaseManager& databaseManager;
    int64_t currentTrackId = 0;
    DatabaseManager::Track currentTrack;
    std::vector<DatabaseManager::CuePoint> cuePoints;
    std::vector<DatabaseManager::CuePoint> originalCuePoints;
    
    // UI Components
    WaveformComponent waveform;
    juce::TableListBox cuePointTable;
    
    juce::TextButton addCueButton;
    juce::TextButton deleteCueButton;
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
    juce::TextButton jumpToCueButton;
    
    juce::Label typeLabel;
    juce::ComboBox typeComboBox;
    
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    
    juce::Label positionLabel;
    juce::TextEditor positionEditor;
    
    juce::Label colorLabel;
    juce::ColourSelector colorSelector;
    juce::TextButton colorButton;
    
    juce::Label hotCueLabel;
    juce::ComboBox hotCueComboBox;
    
    int selectedCueIndex = -1;
    
    // Helper methods
    void updateCuePointDisplay();
    void updateSelectedCueDetails();
    void addNewCuePoint();
    void deleteSelectedCuePoint();
    void onWaveformClicked(double position);
    juce::String cueTypeToString(int type) const;
    int stringToCueType(const juce::String& str) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuePointEditorComponent)
};
