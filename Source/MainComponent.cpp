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
    : progress(0.0),
      progressBar(progress)
{
    // Set the main component size
    setSize (1400, 900);

    // Setup title label
    titleLabel.setText ("Library Manager v1.0.1", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);
    
    // Setup status label (bottom status bar)
    statusLabel.setText ("Initializing...", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (12.0f));
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (statusLabel);
    
    // Setup progress label
    progressLabel.setText ("Ready", juce::dontSendNotification);
    progressLabel.setFont (juce::Font (11.0f));
    progressLabel.setJustificationType (juce::Justification::centredRight);
    progressLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (progressLabel);
    
    // Setup search box
    searchBox.setMultiLine (false);
    searchBox.setReturnKeyStartsNewLine (false);
    searchBox.setTextToShowWhenEmpty ("Search library...", juce::Colours::grey);
    searchBox.onTextChange = [this] { onSearchTextChanged(); };
    addAndMakeVisible (searchBox);
    
    // Setup buttons
    scanButton.setButtonText ("Scan Library");
    scanButton.onClick = [this] { startScan(); };
    addAndMakeVisible (scanButton);
    
    exportButton.setButtonText ("Export to Rekordbox");
    exportButton.onClick = [this] { exportToRekordbox(); };
    addAndMakeVisible (exportButton);
    
    newPlaylistButton.setButtonText ("New Playlist");
    newPlaylistButton.onClick = [this] { createNewPlaylist(); };
    addAndMakeVisible (newPlaylistButton);
    
    // Setup progress bar
    addAndMakeVisible (progressBar);
    
    // Initialize database
    initializeDatabase();
    
    // Check if this is first run
    checkFirstRun();
    
    // Start timer for UI updates
    startTimer (500);
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (analysisWorker)
        analysisWorker->stopWorker();
    
    // Clean up UI components
    libraryTable.reset();
    playlistTree.reset();
    onboardingComponent.reset();
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
    
    if (databaseManager->initialize(dbFile))
    {
        statusLabel.setText("Database initialized", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        DBG("Database initialized successfully");
        
        // Initialize file scanner and analysis worker
        fileScanner = std::make_unique<FileScanner>(*databaseManager);
        analysisWorker = std::make_unique<AnalysisWorker>(*databaseManager);
        rekordboxExporter = std::make_unique<RekordboxExporter>(*databaseManager);
        
        // Set up worker progress callback
        analysisWorker->setProgressCallback([this](const AnalysisWorker::ProgressInfo& info) {
            juce::MessageManager::callAsync([this, info]() {
                juce::String msg = "Processing: " + info.filePath.fromLastOccurrenceOf("/", false, false);
                if (!info.errorMessage.isEmpty())
                    msg += " (Error: " + info.errorMessage + ")";
                DBG(msg);
            });
        });
        
        // Start the analysis worker
        analysisWorker->startWorker();
    }
    else
    {
        statusLabel.setText("Database initialization failed", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        DBG("Database initialization failed: " + databaseManager->getLastError());
    }
}

void MainComponent::checkFirstRun()
{
    // Check if this is first run (no tracks in database)
    if (databaseManager && databaseManager->isOpen())
    {
        auto tracks = databaseManager->getAllTracks();
        if (tracks.empty())
        {
            // Show onboarding
            showOnboarding = true;
            onboardingComponent = std::make_unique<OnboardingComponent>(*databaseManager);
            addAndMakeVisible(*onboardingComponent);
            resized();
        }
        else
        {
            // Show main interface
            showOnboarding = false;
            libraryTable = std::make_unique<LibraryTableComponent>(*databaseManager);
            playlistTree = std::make_unique<PlaylistTreeComponent>(*databaseManager);
            addAndMakeVisible(*libraryTable);
            addAndMakeVisible(*playlistTree);
            resized();
        }
    }
}

void MainComponent::startScan()
{
    // Check if onboarding is complete
    if (onboardingComponent && !onboardingComponent->isComplete())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "Complete Onboarding",
                                               "Please complete the onboarding process first.");
        return;
    }
    
    // If onboarding just completed, switch to main interface
    if (onboardingComponent && onboardingComponent->isComplete() && showOnboarding)
    {
        showOnboarding = false;
        onboardingComponent.reset();
        libraryTable = std::make_unique<LibraryTableComponent>(*databaseManager);
        playlistTree = std::make_unique<PlaylistTreeComponent>(*databaseManager);
        addAndMakeVisible(*libraryTable);
        addAndMakeVisible(*playlistTree);
        resized();
    }
    
    auto* chooser = new juce::FileChooser("Select music library folder");
    
    chooser->launchAsync(juce::FileBrowserComponent::openMode | 
                         juce::FileBrowserComponent::canSelectDirectories,
                         [this, chooser](const juce::FileChooser& fc) {
        std::unique_ptr<juce::FileChooser> deleter(const_cast<juce::FileChooser*>(chooser));
        
        auto results = fc.getResults();
        if (results.isEmpty())
            return;
        
        auto directory = results.getFirst();
        DBG("Starting scan of: " + directory.getFullPathName());
        
        scanButton.setEnabled(false);
        progress = 0.0;
        isScanningActive = true;
        
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
                DBG("Scan complete: " + juce::String(filesFound) + " files queued");
                scanButton.setEnabled(true);
                progress = 0.0;
                isScanningActive = false;
                
                // Refresh library table
                if (libraryTable)
                    libraryTable->refreshTableContent();
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
        DBG("Scan cancelled");
    }
    scanButton.setEnabled(true);
}

void MainComponent::exportToRekordbox()
{
    if (!databaseManager || !databaseManager->isOpen())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Export Failed",
                                               "Database is not available.");
        return;
    }
    
    auto* chooser = new juce::FileChooser("Export Rekordbox XML",
                                          juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                          "*.xml");
    
    chooser->launchAsync(juce::FileBrowserComponent::saveMode,
                         [this, chooser](const juce::FileChooser& fc) {
        std::unique_ptr<juce::FileChooser> deleter(const_cast<juce::FileChooser*>(chooser));
        
        auto result = fc.getResult();
        if (!result.existsAsFile() && result.getParentDirectory().exists())
        {
            // Ensure .xml extension
            if (!result.hasFileExtension(".xml"))
                result = result.withFileExtension(".xml");
            
            exportButton.setEnabled(false);
            statusLabel.setText("Exporting to Rekordbox...", juce::dontSendNotification);
            
            // Set up progress callback
            rekordboxExporter->setProgressCallback([this](double prog, const juce::String& status) {
                juce::MessageManager::callAsync([this, prog, status]() {
                    progress = prog;
                    statusLabel.setText(status, juce::dontSendNotification);
                });
            });
            
            // Export in background thread
            juce::Thread::launch([this, result]() {
                bool success = rekordboxExporter->exportToXML(result);
                
                juce::MessageManager::callAsync([this, success, result]() {
                    exportButton.setEnabled(true);
                    progress = 0.0;
                    
                    if (success)
                    {
                        statusLabel.setText("Export complete!", juce::dontSendNotification);
                        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
                        
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                               "Export Complete",
                                                               "Successfully exported to:\n" + result.getFullPathName());
                    }
                    else
                    {
                        statusLabel.setText("Export failed", juce::dontSendNotification);
                        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
                        
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                               "Export Failed",
                                                               "Error: " + rekordboxExporter->getLastError());
                    }
                });
            });
        }
    });
}

