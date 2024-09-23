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

        std::string basePath = rootFolder.getParentDirectory().getFullPathName().toStdString();

        HighResTimePoint start_tm = std::chrono::high_resolution_clock::now(); // used to calculate duration (not sure if OpenEphys offers an alternative)
        

        if (mode == SPIKES_AND_SET) {
            setFile = std::make_unique<LTXFile>(LTXFile(basePath, ".set", start_tm));
            // no custom header

            for (int i = 0; i < getNumRecordedSpikeChannels(); i++) {
                tetFiles.add(std::make_unique<LTXFile>(LTXFile(basePath, "." + std::to_string(i + 1), start_tm)));
                LTXFile* f = tetFiles.getLast().get();
                f->AddHeaderValue("num_chans", 4);
                f->AddHeaderValue("bytes_per_timestamp", 4);
                f->AddHeaderValue("samples_per_spike", 50);
                f->AddHeaderValue("bytes_per_sample", 1);
                f->AddHeaderValue("spike_format", "t,ch1,t,ch2,t,ch3,t,ch4");
                f->AddHeaderPlaceholder("num_spikes");
                // maybe need to add dummy header value: f->AddHeaderValue("timebase", "96000 hz");

                tetSpikeCount.add(0);
            }
        } else { // mode: EEG_ONLY


            for (int i = 0; i < getNumRecordedContinuousChannels(); i++){ 
                eegFiles.add(std::make_unique<LTXFile>(LTXFile(basePath, ".eeg" + (i == 0 ? "" : std::to_string(i + 1)), start_tm)));
                LTXFile* f = eegFiles.getLast().get();
                f->AddHeaderValue("num_chans", 1);
                f->AddHeaderValue("sample_rate", std::to_string(eegOutputSampRate) + " hz");
                f->AddHeaderPlaceholder("num_EEG_samples");
                eegFullSampCount.add(0);
            }
        }

    }

    void RecordEnginePlugin::closeFiles()
    {
        HighResTimePoint end_tm = std::chrono::high_resolution_clock::now();
           
        if (mode == SPIKES_AND_SET) {
            setFile->FinaliseFile(end_tm);
            
            for (int i = 0; i < tetFiles.size(); i++) {
                tetFiles[i]->FinaliseHeaderPlaceholder(tetSpikeCount[i]);
                tetFiles[i]->FinaliseFile(end_tm);
            }
        }
        else { // mode: EEG_ONLY
            for (int i = 0; i < eegFiles.size(); i++) {
                eegFiles[i]->FinaliseHeaderPlaceholder(eegFullSampCount[i] / eegDownsampleBy);
                eegFiles[i]->FinaliseFile(end_tm);
            }
        }

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
        size_t nSampsWritten = 0;
        for(int i = remainder == 0 ? 0 : eegDownsampleBy - remainder; i<size; i += eegDownsampleBy) {
            float voltage8bit = dataBuffer[i] / (bitVolts * 4);
            if (voltage8bit > 127) voltage8bit = 127;
            if (voltage8bit < -128) voltage8bit = -128;
            eegBuffer[nSampsWritten++] = static_cast<int8_t>(voltage8bit);
        }

        eegFiles[writeChannel]->WriteBinaryData(eegBuffer, nSampsWritten);        
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

        tetFiles[spike->getChannelIndex()]->WriteBinaryData(spikeBuffer, totalBytes);
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
