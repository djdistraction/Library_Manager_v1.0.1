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
}

MainComponent::~MainComponent()
{
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
}
