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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    ToastNotification displays temporary popup messages to the user.
    Messages automatically fade out after a few seconds.
*/
class ToastNotification : public juce::Component,
                          private juce::Timer
{
public:
    enum class Type
    {
        Info,
        Success,
        Warning,
        Error
    };
    
    ToastNotification();
    ~ToastNotification() override;
    
    void showMessage(const juce::String& message, Type type = Type::Info, int durationMs = 3000);
    void hide();
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    void timerCallback() override;
    void updatePosition();
    
    juce::String currentMessage;
    Type currentType = Type::Info;
    float opacity = 1.0f;
    bool fadingOut = false;
    
    static constexpr int toastWidth = 400;
    static constexpr int toastHeight = 60;
    static constexpr int marginFromBottom = 80;
    static constexpr int fadeOutDuration = 500; // milliseconds
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToastNotification)
};
