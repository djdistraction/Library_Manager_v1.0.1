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

#include "WaveformComponent.h"

//==============================================================================
WaveformComponent::WaveformComponent()
{
    formatManager.registerBasicFormats();
    startTimer(50); // Update position indicator at 20 FPS
}

WaveformComponent::~WaveformComponent()
{
    stopTimer();
}

//==============================================================================
bool WaveformComponent::loadAudioFile(const juce::File& audioFile)
{
    if (!audioFile.existsAsFile())
    {
        lastError = "File does not exist";
        return false;
    }
    
    // Clear existing data
    clear();
    
    currentFile = audioFile;
    audioReader.reset(formatManager.createReaderFor(audioFile));
    
    if (audioReader == nullptr)
    {
        lastError = "Could not read audio file";
        return false;
    }
    
    duration = audioReader->lengthInSamples / audioReader->sampleRate;
    
    // Generate waveform in background
    generateWaveform();
    
    isLoaded = true;
    repaint();
    
    return true;
}

void WaveformComponent::clear()
{
    audioReader.reset();
    waveformData.clear();
    currentPosition = 0.0;
    duration = 0.0;
    isLoaded = false;
    cuePoints.clear();
    zoomLevel = 1.0;
    scrollOffset = 0.0;
    repaint();
}

void WaveformComponent::setPlaybackPosition(double position)
{
    currentPosition = position;
    repaint();
}

void WaveformComponent::setZoom(double newZoom)
{
    zoomLevel = juce::jlimit(0.1, 10.0, newZoom);
    repaint();
}

void WaveformComponent::setCuePoints(const std::vector<double>& positions)
{
    cuePoints = positions;
    repaint();
}

//==============================================================================
void WaveformComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    
    if (!isLoaded || waveformData.empty())
    {
        g.setColour(juce::Colours::white);
        g.drawText("No waveform loaded", getLocalBounds(), juce::Justification::centred);
        return;
    }
    
    auto bounds = getLocalBounds().reduced(10);
    
    // Draw center line
    g.setColour(juce::Colours::grey);
    g.drawHorizontalLine(bounds.getCentreY(), bounds.getX(), bounds.getRight());
    
    // Calculate visible range
    const int numSamples = static_cast<int>(waveformData.size());
    const double samplesPerPixel = (numSamples / zoomLevel) / bounds.getWidth();
    
    // Draw waveform
    g.setColour(juce::Colours::lightblue);
    
    for (int x = bounds.getX(); x < bounds.getRight(); ++x)
    {
        const int sampleIndex = static_cast<int>((x - bounds.getX()) * samplesPerPixel);
        
        if (sampleIndex >= 0 && sampleIndex < numSamples)
        {
            const auto& data = waveformData[sampleIndex];
            
            const float centerY = static_cast<float>(bounds.getCentreY());
            const float halfHeight = bounds.getHeight() / 2.0f;
            
            const float minY = centerY + data.min * halfHeight;
            const float maxY = centerY + data.max * halfHeight;
            
            g.drawVerticalLine(x, minY, maxY);
        }
    }
    
    // Draw cue points
    g.setColour(juce::Colours::red);
    for (const auto& cuePos : cuePoints)
    {
        const int x = static_cast<int>(positionToX(cuePos));
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
            g.fillEllipse(static_cast<float>(x - 3), static_cast<float>(bounds.getY()), 6.0f, 6.0f);
        }
    }
    
    // Draw playback position
    if (currentPosition >= 0.0 && currentPosition <= duration)
    {
        g.setColour(juce::Colours::yellow);
        const int x = static_cast<int>(positionToX(currentPosition));
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
        }
    }
}

void WaveformComponent::resized()
{
    // Nothing to do here for now
}

void WaveformComponent::mouseDown(const juce::MouseEvent& event)
{
    if (!isLoaded)
        return;
    
    const double clickedPosition = xToPosition(event.x);
    
    if (clickedPosition >= 0.0 && clickedPosition <= duration && onSeek)
    {
        onSeek(clickedPosition);
    }
}

void WaveformComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Zoom with mouse wheel
    const double zoomFactor = 1.0 + (wheel.deltaY * 0.5);
    setZoom(zoomLevel * zoomFactor);
}

//==============================================================================
void WaveformComponent::timerCallback()
{
    // Timer used for updating position indicator if needed
    // Currently just triggers repaint when playing
}

void WaveformComponent::generateWaveform()
{
    if (audioReader == nullptr)
        return;
    
    // Generate approximately 1000 samples for the waveform
    const int targetSamples = 1000;
    const int64_t samplesPerBlock = audioReader->lengthInSamples / targetSamples;
    
    waveformData.clear();
    waveformData.reserve(targetSamples);
    
    const int numChannels = static_cast<int>(audioReader->numChannels);
    juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(samplesPerBlock));
    
    for (int i = 0; i < targetSamples; ++i)
    {
        const int64_t startSample = i * samplesPerBlock;
        
        if (startSample < audioReader->lengthInSamples)
        {
            audioReader->read(&buffer, 0, static_cast<int>(samplesPerBlock), startSample, true, true);
            
            float minVal = 0.0f;
            float maxVal = 0.0f;
            
            // Find min/max in this block
            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = buffer.getReadPointer(channel);
                for (int sample = 0; sample < samplesPerBlock && sample < buffer.getNumSamples(); ++sample)
                {
                    const float value = channelData[sample];
                    minVal = juce::jmin(minVal, value);
                    maxVal = juce::jmax(maxVal, value);
                }
            }
            
            waveformData.push_back({minVal, maxVal});
        }
    }
}

double WaveformComponent::positionToX(double position) const
{
    if (duration <= 0.0)
        return 0.0;
    
    const auto bounds = getLocalBounds().reduced(10);
    const double ratio = position / duration;
    return bounds.getX() + (ratio * bounds.getWidth() * zoomLevel);
}

double WaveformComponent::xToPosition(int x) const
{
    if (duration <= 0.0)
        return 0.0;
    
    const auto bounds = getLocalBounds().reduced(10);
    const double ratio = (x - bounds.getX()) / (bounds.getWidth() * zoomLevel);
    return ratio * duration;
}
