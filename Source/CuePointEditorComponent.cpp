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

#include "CuePointEditorComponent.h"

//==============================================================================
CuePointEditorComponent::CuePointEditorComponent(DatabaseManager& dbManager)
    : databaseManager(dbManager)
{
    // Setup waveform
    addAndMakeVisible(waveform);
    waveform.onSeek = [this](double position)
    {
        onWaveformClicked(position);
    };
    
    // Setup cue point table
    addAndMakeVisible(cuePointTable);
    cuePointTable.setModel(this);
    cuePointTable.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
    cuePointTable.setOutlineThickness(1);
    
    cuePointTable.getHeader().addColumn("Position", 1, 80);
    cuePointTable.getHeader().addColumn("Name", 2, 150);
    cuePointTable.getHeader().addColumn("Type", 3, 100);
    cuePointTable.getHeader().addColumn("Hot Cue", 4, 70);
    cuePointTable.getHeader().addColumn("Color", 5, 60);
    
    // Setup buttons
    addAndMakeVisible(addCueButton);
    addCueButton.setButtonText("Add Cue");
    addCueButton.onClick = [this]() { addNewCuePoint(); };
    
    addAndMakeVisible(deleteCueButton);
    deleteCueButton.setButtonText("Delete");
    deleteCueButton.onClick = [this]() { deleteSelectedCuePoint(); };
    
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save");
    saveButton.onClick = [this]() { saveChanges(); };
    
    addAndMakeVisible(cancelButton);
    cancelButton.setButtonText("Cancel");
    cancelButton.onClick = [this]() { discardChanges(); };
    
    addAndMakeVisible(jumpToCueButton);
    jumpToCueButton.setButtonText("Jump to Cue");
    jumpToCueButton.onClick = [this]()
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            waveform.setPlaybackPosition(cuePoints[selectedCueIndex].position);
        }
    };
    
    // Setup cue details editors
    addAndMakeVisible(nameLabel);
    nameLabel.setText("Name:", juce::dontSendNotification);
    
    addAndMakeVisible(nameEditor);
    nameEditor.setMultiLine(false);
    nameEditor.onTextChange = [this]()
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            cuePoints[selectedCueIndex].name = nameEditor.getText();
            updateCuePointDisplay();
        }
    };
    
    addAndMakeVisible(positionLabel);
    positionLabel.setText("Position (s):", juce::dontSendNotification);
    
    addAndMakeVisible(positionEditor);
    positionEditor.setMultiLine(false);
    positionEditor.onTextChange = [this]()
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            cuePoints[selectedCueIndex].position = positionEditor.getText().getDoubleValue();
            updateCuePointDisplay();
        }
    };
    
    addAndMakeVisible(typeLabel);
    typeLabel.setText("Type:", juce::dontSendNotification);
    
    addAndMakeVisible(typeComboBox);
    typeComboBox.addItem("Memory Cue", 1);
    typeComboBox.addItem("Hot Cue", 2);
    typeComboBox.addItem("Loop In", 3);
    typeComboBox.addItem("Loop Out", 4);
    typeComboBox.onChange = [this]()
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            cuePoints[selectedCueIndex].type = typeComboBox.getSelectedId() - 1;
            updateCuePointDisplay();
        }
    };
    
    addAndMakeVisible(hotCueLabel);
    hotCueLabel.setText("Hot Cue #:", juce::dontSendNotification);
    
    addAndMakeVisible(hotCueComboBox);
    hotCueComboBox.addItem("None", 1);
    for (int i = 0; i < 8; ++i)
    {
        hotCueComboBox.addItem(juce::String(i), i + 2);
    }
    hotCueComboBox.onChange = [this]()
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            cuePoints[selectedCueIndex].hotCueNumber = hotCueComboBox.getSelectedId() - 2;
            updateCuePointDisplay();
        }
    };
    
    addAndMakeVisible(colorLabel);
    colorLabel.setText("Color:", juce::dontSendNotification);
    
    addAndMakeVisible(colorButton);
    colorButton.setButtonText("Choose...");
    colorButton.onClick = [this]()
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            auto& cue = cuePoints[selectedCueIndex];
            juce::Colour currentColor = juce::Colour::fromString(cue.color);
            
            // Create a color picker dialog with ChangeListener for real-time updates
            auto selector = std::make_unique<juce::ColourSelector>(juce::ColourSelector::showColourspace);
            selector->setCurrentColour(currentColor);
            selector->setSize(300, 400);
            selector->addChangeListener(this);
            
            juce::CallOutBox::launchAsynchronously(std::move(selector), 
                                                   colorButton.getScreenBounds(), nullptr);
        }
    };
}

