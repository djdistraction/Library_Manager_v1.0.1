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

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : progress(0.0),  // Initialize progress before progressBar
      progressBar(progress)  // Initialize progress bar with progress variable
{
    // Set the main component size
    setSize (1200, 800);

    // Setup title label
    titleLabel.setText ("Library Manager v1.0.1", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (32.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);
    
    // Setup status label
    statusLabel.setText ("Initializing...", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (14.0f));
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (statusLabel);
    
    // Setup progress label
    progressLabel.setText ("Ready", juce::dontSendNotification);
    progressLabel.setFont (juce::Font (12.0f));
    progressLabel.setJustificationType (juce::Justification::centred);
    progressLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (progressLabel);
    
    // Setup scan button
    scanButton.setButtonText ("Scan Library");
    scanButton.onClick = [this] { startScan(); };
    addAndMakeVisible (scanButton);
    
    // Setup stop button
    stopButton.setButtonText ("Stop");
    stopButton.onClick = [this] { stopScan(); };
    stopButton.setEnabled (false);
    addAndMakeVisible (stopButton);
    
    // Setup progress bar
    addAndMakeVisible (progressBar);
    
    // Setup log viewer
    logViewer.setMultiLine (true);
    logViewer.setReadOnly (true);
    logViewer.setScrollbarsShown (true);
    logViewer.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff2d2d2d));
    logViewer.setColour (juce::TextEditor::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (logViewer);
    
    // Initialize database
    initializeDatabase();
    
    // Start timer for UI updates (500ms is sufficient for status updates)
    startTimer (500);  // Update every 500ms
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (analysisWorker)
        analysisWorker->stopWorker();
}

