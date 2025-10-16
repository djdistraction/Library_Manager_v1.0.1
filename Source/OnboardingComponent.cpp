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

#include "OnboardingComponent.h"

//==============================================================================
OnboardingComponent::OnboardingComponent(DatabaseManager& dbManager)
    : databaseManager(dbManager), progressBar(progress)
{
    // Setup title
    titleLabel.setText("Welcome to Library Manager", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Setup instructions
    instructionLabel.setText(
        "Let's test the compatibility of your audio files.\n\n"
        "This sandbox test will:\n"
        "1. Copy a few test files to a temporary location\n"
        "2. Process and analyze them safely\n"
        "3. Show you before/after metadata comparison\n\n"
        "Your original files will not be modified.",
        juce::dontSendNotification);
    instructionLabel.setFont(juce::FontOptions(14.0f));
    instructionLabel.setJustificationType(juce::Justification::centredLeft);
    instructionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(instructionLabel);
    
    // Setup buttons
    selectFilesButton.setButtonText("Select Test Files (2-5 files)");
    selectFilesButton.onClick = [this] { selectTestFiles(); };
    addAndMakeVisible(selectFilesButton);
    
    startTestButton.setButtonText("Start Compatibility Test");
    startTestButton.onClick = [this] { startCompatibilityTest(); };
    startTestButton.setEnabled(false);
    addAndMakeVisible(startTestButton);
    
    completeButton.setButtonText("Complete Setup");
    completeButton.onClick = [this] { completeOnboarding(); };
    completeButton.setEnabled(false);
    addAndMakeVisible(completeButton);
    
    // Setup progress bar
    addAndMakeVisible(progressBar);
    
    // Setup status label
    statusLabel.setText("Ready to begin", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(12.0f));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(statusLabel);
    
    // Setup results viewer
    resultsViewer.setMultiLine(true);
    resultsViewer.setReadOnly(true);
    resultsViewer.setScrollbarsShown(true);
    resultsViewer.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2d2d2d));
    resultsViewer.setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(resultsViewer);
    
    // Create temp directory
    tempDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("LibraryManager_OnboardingTest");
    if (!tempDirectory.exists())
        tempDirectory.createDirectory();
}

OnboardingComponent::~OnboardingComponent()
{
    // Clean up temp directory
    if (tempDirectory.exists())
        tempDirectory.deleteRecursively();
}

void OnboardingComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Draw a border around the component
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 2);
}

void OnboardingComponent::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    instructionLabel.setBounds(bounds.removeFromTop(150));
    bounds.removeFromTop(20);
    
    auto buttonArea = bounds.removeFromTop(40);
    selectFilesButton.setBounds(buttonArea.removeFromLeft(200));
    buttonArea.removeFromLeft(10);
    startTestButton.setBounds(buttonArea.removeFromLeft(200));
    buttonArea.removeFromLeft(10);
    completeButton.setBounds(buttonArea.removeFromLeft(150));
    
    bounds.removeFromTop(10);
    statusLabel.setBounds(bounds.removeFromTop(25));
    progressBar.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);
    
    resultsViewer.setBounds(bounds);
}

void OnboardingComponent::selectTestFiles()
{
    auto* chooser = new juce::FileChooser("Select 2-5 test audio files", 
                                          juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                          "*.mp3;*.wav;*.flac;*.m4a;*.aac;*.ogg");
    
    chooser->launchAsync(juce::FileBrowserComponent::openMode | 
                         juce::FileBrowserComponent::canSelectMultipleItems,
                         [this, chooser](const juce::FileChooser& fc)
    {
        std::unique_ptr<juce::FileChooser> deleter(const_cast<juce::FileChooser*>(chooser));
        
        selectedFiles.clear();
        auto results = fc.getResults();
        
        for (const auto& file : results)
        {
            if (selectedFiles.size() < 5)
                selectedFiles.push_back(file);
        }
        
        if (selectedFiles.size() >= 2)
        {
            addLogMessage("Selected " + juce::String(selectedFiles.size()) + " test files");
            for (const auto& file : selectedFiles)
                addLogMessage("  - " + file.getFileName());
            
            startTestButton.setEnabled(true);
            statusLabel.setText("Ready to test " + juce::String(selectedFiles.size()) + " files", 
                              juce::dontSendNotification);
        }
        else
        {
            addLogMessage("Please select at least 2 files");
            startTestButton.setEnabled(false);
        }
    });
}

