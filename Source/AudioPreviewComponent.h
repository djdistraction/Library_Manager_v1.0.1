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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "WaveformComponent.h"

//==============================================================================
/**
    AudioPreviewComponent provides audio playback controls with waveform visualization.
    
    Features:
    - Play/pause/stop controls
    - Seek to position
    - Volume control
    - Integrated waveform display
    - Cue point jumping
*/
class AudioPreviewComponent : public juce::Component,
                              private juce::Timer
{
public:
    AudioPreviewComponent();
    ~AudioPreviewComponent() override;

    /**
     * Load an audio file for preview.
     * @param audioFile The audio file to load
     * @return true if loading was successful
     */
    bool loadAudioFile(const juce::File& audioFile);
    
    /**
     * Start playback.
     */
    void play();
    
    /**
     * Pause playback.
     */
    void pause();
    
    /**
     * Stop playback and reset position.
     */
    void stop();
    
    /**
     * Seek to a specific position.
     * @param position Position in seconds
     */
    void seekTo(double position);
    
    /**
     * Set volume level.
     * @param volume Volume level (0.0 to 1.0)
     */
    void setVolume(float volume);
    
    /**
     * Check if currently playing.
     */
    bool isPlaying() const { return playing; }
    
    /**
     * Get current playback position in seconds.
     */
    double getCurrentPosition() const;
    
    /**
     * Get total duration in seconds.
     */
    double getDuration() const;
    
    /**
     * Set cue points to display on waveform.
     */
    void setCuePoints(const std::vector<double>& positions);

    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    void timerCallback() override;
    void updateTransportState();
    void setupAudioDevice();
    
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    
    WaveformComponent waveform;
    
    juce::TextButton playButton;
    juce::TextButton pauseButton;
    juce::TextButton stopButton;
    juce::Slider volumeSlider;
    juce::Label timeLabel;
    
    bool playing = false;
    juce::File currentFile;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPreviewComponent)
};
