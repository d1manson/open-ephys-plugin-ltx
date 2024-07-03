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

#include "AxonaRecordEnginePlugin.h"

namespace Axona {
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
        return "AXONA_RECORD_ENGINE";
    }


    void RecordEnginePlugin::openFiles(File rootFolder, int experimentNumber, int recordingNumber)
    {
        String basePath(rootFolder.getParentDirectory().getFullPathName());

        std::time_t t = std::time(nullptr);
        std::tm start_tm = *std::localtime(&t);
        
        openSetFile(basePath, start_tm);

        spikeChannels.clear();

        for (int i = 0; i < getNumRecordedSpikeChannels(); i++){
            String fullPath = basePath + "." + String(i+1); // use 1-based indexing for the output file names
            LOGC("OPENING FILE: ", fullPath);

            diskWriteLock.enter();

            FILE* tetFile_ = fopen(fullPath.toUTF8(), "wb");

            std::stringstream header;
            header << "trial_date " << std::put_time(&start_tm, "%A, %d %b %Y")
                << "\r\ntrial_time " << std::put_time(&start_tm, "%H:%M")
                << "\r\ncreated_by open-ephys-plugin-ltx"
                << "\r\nduration ##########" // TODO: overwrite this with a real value in seconds at the end of the trial
                << "\r\nnum_chans 4"
                << "\r\ntimebase 96000 hz" // probably not
                << "\r\nbytes_per_timestamp 4"
                << "\r\nsamples_per_spike 50" // probably need to zero-pad if this isn't right, because i think we need to stick with 50 for backwards compatibility
                << "\r\nbytes_per_sample 1"
                << "\r\nspike_format t,ch1,t,ch2,t,ch3,t,ch4"
                << "\r\nnum_spikes 10" // TODO: set a real value here at the end of the trial
                << "\r\ndata_start";
            std::string data = header.str();
            fwrite(data.c_str(), 1, data.size(), tetFile_);

            diskWriteLock.exit();

            tetFiles.add(tetFile_);
        }

    }

    void RecordEnginePlugin::closeFiles()
    {
        diskWriteLock.enter();
        // TODO: add duration to set file and ideally the tet files too (maybe other things?)
        fclose(setFile);
        for (int i = 0; i < tetFiles.size(); i++) {

            constexpr char endToken[] = "\r\ndata_end";
            fwrite(endToken, 1, sizeof(endToken) - 1 /* ignore \0 */, tetFiles[i]);
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
                // TODO: make the cast in a capped way to avoid overflow or udnerflow
                spikeBuffer[i*bytesPerChan + 4 /* timestamp bytes */ + j] =
                    static_cast<int8_t>(voltageData[i * oeSampsPerSpike + j] / bitVolts / 500 * 127); // convert float to milivolts (?), and then map +- 500 => +-127
            }
        }

        diskWriteLock.enter();

        fwrite(spikeBuffer, 1, totalBytes, tetFiles[spike->getChannelIndex()]);  
        
        diskWriteLock.exit();

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

        LOGD("OPENING FILE: ", fullPath);

        diskWriteLock.enter();

        FILE* setFile_ = fopen(fullPath.toUTF8(), "wb");

        std::stringstream headerPt1;
        headerPt1 << "trial_date " << std::put_time(&tm, "%A, %d %b %Y")
            << "\r\ntrial_time " << std::put_time(&tm, "%H:%M")
            << "\r\ncreated_by open-ephys-axona-plugin"
            << "\r\nduration ##########"; // TODO: overwrite this with a real value in seconds at the end of the trial

        std::string data = headerPt1.str();
        fwrite(data.c_str(), sizeof(char), data.size(), setFile_);

        diskWriteLock.exit();
        setFile = setFile_;
    }

}
