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

#include "AudioPreviewComponent.h"

//==============================================================================
AudioPreviewComponent::AudioPreviewComponent()
{
    formatManager.registerBasicFormats();
    setupAudioDevice();
    
    // Setup waveform
    addAndMakeVisible(waveform);
    waveform.onSeek = [this](double position)
    {
        seekTo(position);
    };
    
    // Setup playback controls
    addAndMakeVisible(playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this]() { play(); };
    
    addAndMakeVisible(pauseButton);
    pauseButton.setButtonText("Pause");
    pauseButton.onClick = [this]() { pause(); };
    
    addAndMakeVisible(stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this]() { stop(); };
    
    // Setup volume control
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.7);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    volumeSlider.onValueChange = [this]()
    {
        setVolume(static_cast<float>(volumeSlider.getValue()));
    };
    
    // Setup time label
    addAndMakeVisible(timeLabel);
    timeLabel.setText("00:00 / 00:00", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    
    // Start timer for updating UI
    startTimer(50); // 20 FPS update rate
}

AudioPreviewComponent::~AudioPreviewComponent()
{
    stopTimer();
    stop();
    transportSource.setSource(nullptr);
    audioSourcePlayer.setSource(nullptr);
}

//==============================================================================
void AudioPreviewComponent::setupAudioDevice()
{
    // Initialize audio device with default settings
    juce::String error = deviceManager.initialise(
        0,     // no input channels
        2,     // stereo output
        nullptr,
        true,  // select default device on failure
        juce::String(), // Use default device type (empty string)
        nullptr
    );
    
    if (error.isNotEmpty())
    {
        DBG("[AudioPreviewComponent] Error initializing audio device: " << error);
    }
    
    // Connect audio source player to transport source
    audioSourcePlayer.setSource(&transportSource);
    deviceManager.addAudioCallback(&audioSourcePlayer);
}

bool AudioPreviewComponent::loadAudioFile(const juce::File& audioFile)
{
    if (!audioFile.existsAsFile())
    {
        DBG("[AudioPreviewComponent] File does not exist: " << audioFile.getFullPathName());
        return false;
    }
    
    // Stop any current playback
    stop();
    
    // Create reader for the file
    auto* reader = formatManager.createReaderFor(audioFile);
    
    if (reader == nullptr)
    {
        DBG("[AudioPreviewComponent] Could not read audio file: " << audioFile.getFullPathName());
        return false;
    }
    
    // Create reader source
    readerSource.reset(new juce::AudioFormatReaderSource(reader, true));
    
    // Connect to transport source
    transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
    
    // Load waveform
    waveform.loadAudioFile(audioFile);
    
    currentFile = audioFile;
    
    DBG("[AudioPreviewComponent] Loaded audio file: " << audioFile.getFileName());
    return true;
}

void AudioPreviewComponent::play()
{
    if (!playing)
    {
        transportSource.start();
        playing = true;
        updateTransportState();
    }
}

void AudioPreviewComponent::pause()
{
    if (playing)
    {
        transportSource.stop();
        playing = false;
        updateTransportState();
    }
}

void AudioPreviewComponent::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);
    playing = false;
    updateTransportState();
}

void AudioPreviewComponent::seekTo(double position)
{
    transportSource.setPosition(position);
}

void AudioPreviewComponent::setVolume(float volume)
{
    transportSource.setGain(juce::jlimit(0.0f, 1.0f, volume));
}

double AudioPreviewComponent::getCurrentPosition() const
{
    return transportSource.getCurrentPosition();
}

double AudioPreviewComponent::getDuration() const
{
    return transportSource.getLengthInSeconds();
}

void AudioPreviewComponent::setCuePoints(const std::vector<double>& positions)
{
    waveform.setCuePoints(positions);
}

//==============================================================================
void AudioPreviewComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioPreviewComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Waveform takes most of the space
    auto waveformBounds = bounds.removeFromTop(bounds.getHeight() - 80);
    waveform.setBounds(waveformBounds);
    
    bounds.removeFromTop(10); // Spacing
    
    // Controls at the bottom
    auto controlsBounds = bounds.removeFromTop(30);
    
    const int buttonWidth = 80;
    playButton.setBounds(controlsBounds.removeFromLeft(buttonWidth));
    controlsBounds.removeFromLeft(5);
    pauseButton.setBounds(controlsBounds.removeFromLeft(buttonWidth));
    controlsBounds.removeFromLeft(5);
    stopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth));
    
    controlsBounds.removeFromLeft(20);
    
    // Volume slider
    volumeSlider.setBounds(controlsBounds.removeFromLeft(200));
    
    // Time label
    bounds.removeFromTop(10);
    timeLabel.setBounds(bounds.removeFromTop(20));
}

void AudioPreviewComponent::timerCallback()
{
    // Update waveform position
    if (playing)
    {
        waveform.setPlaybackPosition(getCurrentPosition());
    }
    
    // Update time label
    const double currentPos = getCurrentPosition();
    const double totalDuration = getDuration();
    
    const int currentMinutes = static_cast<int>(currentPos / 60.0);
    const int currentSeconds = static_cast<int>(currentPos) % 60;
    const int totalMinutes = static_cast<int>(totalDuration / 60.0);
    const int totalSeconds = static_cast<int>(totalDuration) % 60;
    
    juce::String timeText = juce::String::formatted("%02d:%02d / %02d:%02d",
                                                     currentMinutes, currentSeconds,
                                                     totalMinutes, totalSeconds);
    timeLabel.setText(timeText, juce::dontSendNotification);
    
    // Check if playback finished
    if (playing && !transportSource.isPlaying())
    {
        stop();
    }
}

void AudioPreviewComponent::updateTransportState()
{
    playButton.setEnabled(!playing);
    pauseButton.setEnabled(playing);
}