void MainComponent::initializeDatabase()
{
    databaseManager = std::make_unique<DatabaseManager>();
    
    // Get the database file path in the user's application data directory
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("LibraryManager");
    
    // Create directory if it doesn't exist
    if (!appDataDir.exists())
    {
        appDataDir.createDirectory();
    }
    
    auto dbFile = appDataDir.getChildFile("library.db");
    
    DBG("Database file path: " + dbFile.getFullPathName());
    addLogMessage("Database file: " + dbFile.getFullPathName());
    
    if (databaseManager->initialize(dbFile))
    {
        statusLabel.setText("Database initialized successfully", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        addLogMessage("Database initialized successfully");
        DBG("Database initialized successfully");
        
        // Initialize file scanner and analysis worker
        fileScanner = std::make_unique<FileScanner>(*databaseManager);
        analysisWorker = std::make_unique<AnalysisWorker>(*databaseManager);
        
        // Set up worker progress callback
        analysisWorker->setProgressCallback([this](const AnalysisWorker::ProgressInfo& info) {
            juce::MessageManager::callAsync([this, info]() {
                juce::String msg = "Processing: " + info.filePath.fromLastOccurrenceOf("/", false, false);
                if (!info.errorMessage.isEmpty())
                    msg += " (Error: " + info.errorMessage + ")";
                addLogMessage(msg);
            });
        });
        
        // Start the analysis worker
        analysisWorker->startWorker();
        addLogMessage("Analysis worker started");
    }
    else
    {
        statusLabel.setText("Database initialization failed: " + databaseManager->getLastError(), 
                          juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        addLogMessage("ERROR: Database initialization failed: " + databaseManager->getLastError());
        DBG("Database initialization failed: " + databaseManager->getLastError());
    }
}

void MainComponent::startScan()
{
    // Use a simple FileChooser without shared_ptr
    auto* chooser = new juce::FileChooser("Select music library folder");
    
    chooser->launchAsync(juce::FileBrowserComponent::openMode | 
                         juce::FileBrowserComponent::canSelectDirectories,
                         [this, chooser](const juce::FileChooser& fc) {
        // Capture chooser to ensure it's deleted after use
        std::unique_ptr<juce::FileChooser> deleter(const_cast<juce::FileChooser*>(chooser));
        
        auto results = fc.getResults();
        if (results.isEmpty())
            return;
        
        auto directory = results.getFirst();
        addLogMessage("Starting scan of: " + directory.getFullPathName());
        
        scanButton.setEnabled(false);
        stopButton.setEnabled(true);
        progress = 0.0;
        
        // Store scanning state flag
        isScanningActive = true;
        
        // Run scan in background thread with proper lifetime management
        juce::Thread::launch([this, directory]() {
            if (!isScanningActive)
                return;
                
            fileScanner->setProgressCallback([this](int current, int total) {
                if (!isScanningActive)
                    return;
                    
                progress = (double)current / (double)total;
                juce::MessageManager::callAsync([this, current, total]() {
                    if (!isScanningActive)
                        return;
                    progressLabel.setText("Scanning: " + juce::String(current) + "/" + juce::String(total),
                                        juce::dontSendNotification);
                });
            });
            
            int filesFound = fileScanner->scanDirectory(directory, true);
            
            juce::MessageManager::callAsync([this, filesFound]() {
                if (!isScanningActive)
                    return;
                addLogMessage("Scan complete: " + juce::String(filesFound) + " files queued for analysis");
                scanButton.setEnabled(true);
                stopButton.setEnabled(false);
                progress = 0.0;
                isScanningActive = false;
            });
        });
    });
}

void MainComponent::stopScan()
{
    isScanningActive = false;
    if (fileScanner)
    {
        fileScanner->cancelScan();
        addLogMessage("Scan cancelled");
    }
    scanButton.setEnabled(true);
    stopButton.setEnabled(false);
}

void MainComponent::updateProgress()
{
    if (analysisWorker)
    {
        int pendingJobs = analysisWorker->getPendingJobCount();
        bool isProcessing = analysisWorker->isProcessing();
        
        if (pendingJobs > 0 || isProcessing)
        {
            currentStatus = "Processing: " + juce::String(pendingJobs) + " jobs remaining";
        }
        else
        {
            currentStatus = "Ready";
        }
        
        statusLabel.setText(currentStatus, juce::dontSendNotification);
    }
}

void MainComponent::timerCallback()
{
    updateProgress();
}

void MainComponent::addLogMessage(const juce::String& message)
{
    logViewer.moveCaretToEnd();
    logViewer.insertTextAtCaret(juce::Time::getCurrentTime().toString(true, true) + " - " + message + "\n");
    logViewer.moveCaretToEnd();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Fill the background with a gradient
    g.fillAll (juce::Colour (0xff1a1a1a));
    
    // Draw a simple gradient background
    juce::ColourGradient gradient (juce::Colour (0xff2d2d2d), 
                                   getWidth() / 2.0f, 0.0f,
                                   juce::Colour (0xff1a1a1a), 
                                   getWidth() / 2.0f, getHeight(),
                                   false);
    g.setGradientFill (gradient);
    g.fillRect (getLocalBounds());
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Layout the title label at the top center
    titleLabel.setBounds (bounds.removeFromTop(60).reduced(10));
    
    // Layout the status label
    statusLabel.setBounds (bounds.removeFromTop(30).reduced(10));
    
    bounds.removeFromTop(10);  // Spacing
    
    // Button area
    auto buttonArea = bounds.removeFromTop(40).reduced(10);
    scanButton.setBounds (buttonArea.removeFromLeft(150));
    buttonArea.removeFromLeft(10);  // Spacing
    stopButton.setBounds (buttonArea.removeFromLeft(100));
    
    bounds.removeFromTop(10);  // Spacing
    
    // Progress label
    progressLabel.setBounds (bounds.removeFromTop(25).reduced(10));
    
    // Progress bar
    progressBar.setBounds (bounds.removeFromTop(25).reduced(10));
    
    bounds.removeFromTop(10);  // Spacing
    
    // Log viewer takes remaining space
    logViewer.setBounds (bounds.reduced(10));
}
