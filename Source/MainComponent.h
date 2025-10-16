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
#include "FileScanner.h"
#include "AnalysisWorker.h"
#include <atomic>

//==============================================================================
/*
    This component serves as the main content component for the Library Manager application.
    It will be the container for the music library interface and all related UI components.
*/
class MainComponent  : public juce::Component,
                       private juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label progressLabel;
    juce::TextButton scanButton;
    juce::TextButton stopButton;
    juce::ProgressBar progressBar;
    juce::TextEditor logViewer;
    
    std::unique_ptr<DatabaseManager> databaseManager;
    std::unique_ptr<FileScanner> fileScanner;
    std::unique_ptr<AnalysisWorker> analysisWorker;
    
    double progress = 0.0;
    juce::String currentStatus;
    std::atomic<bool> isScanningActive{false};
    
    void initializeDatabase();
    void startScan();
    void stopScan();
    void updateProgress();
    void timerCallback() override;
    void addLogMessage(const juce::String& message);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
