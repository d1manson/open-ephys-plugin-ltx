/*
	------------------------------------------------------------------

	This file is part of the Open Ephys GUI
	Copyright (C) 2022 Open Ephys

	------------------------------------------------------------------

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "LTXRecordEnginePlugin.h"

namespace LTX {
    constexpr size_t headerMarkerSize = 14;
    constexpr char headerMarkerFormat[] = "%-14d"; // this 14 must match the 14 above.
    constexpr char headerMarkerNumSpikes[] = "###NUM_SPIKES#";
    constexpr char headerMarkerDuration[] = "####DURATION##";
    constexpr char headerMarkerNumEEGSamples[] = "###NUM_SAMPS##";
    static_assert(std::char_traits<char>::length(headerMarkerNumSpikes) == headerMarkerSize, "header marker length should be 14");
    static_assert(std::char_traits<char>::length(headerMarkerDuration) == headerMarkerSize, "header marker length should be 14");
    static_assert(std::char_traits<char>::length(headerMarkerNumEEGSamples) == headerMarkerSize, "header marker length should be 14");
   
    constexpr int eegInputSampRate = 30000;
    constexpr int eegOutputSampRate = 1000;
    constexpr int eegDownsampleBy = eegInputSampRate / eegOutputSampRate;


    int32_t swapEndianness(uint32_t value) {
        return ((value & 0xFF000000) >> 24) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0x0000FF00) << 8) |
            ((value & 0x000000FF) << 24);
    }


    RecordEnginePlugin::RecordEnginePlugin()
    {
       
    }

    RecordEnginePlugin::~RecordEnginePlugin()
    {

    }


    RecordEngineManager* RecordEnginePlugin::getEngineManager()
    {
        RecordEngineManager* man = new RecordEngineManager("LTX", "LTX Format",
            &(engineFactory<RecordEnginePlugin>));
        return man;
    }


    void RecordEnginePlugin::openFiles(File rootFolder, int experimentNumber, int recordingNumber)
    {
        mode = getNumRecordedSpikeChannels() > 0 ? SPIKES_AND_SET : EEG_ONLY; // i can't see how to configure a record node in the openephys interface to *not* recieve continuous data, so we do this hacky thing

        String basePath(rootFolder.getParentDirectory().getFullPathName());

        startTime = std::chrono::high_resolution_clock::now(); // used to calculate duration (not sure if OpenEphys offers an alternative)

        std::time_t t = std::time(nullptr);
        std::tm start_tm = *std::localtime(&t);
        
        if (mode == SPIKES_AND_SET) {
            // set file
            {
                String fullPath = basePath + ".set";
                LOGC("OPENING FILE: ", fullPath);

                std::stringstream setHeader;
                setHeader << "trial_date " << std::put_time(&start_tm, "%A, %d %b %Y")
                    << "\r\ntrial_time " << std::put_time(&start_tm, "%H:%M")
                    << "\r\ncreated_by open-ephys-ltx-plugin"
                    << "\r\nduration " << headerMarkerDuration;
                std::string setHeaderStr = setHeader.str();
                setHeaderOffsetDuration = setHeaderStr.find(headerMarkerDuration);

                diskWriteLock.enter();
                FILE* setFile_ = fopen(fullPath.toUTF8(), "wb");
                fwrite(setHeaderStr.c_str(), sizeof(char), setHeaderStr.size(), setFile_);
                diskWriteLock.exit();
                setFile = setFile_;
            }

            // the tet files all have the same header, that is until we write their num spikes at the end.
            std::stringstream tetHeader;
            tetHeader << "trial_date " << std::put_time(&start_tm, "%A, %d %b %Y")
                << "\r\ntrial_time " << std::put_time(&start_tm, "%H:%M")
                << "\r\ncreated_by open-ephys-plugin-ltx"
                << "\r\nduration " << headerMarkerDuration
                << "\r\nnum_chans 4"
                << "\r\ntimebase 96000 hz" // probably not
                << "\r\nbytes_per_timestamp 4"
                << "\r\nsamples_per_spike 50" // actually it's not, but we zero-pad when writing to the file to keep it at 50
                << "\r\nbytes_per_sample 1"
                << "\r\nspike_format t,ch1,t,ch2,t,ch3,t,ch4"
                << "\r\nnum_spikes " << headerMarkerNumSpikes
                << "\r\ndata_start";
            std::string tetHeaderStr = tetHeader.str();
            tetHeaderOffsetNumSpikes = tetHeaderStr.find(headerMarkerNumSpikes);
            tetHeaderOffsetDuration = tetHeaderStr.find(headerMarkerDuration);


            for (int i = 0; i < getNumRecordedSpikeChannels(); i++) {
                String fullPath = basePath + "." + String(i + 1); // use 1-based indexing for the output file names
                LOGC("OPENING FILE: ", fullPath);

                diskWriteLock.enter();
                FILE* tetFile_ = fopen(fullPath.toUTF8(), "wb");
                fwrite(tetHeaderStr.c_str(), 1, tetHeaderStr.size(), tetFile_);
                diskWriteLock.exit();

                tetFiles.add(tetFile_);
                tetSpikeCount.add(0);
            }
        } else { // mode: EEG_ONLY

            // the eeg files all have the same header, that is until we write their num samples at the end (which probably should be the same as it happens)
            std::stringstream eegHeader;
            eegHeader << "trial_date " << std::put_time(&start_tm, "%A, %d %b %Y")
                << "\r\ntrial_time " << std::put_time(&start_tm, "%H:%M")
                << "\r\ncreated_by open-ephys-plugin-ltx"
                << "\r\nduration " << headerMarkerDuration
                << "\r\nnum_chans 1"
                << "\r\nsample_rate " << eegOutputSampRate << " hz"
                //<< "\r\nEEG_samples_per_position ???"
                << "\r\nnum_EEG_samples " << headerMarkerNumEEGSamples
                << "\r\ndata_start";
            std::string eegHeaderStr = eegHeader.str();
            eegHeaderOffsetNumEEGSamples = eegHeaderStr.find(headerMarkerNumEEGSamples);
            eegHeaderOffsetDuration = eegHeaderStr.find(headerMarkerDuration);

            for (int i = 0; i < getNumRecordedContinuousChannels(); i++) {
                String fullPath = basePath + ".eeg" + (i == 0 ? "" : String(i + 1)); // .eeg, .eeg2, eeg3, ...
                LOGC("OPENING FILE: ", fullPath);

                diskWriteLock.enter();
                FILE* eegFile_ = fopen(fullPath.toUTF8(), "wb");
                fwrite(eegHeaderStr.c_str(), 1, eegHeaderStr.size(), eegFile_);
                diskWriteLock.exit();

                eegFiles.add(eegFile_);
                eegFullSampCount.add(0);
            }
        }

    }

    void RecordEnginePlugin::closeFiles()
    {
        // compute duration
        char durationStr[headerMarkerSize + 1];
        std::chrono::seconds durationSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - startTime);
        sprintf(durationStr, headerMarkerFormat, durationSeconds);
        LOGC("Finalising headers, with durationStr = ", durationStr);

        if (mode == SPIKES_AND_SET) {
            // finalise set file
            diskWriteLock.enter();
            fseek(setFile, setHeaderOffsetDuration, SEEK_SET);
            fwrite(durationStr, 1, headerMarkerSize, setFile);
            fclose(setFile);


            // finalise tet files
            for (int i = 0; i < tetFiles.size(); i++) {
                constexpr char endToken[] = "\r\ndata_end";
                fwrite(endToken, 1, sizeof(endToken) - 1 /* ignore \0 */, tetFiles[i]);


                // write duration
                fseek(tetFiles[i], tetHeaderOffsetDuration, SEEK_SET);
                fwrite(durationStr, 1, headerMarkerSize, tetFiles[i]);

                // write num spikes
                char nSpikesStr[headerMarkerSize + 1];
                sprintf(nSpikesStr, headerMarkerFormat, tetSpikeCount[i]);
                fseek(tetFiles[i], tetHeaderOffsetNumSpikes, SEEK_SET);
                fwrite(nSpikesStr, 1, headerMarkerSize, tetFiles[i]);

                fclose(tetFiles[i]);

            }
        }
        else { // mode: EEG_ONLY

            // finalise eeg files
            for (int i = 0; i < eegFiles.size(); i++) {
                constexpr char endToken[] = "\r\ndata_end";
                fwrite(endToken, 1, sizeof(endToken) - 1 /* ignore \0 */, eegFiles[i]);

                // write duration
                fseek(eegFiles[i], eegHeaderOffsetDuration, SEEK_SET);
                fwrite(durationStr, 1, headerMarkerSize, eegFiles[i]);

                // write num EEG samples
                char nEEGSampsStr[headerMarkerSize + 1];
                sprintf(nEEGSampsStr, headerMarkerFormat, eegFullSampCount[i]/eegDownsampleBy);
                fseek(eegFiles[i], eegHeaderOffsetNumEEGSamples, SEEK_SET);
                fwrite(nEEGSampsStr, 1, headerMarkerSize, eegFiles[i]);

                fclose(eegFiles[i]);
            }
        }
        diskWriteLock.exit();

        LOGC("Completed writing files.")

    }

    void RecordEnginePlugin::writeContinuousData(int writeChannel,
                                                   int realChannel,
                                                   const float* dataBuffer,
                                                   const double* ftsBuffer,
                                                   int size)
    {
        if (mode != EEG_ONLY) {
            return;
        }


        constexpr int outputBufferSize = 64; // actually we expect downsample from 30k to 1k (i.e. by 30), and max input of 1024. So max output of 1024/30 < 37
        const ContinuousChannel* channel = getContinuousChannel(realChannel);

        if (channel->getSampleRate() != eegInputSampRate) {
            LOGE("Expected a sample rate of exactly ", eegInputSampRate, ", but found ", channel->getSampleRate());
            return;
        }

        if (size / eegDownsampleBy + 1 > outputBufferSize) {
            LOGE("size / downsampleBy +1 = ", size, " / ", eegDownsampleBy, " +1 is greater than expected (expected ", outputBufferSize, ")");
            return;
        }

        
        uint8_t eegBuffer[outputBufferSize] = {}; // initialise with zeros

        uint64 remainder = eegFullSampCount[writeChannel] % eegDownsampleBy;
        const float bitVolts = channel->getBitVolts();
        int nSampsWritten = 0;
        for(int i = remainder == 0 ? 0 : eegDownsampleBy - remainder; i<size; i += eegDownsampleBy) {
            float voltage8bit = dataBuffer[i] / (bitVolts * 4);
            if (voltage8bit > 127) voltage8bit = 127;
            if (voltage8bit < -128) voltage8bit = -128;
            eegBuffer[nSampsWritten++] = static_cast<int8_t>(voltage8bit);
        }

        diskWriteLock.enter();
        fwrite(eegBuffer, 1, nSampsWritten, eegFiles[writeChannel]);
        diskWriteLock.exit();
        
        eegFullSampCount.getReference(writeChannel) += size;

    }

    void RecordEnginePlugin::writeEvent(int eventChannel, const EventPacket& event)
    {
        // Not sure what an 'event' is in this context, i don't think we want to write anything, but maybe we could
        // write out to a log file of some kind.
    }


    void RecordEnginePlugin::writeSpike(int electrodeIndex, const Spike* spike)
    {
        if (mode != SPIKES_AND_SET) {
            return;
        }

        constexpr int numChans = 4;
        constexpr int bytesPerChan = 4 /* 4 byte timestamp */ + 50 /* one-byte voltage for 50 samples */;
        constexpr int totalBytes = bytesPerChan * numChans;
        constexpr int oeSampsPerSpike = 40; // seems to be hard-coded as 8+32 = 40


        const SpikeChannel* channel = getSpikeChannel(electrodeIndex);
        if (channel->getNumChannels() != numChans) {
            LOGE("Expected exactly 4 channels for a spike, but found ", channel->getNumChannels());
            return;
        }
        if (channel->getTotalSamples() != oeSampsPerSpike) {
            LOGE("Expected exactly 40 samples per spike, which will be padded out to 50, but found ", channel->getTotalSamples());
            return;
        }

        uint8_t spikeBuffer[totalBytes] = {}; // initialise with zeros


        uint32_t timestamp = static_cast<uint32_t>(spike->getTimestampInSeconds() * 96000);
        timestamp = swapEndianness(timestamp);

        const float* voltageData = spike->getDataPointer();
        
        const float bitVolts = channel->getChannelBitVolts(0);

        for (int i = 0; i < numChans; i++)
        {
            std::memcpy(spikeBuffer + bytesPerChan * i, &timestamp, sizeof(uint32_t));

            const float bitVolts = channel->getChannelBitVolts(i);
            for (int j = 0; j < oeSampsPerSpike; j++)
            {
                // Go from openephys floating point [-something, +something] => [-128, +127]. Not sure why exactly, but dividing by bitVolts * 4 seems to be about right.
                float voltage8bit = voltageData[i * oeSampsPerSpike + j] / (bitVolts * 4);
                if (voltage8bit > 127) voltage8bit = 127;
                if (voltage8bit < -128) voltage8bit = -128;
                spikeBuffer[i*bytesPerChan + 4 /* timestamp bytes */ + j] = static_cast<int8_t>(voltage8bit);
            }
        }

        diskWriteLock.enter();

        fwrite(spikeBuffer, 1, totalBytes, tetFiles[spike->getChannelIndex()]);  
        
        diskWriteLock.exit();
        tetSpikeCount.getReference(spike->getChannelIndex())++;
    }


    void RecordEnginePlugin::writeTimestampSyncText(
        uint64 streamId,
        int64 timestamp,
        float sourceSampleRate,
        String text)
    {
        // maybe we could write this to the header of each file?
    }


}
