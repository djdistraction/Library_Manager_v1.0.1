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

#include "ToastNotification.h"

//==============================================================================
ToastNotification::ToastNotification()
{
    setAlwaysOnTop(true);
    setVisible(false);
}

ToastNotification::~ToastNotification()
{
    stopTimer();
}

void ToastNotification::showMessage(const juce::String& message, Type type, int durationMs)
{
    stopTimer();
    
    currentMessage = message;
    currentType = type;
    opacity = 1.0f;
    fadingOut = false;
    
    updatePosition();
    setVisible(true);
    repaint();
    
    // Start timer to hide the toast after specified duration
    startTimer(durationMs);
}

void ToastNotification::hide()
{
    stopTimer();
    setVisible(false);
}

void ToastNotification::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Choose color based on type
    juce::Colour bgColor;
    juce::Colour textColor = juce::Colours::white;
    
    switch (currentType)
    {
        case Type::Info:
            bgColor = juce::Colour(0xff2d5a8c);
            break;
        case Type::Success:
            bgColor = juce::Colour(0xff2d8c3e);
            break;
        case Type::Warning:
            bgColor = juce::Colour(0xff8c6d2d);
            break;
        case Type::Error:
            bgColor = juce::Colour(0xff8c2d2d);
            break;
    }
    
    // Apply opacity
    bgColor = bgColor.withAlpha(opacity);
    textColor = textColor.withAlpha(opacity);
    
    // Draw rounded rectangle background
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw border
    g.setColour(bgColor.brighter(0.3f).withAlpha(opacity));
    g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
    
    // Draw text
    g.setColour(textColor);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText(currentMessage, bounds.reduced(15, 10), juce::Justification::centredLeft, true);
}

void ToastNotification::resized()
{
    updatePosition();
}

void ToastNotification::timerCallback()
{
    stopTimer();
    fadingOut = true;
    
    // Start fade out animation
    juce::Timer::callAfterDelay(fadeOutDuration, [this]()
    {
        hide();
    });
    
    // Animate fade out
    for (int i = 0; i < 10; ++i)
    {
        juce::Timer::callAfterDelay(i * (fadeOutDuration / 10), [this, i]()
        {
            opacity = 1.0f - (i / 10.0f);
            repaint();
        });
    }
}

void ToastNotification::updatePosition()
{
    if (auto* parent = getParentComponent())
    {
        int x = (parent->getWidth() - toastWidth) / 2;
        int y = parent->getHeight() - toastHeight - marginFromBottom;
        setBounds(x, y, toastWidth, toastHeight);
    }
}
