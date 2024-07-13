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
    static_assert(std::char_traits<char>::length(headerMarkerNumSpikes) == headerMarkerSize, "header marker length should be 14");
    static_assert(std::char_traits<char>::length(headerMarkerDuration) == headerMarkerSize, "header marker length should be 14");


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

    String RecordEnginePlugin::getEngineId() const
    {
        return "LTX_RECORD_ENGINE";
    }


    void RecordEnginePlugin::openFiles(File rootFolder, int experimentNumber, int recordingNumber)
    {
        String basePath(rootFolder.getParentDirectory().getFullPathName());

        startTime = std::chrono::high_resolution_clock::now(); // used to calculate duration (not sure if OpenEphys offers an alternative)

        std::time_t t = std::time(nullptr);
        std::tm start_tm = *std::localtime(&t);
        
        openSetFile(basePath, start_tm);

        // the tet files all have the same header, that is until we write their num spikes at the end.
        std::stringstream header;
        header << "trial_date " << std::put_time(&start_tm, "%A, %d %b %Y")
            << "\r\ntrial_time " << std::put_time(&start_tm, "%H:%M")
            << "\r\ncreated_by open-ephys-plugin-ltx"
            << "\r\nduration " << headerMarkerDuration
            << "\r\nnum_chans 4"
            << "\r\ntimebase 96000 hz" // probably not
            << "\r\nbytes_per_timestamp 4"
            << "\r\nsamples_per_spike 50" // actually it's not, but we zero-pad when writing tot he file to keep it at 50
            << "\r\nbytes_per_sample 1"
            << "\r\nspike_format t,ch1,t,ch2,t,ch3,t,ch4"
            << "\r\nnum_spikes " << headerMarkerNumSpikes
            << "\r\ndata_start";
        std::string headerStr = header.str();
        tetHeaderOffsetNumSpikes = headerStr.find(headerMarkerNumSpikes);
        tetHeaderOffsetDuration = headerStr.find(headerMarkerDuration);


        for (int i = 0; i < getNumRecordedSpikeChannels(); i++){
            String fullPath = basePath + "." + String(i+1); // use 1-based indexing for the output file names
            LOGC("OPENING FILE: ", fullPath);

            diskWriteLock.enter();
            FILE* tetFile_ = fopen(fullPath.toUTF8(), "wb");
            fwrite(headerStr.c_str(), 1, headerStr.size(), tetFile_);
            diskWriteLock.exit();

            tetFiles.add(tetFile_);
            tetSpikeCount.add(0);
        }

    }

    void RecordEnginePlugin::closeFiles()
    {
        // compute duration
        char durationStr[headerMarkerSize + 1];
        std::chrono::seconds durationSeconds std::chrono::duration_cast<Seconds>(std::chrono::high_resolution_clock::now() - startTime);
        sprintf(durationStr, headerMarkerFormat, durationSeconds);

        // finalise set file
        diskWriteLock.enter();
        fseek(setFile, setHeaderOffsetDuration, SEEK_SET);
        fwrite(durationStr.c_str(), 1, durationStr.size(), setFile);
        fclose(setFile);

        // finalise tet files
        for (int i = 0; i < tetFiles.size(); i++) {
            constexpr char endToken[] = "\r\ndata_end";
            fwrite(endToken, 1, sizeof(endToken) - 1 /* ignore \0 */, tetFiles[i]);

            // write duration
            fseek(tetFiles[i], tetHeaderOffsetDuration, SEEK_SET);
            fwrite(durationStr.c_str(), 1, durationStr.size(), tetFiles[i]);

            // write num spikes
            char nSpikesStr[headerMarkerSize + 1];
            sprintf(nSpikesStr, headerMarkerFormat, tetSpikeCount[i]);
            fseek(tetFiles[i], tetHeaderOffsetNumSpikes, SEEK_SET);
            fwrite(nSpikesStr.c_str(), 1, nSpikesStr.size(), tetFiles[i]);

            fclose(tetFiles[i]);

        }
        diskWriteLock.exit();

    }

    void RecordEnginePlugin::writeContinuousData(int writeChannel,
                                                   int realChannel,
                                                   const float* dataBuffer,
                                                   const double* ftsBuffer,
                                                   int size)
    {
        // not implementing, but may want to store some continuous data (maybe with a lower sample rate and band-pass
        // filter) as an EEG.
    }

    void RecordEnginePlugin::writeEvent(int eventChannel, const EventPacket& event)
    {
        // Not sure what an 'event' is in this context, i don't think we want to write anything, but maybe we could
        // write out to a log file of some kind.
    }


    void RecordEnginePlugin::writeSpike(int electrodeIndex, const Spike* spike)
    {

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

        const float* voltageData = spike->getDataPointer();
        
        const float bitVolts = channel->getChannelBitVolts(0);
        LOGC("bitVolts: ", bitVolts);

        for (int i = 0; i < numChans; i++)
        {
            std::memcpy(spikeBuffer + bytesPerChan * i, &timestamp, sizeof(uint32_t));

            const float bitVolts = channel->getChannelBitVolts(i);
            for (int j = 0; j < oeSampsPerSpike; j++)
            {
                // Go from openephys floating point [-something, +something] => [-128, +127]
                // I think in principle we can do this properly, given the bitVolts, but for now we just do this fairly arbitrary thing
                float voltage8bit = voltageData[i * oeSampsPerSpike + j] / bitVolts / 500 * 127;
                if (voltage8bit > 127) voltage8bit = 127;
                if (voltage8bit < -128) voltage8bit = -128;
                spikeBuffer[i*bytesPerChan + 4 /* timestamp bytes */ + j] = static_cast<int8_t>(voltage8bit);
            }
        }

        diskWriteLock.enter();

        fwrite(spikeBuffer, 1, totalBytes, tetFiles[spike->getChannelIndex()]);  
        
        diskWriteLock.exit();
        tetSpikeCount[spike->getChannelIndex()]++;
    }


    void RecordEnginePlugin::writeTimestampSyncText(
        uint64 streamId,
        int64 timestamp,
        float sourceSampleRate,
        String text)
    {
        // maybe we could write this to the header of each file?
    }





    void RecordEnginePlugin::openSetFile(String basePath, std::tm tm)
    {
        String fullPath = basePath + ".set";

        std::stringstream header;
        headerPt1 << "trial_date " << std::put_time(&tm, "%A, %d %b %Y")
            << "\r\ntrial_time " << std::put_time(&tm, "%H:%M")
            << "\r\ncreated_by open-ephys-ltx-plugin"
            << "\r\nduration " << headerMarkerDuration;
        std::string headerStr = header.str();
        setHeaderOffsetDuration = headerStr.find(headerMarkerDuration);

        diskWriteLock.enter();
        FILE* setFile_ = fopen(fullPath.toUTF8(), "wb");
        fwrite(headerStr.c_str(), sizeof(char), headerStr.size(), setFile_);
        diskWriteLock.exit();
        setFile = setFile_;
    }

}