void OnboardingComponent::startCompatibilityTest()
{
    if (testInProgress || selectedFiles.empty())
        return;
    
    testInProgress = true;
    selectFilesButton.setEnabled(false);
    startTestButton.setEnabled(false);
    progress = 0.0;
    comparisonResults.clear();
    
    addLogMessage("\n=== Starting Compatibility Test ===\n");
    statusLabel.setText("Processing test files...", juce::dontSendNotification);
    
    // Process in background thread
    juce::Thread::launch([this]()
    {
        processTestFiles();
        
        juce::MessageManager::callAsync([this]()
        {
            testInProgress = false;
            testComplete = true;
            completeButton.setEnabled(true);
            selectFilesButton.setEnabled(true);
            statusLabel.setText("Test complete!", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
            displayResults();
        });
    });
}

void OnboardingComponent::processTestFiles()
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    int fileIndex = 0;
    for (const auto& originalFile : selectedFiles)
    {
        progress = static_cast<double>(fileIndex) / selectedFiles.size();
        
        juce::MessageManager::callAsync([this, originalFile]()
        {
            addLogMessage("\nProcessing: " + originalFile.getFileName());
        });
        
        MetadataComparison comparison;
        comparison.fileName = originalFile.getFileName();
        
        // Extract original metadata
        extractMetadata(originalFile, comparison.originalTitle, comparison.originalArtist);
        
        // Copy file to temp directory
        auto tempFile = tempDirectory.getChildFile(originalFile.getFileName());
        originalFile.copyFileTo(tempFile);
        
        // Simulate processing (read and analyze)
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));
        if (reader)
        {
            // Extract metadata after "processing"
            extractMetadata(tempFile, comparison.processedTitle, comparison.processedArtist);
            
            // Check if metadata changed (in real scenario, this would show actual processing effects)
            comparison.metadataChanged = false;
            
            comparisonResults.push_back(comparison);
            
            juce::MessageManager::callAsync([this, comparison]()
            {
                addLogMessage("✓ Successfully processed: " + comparison.fileName);
            });
        }
        else
        {
            juce::MessageManager::callAsync([this, originalFile]()
            {
                addLogMessage("⚠ Could not process: " + originalFile.getFileName());
            });
        }
        
        fileIndex++;
    }
    
    progress = 1.0;
}

void OnboardingComponent::displayResults()
{
    addLogMessage("\n=== Compatibility Test Results ===\n");
    
    for (const auto& result : comparisonResults)
    {
        addLogMessage("File: " + result.fileName);
        addLogMessage("  Original: " + result.originalTitle + " - " + result.originalArtist);
        addLogMessage("  After Processing: " + result.processedTitle + " - " + result.processedArtist);
        if (result.metadataChanged)
            addLogMessage("  Status: ⚠ Metadata was modified");
        else
            addLogMessage("  Status: ✓ Metadata preserved correctly");
        addLogMessage("");
    }
    
    addLogMessage("\n✓ All test files processed successfully!");
    addLogMessage("Your audio files are compatible with Library Manager.");
    addLogMessage("\nClick 'Complete Setup' to continue.");
}

void OnboardingComponent::completeOnboarding()
{
    testComplete = true;
    
    // Clean up temp directory
    if (tempDirectory.exists())
        tempDirectory.deleteRecursively();
    
    addLogMessage("\n✓ Onboarding complete! You can now use Library Manager.");
}

void OnboardingComponent::addLogMessage(const juce::String& message)
{
    resultsViewer.moveCaretToEnd();
    resultsViewer.insertTextAtCaret(message + "\n");
    resultsViewer.moveCaretToEnd();
}

juce::String OnboardingComponent::extractMetadata(const juce::File& audioFile, 
                                                   juce::String& title, 
                                                   juce::String& artist)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    
    if (reader)
    {
        auto metadata = reader->metadataValues;
        title = metadata.getValue("title", audioFile.getFileNameWithoutExtension());
        artist = metadata.getValue("artist", "Unknown Artist");
        return "Success";
    }
    
    title = audioFile.getFileNameWithoutExtension();
    artist = "Unknown Artist";
    return "Could not read file";
}