CuePointEditorComponent::~CuePointEditorComponent()
{
}

//==============================================================================
bool CuePointEditorComponent::loadTrack(int64_t trackId)
{
    currentTrackId = trackId;
    currentTrack = databaseManager.getTrack(trackId);
    
    if (currentTrack.id == 0)
    {
        DBG("[CuePointEditorComponent] Track not found: " << trackId);
        return false;
    }
    
    // Load cue points
    cuePoints = databaseManager.getCuePointsForTrack(trackId);
    originalCuePoints = cuePoints;
    
    // Load waveform
    juce::File audioFile(currentTrack.filePath);
    if (!waveform.loadAudioFile(audioFile))
    {
        DBG("[CuePointEditorComponent] Failed to load waveform");
        return false;
    }
    
    updateCuePointDisplay();
    
    DBG("[CuePointEditorComponent] Loaded track: " << currentTrack.title << " with " << cuePoints.size() << " cue points");
    return true;
}

bool CuePointEditorComponent::saveChanges()
{
    // Delete removed cue points
    for (const auto& original : originalCuePoints)
    {
        bool found = false;
        for (const auto& current : cuePoints)
        {
            if (current.id == original.id)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            databaseManager.deleteCuePoint(original.id);
        }
    }
    
    // Update or add cue points
    for (auto& cue : cuePoints)
    {
        if (cue.id == 0)
        {
            // New cue point
            cue.trackId = currentTrackId;
            cue.dateCreated = juce::Time::getCurrentTime();
            int64_t newId;
            if (databaseManager.addCuePoint(cue, newId))
            {
                cue.id = newId;
            }
        }
        else
        {
            // Existing cue point - update it
            databaseManager.updateCuePoint(cue);
        }
    }
    
    originalCuePoints = cuePoints;
    
    DBG("[CuePointEditorComponent] Saved changes for track: " << currentTrack.title);
    return true;
}

void CuePointEditorComponent::discardChanges()
{
    cuePoints = originalCuePoints;
    updateCuePointDisplay();
    selectedCueIndex = -1;
    
    DBG("[CuePointEditorComponent] Discarded changes");
}

//==============================================================================
void CuePointEditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CuePointEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Waveform at the top
    auto waveformBounds = bounds.removeFromTop(200);
    waveform.setBounds(waveformBounds);
    
    bounds.removeFromTop(10);
    
    // Cue point table in the middle
    auto tableBounds = bounds.removeFromTop(200);
    cuePointTable.setBounds(tableBounds);
    
    bounds.removeFromTop(10);
    
    // Controls at the bottom
    auto buttonRow = bounds.removeFromTop(30);
    addCueButton.setBounds(buttonRow.removeFromLeft(80));
    buttonRow.removeFromLeft(5);
    deleteCueButton.setBounds(buttonRow.removeFromLeft(80));
    buttonRow.removeFromLeft(5);
    jumpToCueButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(20);
    saveButton.setBounds(buttonRow.removeFromRight(80));
    buttonRow.removeFromRight(5);
    cancelButton.setBounds(buttonRow.removeFromRight(80));
    
    bounds.removeFromTop(10);
    
    // Cue details editors
    auto detailsRow = bounds.removeFromTop(25);
    nameLabel.setBounds(detailsRow.removeFromLeft(80));
    nameEditor.setBounds(detailsRow.removeFromLeft(150));
    detailsRow.removeFromLeft(10);
    positionLabel.setBounds(detailsRow.removeFromLeft(80));
    positionEditor.setBounds(detailsRow.removeFromLeft(80));
    
    bounds.removeFromTop(5);
    detailsRow = bounds.removeFromTop(25);
    typeLabel.setBounds(detailsRow.removeFromLeft(80));
    typeComboBox.setBounds(detailsRow.removeFromLeft(120));
    detailsRow.removeFromLeft(10);
    hotCueLabel.setBounds(detailsRow.removeFromLeft(80));
    hotCueComboBox.setBounds(detailsRow.removeFromLeft(80));
    detailsRow.removeFromLeft(10);
    colorLabel.setBounds(detailsRow.removeFromLeft(50));
    colorButton.setBounds(detailsRow.removeFromLeft(80));
}

