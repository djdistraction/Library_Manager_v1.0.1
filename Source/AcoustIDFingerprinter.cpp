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

#include "AcoustIDFingerprinter.h"
#include <chromaprint.h>

//==============================================================================
AcoustIDFingerprinter::AcoustIDFingerprinter()
{
}

AcoustIDFingerprinter::~AcoustIDFingerprinter()
{
}

//==============================================================================
bool AcoustIDFingerprinter::generateFingerprint(const juce::File& audioFile,
                                               juce::String& fingerprint,
                                               int& duration)
{
    if (!audioFile.existsAsFile())
    {
        lastError = "File does not exist: " + audioFile.getFullPathName();
        DBG("[AcoustIDFingerprinter] " << lastError);
        return false;
    }
    
    return processAudioFile(audioFile, fingerprint, duration);
}

bool AcoustIDFingerprinter::processAudioFile(const juce::File& audioFile,
                                            juce::String& fingerprint,
                                            int& duration)
{
    // Create audio format manager and register formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // Try to read the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    
    if (reader == nullptr)
    {
        lastError = "Could not read audio file: " + audioFile.getFileName();
        DBG("[AcoustIDFingerprinter] " << lastError);
        return false;
    }
    
    // Create chromaprint context
    ChromaprintContext* ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
    if (ctx == nullptr)
    {
        lastError = "Failed to create Chromaprint context";
        DBG("[AcoustIDFingerprinter] " << lastError);
        return false;
    }
    
    // Initialize chromaprint with sample rate and number of channels
    int sampleRate = static_cast<int>(reader->sampleRate);
    int numChannels = static_cast<int>(reader->numChannels);
    
    if (!chromaprint_start(ctx, sampleRate, numChannels))
    {
        lastError = "Failed to start Chromaprint";
        DBG("[AcoustIDFingerprinter] " << lastError);
        chromaprint_free(ctx);
        return false;
    }
    
    // Read audio data in chunks and feed to chromaprint
    const int chunkSize = 4096;
    juce::AudioBuffer<float> buffer(numChannels, chunkSize);
    
    // Chromaprint expects 16-bit PCM, so we'll convert
    std::vector<int16_t> int16Buffer(chunkSize * numChannels);
    
    int64_t totalSamplesRead = 0;
    int64_t maxSamplesToRead = reader->lengthInSamples;
    
    // Limit to first 2 minutes for performance (120 seconds)
    const int64_t maxSamplesForFingerprint = sampleRate * 120;
    if (maxSamplesToRead > maxSamplesForFingerprint)
        maxSamplesToRead = maxSamplesForFingerprint;
    
    while (totalSamplesRead < maxSamplesToRead)
    {
        int samplesToRead = static_cast<int>(juce::jmin((int64_t)chunkSize, maxSamplesToRead - totalSamplesRead));
        
        reader->read(&buffer, 0, samplesToRead, totalSamplesRead, true, true);
        
        // Convert float samples to 16-bit integers
        for (int sample = 0; sample < samplesToRead; ++sample)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float floatSample = buffer.getSample(channel, sample);
                // Clamp to [-1, 1] and convert to 16-bit
                floatSample = juce::jlimit(-1.0f, 1.0f, floatSample);
                int16Buffer[sample * numChannels + channel] = static_cast<int16_t>(floatSample * 32767.0f);
            }
        }
        
        // Feed data to chromaprint
        if (!chromaprint_feed(ctx, int16Buffer.data(), samplesToRead * numChannels))
        {
            lastError = "Failed to feed data to Chromaprint";
            DBG("[AcoustIDFingerprinter] " << lastError);
            chromaprint_free(ctx);
            return false;
        }
        
        totalSamplesRead += samplesToRead;
    }
    
    // Finish and get the fingerprint
    if (!chromaprint_finish(ctx))
    {
        lastError = "Failed to finish Chromaprint processing";
        DBG("[AcoustIDFingerprinter] " << lastError);
        chromaprint_free(ctx);
        return false;
    }
    
    // Get the fingerprint as a string
    char* fingerprintCStr = nullptr;
    if (!chromaprint_get_fingerprint(ctx, &fingerprintCStr))
    {
        lastError = "Failed to get fingerprint from Chromaprint";
        DBG("[AcoustIDFingerprinter] " << lastError);
        chromaprint_free(ctx);
        return false;
    }
    
    // Convert to JUCE string
    fingerprint = juce::String(fingerprintCStr);
    chromaprint_dealloc(fingerprintCStr);
    
    // Calculate duration
    duration = static_cast<int>(reader->lengthInSamples / reader->sampleRate);
    
    // Clean up
    chromaprint_free(ctx);
    
    DBG("[AcoustIDFingerprinter] Successfully generated fingerprint for: " << audioFile.getFileName());
    DBG("[AcoustIDFingerprinter] Duration: " << duration << " seconds");
    DBG("[AcoustIDFingerprinter] Fingerprint length: " << fingerprint.length() << " characters");
    
    return true;
}
