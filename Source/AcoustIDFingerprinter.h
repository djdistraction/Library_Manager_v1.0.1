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

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
/**
    AcoustIDFingerprinter provides functionality to generate acoustic fingerprints
    using the Chromaprint library. These fingerprints can be used to identify
    tracks via the AcoustID/MusicBrainz service or detect duplicates.
*/
class AcoustIDFingerprinter
{
public:
    //==============================================================================
    AcoustIDFingerprinter();
    ~AcoustIDFingerprinter();
    
    /**
     * Generate an AcoustID fingerprint for an audio file.
     * @param audioFile The audio file to fingerprint
     * @param fingerprint Output parameter that will contain the base64-encoded fingerprint
     * @param duration Output parameter that will contain the duration in seconds
     * @return True if fingerprinting was successful, false otherwise
     */
    bool generateFingerprint(const juce::File& audioFile, 
                            juce::String& fingerprint,
                            int& duration);
    
    /**
     * Get the last error message.
     */
    juce::String getLastError() const { return lastError; }
    
private:
    //==============================================================================
    juce::String lastError;
    
    // Helper to read audio data and pass to chromaprint
    bool processAudioFile(const juce::File& audioFile,
                         juce::String& fingerprint,
                         int& duration);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcoustIDFingerprinter)
};
