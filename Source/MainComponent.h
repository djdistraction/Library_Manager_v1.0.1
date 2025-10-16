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
#include "LibraryTableComponent.h"
#include "PlaylistTreeComponent.h"
#include "OnboardingComponent.h"
#include "RekordboxExporter.h"
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
    // UI Components
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextEditor searchBox;
    juce::TextButton scanButton;
    juce::TextButton exportButton;
    juce::TextButton newPlaylistButton;
    juce::Label progressLabel;
    
    std::unique_ptr<LibraryTableComponent> libraryTable;
    std::unique_ptr<PlaylistTreeComponent> playlistTree;
    std::unique_ptr<OnboardingComponent> onboardingComponent;
    
    // Backend components
    std::unique_ptr<DatabaseManager> databaseManager;
    std::unique_ptr<FileScanner> fileScanner;
    std::unique_ptr<AnalysisWorker> analysisWorker;
    std::unique_ptr<RekordboxExporter> rekordboxExporter;
    
    // State
    double progress = 0.0;
    juce::ProgressBar progressBar;
    juce::String currentStatus;
    std::atomic<bool> isScanningActive{false};
    bool showOnboarding = true;
    
    // Methods
    void initializeDatabase();
    void checkFirstRun();
    void startScan();
    void stopScan();
    void exportToRekordbox();
    void createNewPlaylist();
    void updateProgress();
    void timerCallback() override;
    void onSearchTextChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