void MainComponent::createNewPlaylist()
{
    // Use async message box instead of modal loop
    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions()
            .withTitle("Create Playlist")
            .withMessage("Enter playlist name:")
            .withButton("Create")
            .withButton("Cancel")
            .withAssociatedComponent(this),
        [this](int result) {
            if (result == 1)
            {
                // In real implementation, we'd need a custom dialog with text input
                // For now, use a simplified approach
                DBG("Create playlist button clicked");
                
                // Create a default playlist for demonstration
                DatabaseManager::VirtualFolder playlist;
                playlist.name = "New Playlist " + juce::String(juce::Time::getCurrentTime().toMilliseconds());
                playlist.description = "";
                playlist.dateCreated = juce::Time::getCurrentTime();
                
                int64_t playlistId;
                if (databaseManager->addVirtualFolder(playlist, playlistId))
                {
                    DBG("Created playlist: " + playlist.name);
                    if (playlistTree)
                        playlistTree->refreshTree();
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                           "Error",
                                                           "Failed to create playlist");
                }
            }
        });
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
    
    // Check if onboarding is complete and switch to main interface
    if (onboardingComponent && onboardingComponent->isComplete() && showOnboarding)
    {
        showOnboarding = false;
        onboardingComponent.reset();
        libraryTable = std::make_unique<LibraryTableComponent>(*databaseManager);
        playlistTree = std::make_unique<PlaylistTreeComponent>(*databaseManager);
        addAndMakeVisible(*libraryTable);
        addAndMakeVisible(*playlistTree);
        resized();
    }
}

void MainComponent::onSearchTextChanged()
{
    if (libraryTable)
    {
        libraryTable->setSearchFilter(searchBox.getText());
    }
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Fill the background with a gradient
    g.fillAll (juce::Colour (0xff1a1a1a));
    
    // Draw a simple gradient background
    juce::ColourGradient gradient (juce::Colour (0xff2d2d2d), 
                                   static_cast<float>(getWidth()) / 2.0f, 0.0f,
                                   juce::Colour (0xff1a1a1a), 
                                   static_cast<float>(getWidth()) / 2.0f, static_cast<float>(getHeight()),
                                   false);
    g.setGradientFill (gradient);
    g.fillRect (getLocalBounds());
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    if (showOnboarding && onboardingComponent)
    {
        // Onboarding mode - center the onboarding component
        auto onboardingBounds = bounds.reduced(50);
        onboardingComponent->setBounds(onboardingBounds);
    }
    else
    {
        // Main interface mode
        // Top bar with title and controls
        auto topBar = bounds.removeFromTop(50);
        titleLabel.setBounds(topBar.removeFromLeft(250).reduced(10, 10));
        
        // Buttons on the right
        auto buttonArea = topBar.removeFromRight(500).reduced(5);
        newPlaylistButton.setBounds(buttonArea.removeFromRight(120));
        buttonArea.removeFromRight(5);
        exportButton.setBounds(buttonArea.removeFromRight(160));
        buttonArea.removeFromRight(5);
        scanButton.setBounds(buttonArea.removeFromRight(120));
        
        // Search box
        searchBox.setBounds(topBar.reduced(5));
        
        // Status bar at bottom
        auto statusBar = bounds.removeFromBottom(25);
        statusLabel.setBounds(statusBar.removeFromLeft(getWidth() * 2 / 3).reduced(5, 2));
        progressLabel.setBounds(statusBar.reduced(5, 2));
        
        // Progress bar above status bar
        progressBar.setBounds(bounds.removeFromBottom(4));
        
        // Main content area
        auto contentArea = bounds.reduced(5);
        
        // Left side: playlist tree (30% width)
        if (playlistTree)
        {
            auto leftPanel = contentArea.removeFromLeft(getWidth() * 30 / 100);
            playlistTree->setBounds(leftPanel);
            contentArea.removeFromLeft(5); // Spacing
        }
        
        // Right side: library table (remaining width)
        if (libraryTable)
        {
            libraryTable->setBounds(contentArea);
        }
    }
}