//==============================================================================
int CuePointEditorComponent::getNumRows()
{
    return static_cast<int>(cuePoints.size());
}

void CuePointEditorComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::lightblue);
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colours::white);
    else
        g.fillAll(juce::Colour(0xfff0f0f0));
}

void CuePointEditorComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= static_cast<int>(cuePoints.size()))
        return;
    
    const auto& cue = cuePoints[rowNumber];
    
    g.setColour(rowIsSelected ? juce::Colours::black : juce::Colours::darkgrey);
    
    juce::String text;
    
    switch (columnId)
    {
        case 1: // Position
            text = juce::String(cue.position, 2) + "s";
            break;
        case 2: // Name
            text = cue.name;
            break;
        case 3: // Type
            text = cueTypeToString(cue.type);
            break;
        case 4: // Hot Cue
            text = (cue.hotCueNumber >= 0) ? juce::String(cue.hotCueNumber) : "-";
            break;
        case 5: // Color
            if (cue.color.isNotEmpty())
            {
                g.setColour(juce::Colour::fromString(cue.color));
                g.fillRect(2, 2, width - 4, height - 4);
                return;
            }
            break;
    }
    
    g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
}

void CuePointEditorComponent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent&)
{
    selectedCueIndex = rowNumber;
    updateSelectedCueDetails();
}

//==============================================================================
void CuePointEditorComponent::updateCuePointDisplay()
{
    cuePointTable.updateContent();
    
    // Update waveform cue points
    std::vector<double> positions;
    for (const auto& cue : cuePoints)
    {
        positions.push_back(cue.position);
    }
    waveform.setCuePoints(positions);
}

void CuePointEditorComponent::updateSelectedCueDetails()
{
    if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
    {
        const auto& cue = cuePoints[selectedCueIndex];
        
        nameEditor.setText(cue.name, false);
        positionEditor.setText(juce::String(cue.position, 2), false);
        typeComboBox.setSelectedId(cue.type + 1, juce::dontSendNotification);
        hotCueComboBox.setSelectedId(cue.hotCueNumber + 2, juce::dontSendNotification);
    }
}

void CuePointEditorComponent::addNewCuePoint()
{
    DatabaseManager::CuePoint newCue;
    newCue.id = 0; // New cue point
    newCue.trackId = currentTrackId;
    newCue.position = waveform.getPlaybackPosition();
    newCue.name = "Cue " + juce::String(cuePoints.size() + 1);
    newCue.type = 0; // Memory Cue
    newCue.hotCueNumber = -1;
    newCue.color = "#FF0000";
    newCue.dateCreated = juce::Time::getCurrentTime();
    
    cuePoints.push_back(newCue);
    updateCuePointDisplay();
    
    DBG("[CuePointEditorComponent] Added new cue point at position: " << newCue.position);
}

void CuePointEditorComponent::deleteSelectedCuePoint()
{
    if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
    {
        cuePoints.erase(cuePoints.begin() + selectedCueIndex);
        selectedCueIndex = -1;
        updateCuePointDisplay();
        
        DBG("[CuePointEditorComponent] Deleted cue point");
    }
}

void CuePointEditorComponent::onWaveformClicked(double position)
{
    // Set position for next cue point creation
    DBG("[CuePointEditorComponent] Waveform clicked at position: " << position);
}

juce::String CuePointEditorComponent::cueTypeToString(int type) const
{
    switch (type)
    {
        case 0: return "Memory Cue";
        case 1: return "Hot Cue";
        case 2: return "Loop In";
        case 3: return "Loop Out";
        default: return "Unknown";
    }
}

int CuePointEditorComponent::stringToCueType(const juce::String& str) const
{
    if (str == "Memory Cue") return 0;
    if (str == "Hot Cue") return 1;
    if (str == "Loop In") return 2;
    if (str == "Loop Out") return 3;
    return 0;
}

void CuePointEditorComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Handle color selector changes
    if (auto* selector = dynamic_cast<juce::ColourSelector*>(source))
    {
        if (selectedCueIndex >= 0 && selectedCueIndex < static_cast<int>(cuePoints.size()))
        {
            auto newColor = selector->getCurrentColour();
            cuePoints[selectedCueIndex].color = newColor.toString();
            updateCuePointDisplay();
        }
    }
}
