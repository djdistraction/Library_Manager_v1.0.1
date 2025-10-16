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

//==============================================================================
/**
    BatchMetadataEditor allows editing metadata for multiple tracks simultaneously.
    
    Features:
    - Edit common fields across multiple tracks
    - Option to preserve existing values
    - Undo support
*/
class BatchMetadataEditor : public juce::Component
{
public:
    BatchMetadataEditor(DatabaseManager& dbManager, const std::vector<int64_t>& trackIds);
    ~BatchMetadataEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    /**
     * Show the editor as a modal dialog.
     * @return true if changes were applied, false if cancelled
     */
    bool showModal();

private:
    DatabaseManager& databaseManager;
    std::vector<int64_t> selectedTrackIds;
    
    // UI Components
    juce::Label instructionLabel;
    
    juce::Label artistLabel;
    juce::TextEditor artistEditor;
    juce::ToggleButton artistCheckbox;
    
    juce::Label albumLabel;
    juce::TextEditor albumEditor;
    juce::ToggleButton albumCheckbox;
    
    juce::Label genreLabel;
    juce::TextEditor genreEditor;
    juce::ToggleButton genreCheckbox;
    
    juce::Label bpmLabel;
    juce::TextEditor bpmEditor;
    juce::ToggleButton bpmCheckbox;
    
    juce::Label keyLabel;
    juce::TextEditor keyEditor;
    juce::ToggleButton keyCheckbox;
    
    juce::TextButton applyButton;
    juce::TextButton cancelButton;
    
    bool applyChanges = false;
    
    void onApplyClicked();
    void onCancelClicked();
    void updateCheckboxStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchMetadataEditor)
};
