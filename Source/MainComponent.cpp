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
    
    // Initialize database
    initializeDatabase();
}

MainComponent::~MainComponent()
{
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
        statusLabel.setText("Database initialized successfully", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        DBG("Database initialized successfully");
    }
    else
    {
        statusLabel.setText("Database initialization failed: " + databaseManager->getLastError(), 
                          juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        DBG("Database initialization failed: " + databaseManager->getLastError());
    }
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
    // Layout the title label at the top center
    titleLabel.setBounds (0, 50, getWidth(), 50);
    
    // Layout the status label below the title
    statusLabel.setBounds (0, 110, getWidth(), 30);
}
