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
#include <juce_audio_formats/juce_audio_formats.h>
#include "DatabaseManager.h"

//==============================================================================
/**
    OnboardingComponent guides users through a sandbox compatibility test.
    It allows users to select test files, processes them in a temporary directory,
    and displays before/after metadata comparison.
*/
class OnboardingComponent : public juce::Component
{
public:
    OnboardingComponent(DatabaseManager& dbManager);
    ~OnboardingComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    bool isComplete() const { return testComplete; }

private:
    DatabaseManager& databaseManager;
    
    // UI Components
    juce::Label titleLabel;
    juce::Label instructionLabel;
    juce::TextButton selectFilesButton;
    juce::TextButton startTestButton;
    juce::TextButton completeButton;
    juce::Label statusLabel;
    juce::TextEditor resultsViewer;
    
    // State
    std::vector<juce::File> selectedFiles;
    juce::File tempDirectory;
    double progress = 0.0;
    juce::ProgressBar progressBar;
    bool testInProgress = false;
    bool testComplete = false;
    
    struct MetadataComparison
    {
        juce::String fileName;
        juce::String originalTitle;
        juce::String originalArtist;
        juce::String processedTitle;
        juce::String processedArtist;
        bool metadataChanged = false;
    };
    
    std::vector<MetadataComparison> comparisonResults;
    
    // Methods
    void selectTestFiles();
    void startCompatibilityTest();
    void processTestFiles();
    void displayResults();
    void completeOnboarding();
    
    void addLogMessage(const juce::String& message);
    juce::String extractMetadata(const juce::File& audioFile, juce::String& title, juce::String& artist);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnboardingComponent)
};
