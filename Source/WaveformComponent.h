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
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <functional>

//==============================================================================
/**
    WaveformComponent displays audio waveforms for tracks.
    
    Features:
    - Thumbnail-style waveform rendering
    - Zoom and pan controls
    - Cue point overlay support
    - Click to seek position
*/
class WaveformComponent : public juce::Component,
                          private juce::Timer
{
public:
    WaveformComponent();
    ~WaveformComponent() override;

    /**
     * Load an audio file and generate its waveform.
     * @param audioFile The audio file to visualize
     * @return true if loading was successful
     */
    bool loadAudioFile(const juce::File& audioFile);
    
    /**
     * Clear the current waveform.
     */
    void clear();
    
    /**
     * Set the current playback position (for highlighting).
     * @param position Position in seconds
     */
    void setPlaybackPosition(double position);
    
    /**
     * Get the current playback position.
     */
    double getPlaybackPosition() const { return currentPosition; }
    
    /**
     * Set zoom level (1.0 = 100%, 2.0 = 200%, etc.)
     */
    void setZoom(double newZoom);
    
    /**
     * Get current zoom level.
     */
    double getZoom() const { return zoomLevel; }
    
    /**
     * Set markers/cue points to display on the waveform.
     * @param positions Vector of positions in seconds
     */
    void setCuePoints(const std::vector<double>& positions);
    
    /**
     * Callback when user clicks on waveform to seek.
     */
    std::function<void(double)> onSeek;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
private:
    void timerCallback() override;
    void generateWaveform();
    double positionToX(double position) const;
    double xToPosition(int x) const;
    
    juce::File currentFile;
    std::unique_ptr<juce::AudioFormatReader> audioReader;
    juce::AudioFormatManager formatManager;
    
    // Waveform data (min/max pairs for each time slice)
    struct WaveformData
    {
        float min;
        float max;
    };
    std::vector<WaveformData> waveformData;
    
    double duration = 0.0;
    double currentPosition = 0.0;
    double zoomLevel = 1.0;
    std::vector<double> cuePoints;
    
    bool isLoaded = false;
    juce::String lastError;
    // Horizontal scroll offset for waveform drawing / future scrolling support
    double scrollOffset = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};
