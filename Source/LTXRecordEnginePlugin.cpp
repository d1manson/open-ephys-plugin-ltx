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

        if (getNumRecordedSpikeChannels() > 0) {
            mode = RecordMode::SPIKES_AND_SET;
            LOGC("LTX RecordEngine using mode:SPIKES_AND_SET.");
        } else if (getNumRecordedContinuousChannels() == 0) {
            LOGE("No spikes and no continous channels, nothing to record.");
            mode = RecordMode::NONE;
            return;
        } else if (getContinuousChannel(0)->getStreamName() == "bonsai") {
            mode = RecordMode::POS_ONLY; // first continuous channel is bonsai data, treat as pos
            LOGC("LTX RecordEngine using mode:POS_ONLY.");
        } else {
            mode = RecordMode::EEG_ONLY;
            LOGC("LTX RecordEngine using mode:EEG_ONLY.");
        }

        std::string basePath = rootFolder.getParentDirectory().getFullPathName().toStdString()
            + (experimentNumber == 1 ? "" : " e" + std::to_string(experimentNumber))
            + (recordingNumber == 0 ? "" : " r" + std::to_string(recordingNumber+1));

        std::chrono::system_clock::time_point start_tm = std::chrono::system_clock::now(); // used to calculate duration (not sure if OpenEphys offers an alternative)
        

        if (mode == RecordMode::SPIKES_AND_SET) {
            setFile = std::make_unique<LTXFile>(basePath, ".set", start_tm);
            // no custom header

            tetFiles.clear();
            tetSpikeCount.clear();
            for (int i = 0; i < getNumRecordedSpikeChannels(); i++) {
                tetFiles.push_back(std::make_unique<LTXFile>(basePath, "." + std::to_string(i + 1), start_tm));
                LTXFile* f = tetFiles.back().get();
                f->AddHeaderValue("num_chans", 4);
                f->AddHeaderValue("bytes_per_timestamp", 4);
                f->AddHeaderValue("samples_per_spike", 50);
                f->AddHeaderValue("bytes_per_sample", 1);
                f->AddHeaderValue("spike_format", "t,ch1,t,ch2,t,ch3,t,ch4");
                f->AddHeaderPlaceholder("num_spikes");
                // maybe need to add dummy header value: f->AddHeaderValue("timebase", "96000 hz");

                tetSpikeCount.push_back(0);
            }
        } else if (mode == RecordMode::EEG_ONLY){ 
            eegFiles.clear();
            eegFullSampCount.clear();
            for (int i = 0; i < getNumRecordedContinuousChannels(); i++){ 
               eegFiles.push_back(std::make_unique<LTXFile>(basePath, ".efg" + (i == 0 ? "" : std::to_string(i + 1)), start_tm));
                LTXFile* f = eegFiles.back().get();
                f->AddHeaderValue("num_chans", 1);
                f->AddHeaderValue("sample_rate", std::to_string(eegOutputSampRate) + " hz");
                f->AddHeaderPlaceholder("num_EEG_samples");
                eegFullSampCount.push_back(0);
            }
        }
        else if (mode == RecordMode::POS_ONLY) {
            posFile = std::make_unique<LTXFile>(basePath, ".pos", start_tm);
            
            float sampleRate = getContinuousChannel(0)->getSampleRate();
            posFile->AddHeaderValue("timebase", std::to_string(sampleRate) + " hz");
            posFile->AddHeaderValue("sample_rate", std::to_string(sampleRate) + " hz");
            
            posFile->AddHeaderValue("bytes_per_timestamp", 4);
            posFile->AddHeaderValue("bytes_per_coord", 2);
            
            posFile->AddHeaderValue("pos_format", "t,x1,y1,x2,y2,numpix1,numpix2");

            // no idea if this is needed for anything. dummy values...
            posFile->AddHeaderValue("num_colors", 4); 
            posFile->AddHeaderValue("bearing_color_1", 330);
            posFile->AddHeaderValue("bearing_color_2", 150);
            posFile->AddHeaderValue("bearing_color_3", 0);
            posFile->AddHeaderValue("bearing_color_4", 0);

            // these probably need some way to be configured properly, but for now hard code
            // not actually sure what they are supposed to mean.
            posFile->AddHeaderValue("pixels_per_meter", 400);
            posFile->AddHeaderValue("window_min_x", 0);
            posFile->AddHeaderValue("window_max_x", 1000);
            posFile->AddHeaderValue("window_min_y", 0);
            posFile->AddHeaderValue("window_max_y", 1000);
            posFile->AddHeaderValue("min_x", 0);
            posFile->AddHeaderValue("max_x", 1000);
            posFile->AddHeaderValue("min_y", 0);
            posFile->AddHeaderValue("max_y", 1000);

            posFile->AddHeaderPlaceholder("num_pos_samples");
            posSampCount = 0;
            posSampleBuffer = {};
            posNumChans = getDataStream(0)->getContinuousChannels().size();
            LOGC("Recording pos file with ", posNumChans, " actual datapoints per sample.");

        }

    }

    void RecordEnginePlugin::closeFiles()
    {
        std::chrono::system_clock::time_point end_tm = std::chrono::system_clock::now();
           
        if (mode == SPIKES_AND_SET) {
            setFile->FinaliseFile(end_tm);
            
            for (int i = 0; i < tetFiles.size(); i++) {
                tetFiles[i]->FinaliseHeaderPlaceholder(tetSpikeCount[i]);
                tetFiles[i]->FinaliseFile(end_tm);
            }
        } else if(mode == RecordMode::EEG_ONLY){
            for (int i = 0; i < eegFiles.size(); i++) {
                eegFiles[i]->FinaliseHeaderPlaceholder(eegFullSampCount[i] / eegDownsampleBy);
                eegFiles[i]->FinaliseFile(end_tm);
            }
        } else if (mode == RecordMode::POS_ONLY) {
            posFile->FinaliseHeaderPlaceholder(posSampCount);
            posFile->FinaliseFile(end_tm);
        }

        LOGC("Completed writing files.")

    }

    void RecordEnginePlugin::writeContinuousData(int writeChannel,
                                                   int realChannel,
                                                   const float* dataBuffer,
                                                   const double* ftsBuffer,
                                                   int size)
    {
        if (mode == RecordMode::EEG_ONLY) {

            constexpr int outputBufferSize = 1024;
            const ContinuousChannel* channel = getContinuousChannel(realChannel);

            if (channel->getSampleRate() != eegInputSampRate) {
                LOGE("Expected a sample rate of exactly ", eegInputSampRate, ", but found ", channel->getSampleRate());
                return;
            }

            if (size / eegDownsampleBy + 1 > outputBufferSize) {
                LOGE("size / downsampleBy +1 = ", size, " / ", eegDownsampleBy, " +1 is greater than expected (expected ", outputBufferSize, ")");
                return;
            }

            // TODO: can we do better here using intrinsics, similar to float32sToInt8s, but with a take step size?
            //       maybe some kind of gather op?
            uint64 remainder = eegFullSampCount[writeChannel] % eegDownsampleBy;
            int8_t eegBuffer[outputBufferSize] = {}; // initialise with zeros
            const float bitVolts = channel->getBitVolts();
            size_t nSampsWritten = 0;
            for (int i = remainder == 0 ? 0 : eegDownsampleBy - remainder; i < size; i += eegDownsampleBy) {
                float voltage8bit = dataBuffer[i] / (bitVolts * 4);
                if (voltage8bit > 127) voltage8bit = 127;
                if (voltage8bit < -128) voltage8bit = -128;
                eegBuffer[nSampsWritten++] = static_cast<int8_t>(voltage8bit);
            }

            eegFiles[writeChannel]->WriteBinaryData(eegBuffer, nSampsWritten);
            eegFullSampCount[writeChannel] += size;
        }
        else if (mode == RecordMode::POS_ONLY) {
            // see note in header file about the extra complication here, and the two assumtions we make (one of which we do assert on here).
            if (size != 1) {
                LOGE("LTX mode:POS_ONLY currently only supports one pos sample per chunk, but encountered ", size, ". Recording stopped.");
                CoreServices::setAcquisitionStatus(false); 
                return; 
            }

            const float factor = 1024;  // not is a hacky thing for now, todo: decide how best to store these numbers upstream

            switch (writeChannel){
            case 0:
                posSampCount++;
                posSampleBuffer.timestamp = swapEndianness(ftsBuffer[0]); // todo: get serious about timestamps for pos
                posSampleBuffer.x1 = dataBuffer[0] * factor;
                break;
            case 1:
                posSampleBuffer.y1 = dataBuffer[0] * factor;
                break;
            case 2:
                posSampleBuffer.x2 = dataBuffer[0] * factor;
                break;
            case 3:
                posSampleBuffer.y2 = dataBuffer[0] * factor;
                break;
            case 4:
                posSampleBuffer.numpix1 = dataBuffer[0] * factor;
                break;
            case 5:
                posSampleBuffer.numpix2 = dataBuffer[0] * factor;
                break;
            }

            if (writeChannel == posNumChans-1) {
                posFile->WriteBinaryData(&posSampleBuffer, sizeof posSampleBuffer);                
            }
            
        }

    }

    void RecordEnginePlugin::writeEvent(int eventChannel, const EventPacket& event)
    {
        // Not sure what an 'event' is in this context, i don't think we want to write anything, but maybe we could
        // write out to a log file of some kind.
    }


    template <size_t N>
    inline void float32sToInt8s(const float* src, int8_t* dest, float factor) {
        // TODO: for N=40, use SIMD 512bit intrinsics three times, rather than a loop
        //       may need to copy into a properly aligned float[48] buffer, with zero padding at the end
        //       should be able to make that static as the zeros will never be overwritten
        //       can then use 16,16,16 float operations. Think there might even be a builtin way to "saturate" the cast
        for (int i = 0; i < N; i++) {
            float v = src[i] * factor;
            if (v > 127) v = 127;
            if (v < -128) v = -128;
            dest[i] = static_cast<int8_t>(v);
        }
    }

    void RecordEnginePlugin::writeSpike(int electrodeIndex, const Spike* spike)
    {
        if (mode != RecordMode::SPIKES_AND_SET) {
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

        int8_t spikeBuffer[totalBytes] = {}; // initialise with zeros


        uint32_t timestamp = static_cast<uint32_t>(spike->getTimestampInSeconds() * 96000);
        timestamp = swapEndianness(timestamp);

        const float* voltageData = spike->getDataPointer();
        
        const float bitVolts = channel->getChannelBitVolts(0);

        for (int i = 0; i < numChans; i++)
        {
            std::memcpy(spikeBuffer + bytesPerChan * i, &timestamp, sizeof(uint32_t));

            const float bitVolts = channel->getChannelBitVolts(i);

            float32sToInt8s<oeSampsPerSpike>(
                &voltageData[i * oeSampsPerSpike],
                &spikeBuffer[i * bytesPerChan + 4 /* timestamp bytes */],
                1. / (channel->getChannelBitVolts(i) * 4.) //  Not sure why exactly, but dividing by bitVolts * 4 seems to be about right.
            );
        }

        tetFiles[spike->getChannelIndex()]->WriteBinaryData(spikeBuffer, totalBytes);
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


}
