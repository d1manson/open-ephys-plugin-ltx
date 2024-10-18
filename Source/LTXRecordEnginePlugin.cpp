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
#include "util.h"

namespace LTX {
    constexpr int timestampTimebase = 96000;
    constexpr int eegInputSampRate = 30000;
    constexpr int eegOutputSampRate = 1000;
    constexpr int eegDownsampleBy = eegInputSampRate / eegOutputSampRate;
    constexpr int posWindowSize = 1000; // todo: allow user to configure this
    constexpr int posPixelsPerPos = 600; // todo: allow user to configure this
    constexpr int requiredPosChans = 7; // see assertion below for more details
 
    RecordEnginePlugin::RecordEnginePlugin(){}

    RecordEnginePlugin::~RecordEnginePlugin() {}

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
            LOGC("LTX RecordEngine using mode:SPIKES_AND_SET (", mode, ").");
        } else if (getNumRecordedContinuousChannels() == 0) {
            LOGE("No spikes and no continous channels, nothing to record.");
            mode = RecordMode::NONE;
            return;
        } else if (getContinuousChannel(0)->getStreamName() == "bonsai") {
            mode = RecordMode::POS_ONLY; // first continuous channel is bonsai data, treat as pos
            LOGC("LTX RecordEngine using mode:POS_ONLY (", mode, ").");
        } else {
            mode = RecordMode::EEG_ONLY;
            LOGC("LTX RecordEngine using mode:EEG_ONLY (", mode, ").");
        }

        std::string basePath = rootFolder.getParentDirectory().getFullPathName().toStdString()
            + (experimentNumber == 1 ? "" : " e" + std::to_string(experimentNumber))
            + (recordingNumber == 0 ? "" : " r" + std::to_string(recordingNumber+1));

        std::chrono::system_clock::time_point start_tm = std::chrono::system_clock::now(); // used to calculate duration (not sure if OpenEphys offers an alternative)
        

        if (mode == RecordMode::SPIKES_AND_SET) {
            setFile = std::make_unique<LTXFile>(basePath, ".set", start_tm);

            setFile->AddHeaderValue("lasttrialdatetime", std::chrono::duration_cast<std::chrono::seconds>(start_tm.time_since_epoch()).count());

            // need some numbers here. todo: allow configuration
            setFile->AddHeaderValue("lightBearing_1", 0);
            setFile->AddHeaderValue("lightBearing_2", 180);
            setFile->AddHeaderValue("lightBearing_3", 0);
            setFile->AddHeaderValue("lightBearing_4", 0);
            

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
                f->AddHeaderValue("sample_rate", std::to_string(getSpikeChannel(i)->getSampleRate()) + " hz"); 
                f->AddHeaderValue("timebase", std::to_string(timestampTimebase) + " hz");
                f->AddHeaderPlaceholder("num_spikes");

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
            if (getDataStream(0)->getContinuousChannels().size() != requiredPosChans) {
                LOGE("For LTX pos, require exactly ", requiredPosChans, " float channels from Bonsai. The first is a timestamp, and then x1,y1,x2,y2,numpix1,numpix2. ",
                    "However, received ", getDataStream(0)->getContinuousChannels().size(), " channels.");
                CoreServices::setAcquisitionStatus(false);
                return;
            }

            posFile = std::make_unique<LTXFile>(basePath, ".pos", start_tm);
            
            posSampRate = getContinuousChannel(0)->getSampleRate();
            posFile->AddHeaderValue("timestamp_timebase", std::to_string(timestampTimebase) + " hz");
            posFile->AddHeaderValue("timebase", std::to_string(posSampRate) + " hz"); // required by Waveform GUI at least, maybe other code
            posFile->AddHeaderValue("sample_rate", std::to_string(posSampRate) + " hz");
            
            posFile->AddHeaderValue("bytes_per_timestamp", 4);
            posFile->AddHeaderValue("bytes_per_coord", 2);
            
            posFile->AddHeaderValue("pos_format", "t,x1,y1,x2,y2,numpix1,numpix2");

            // no idea if this is needed for anything. dummy values...
            posFile->AddHeaderValue("num_colours", 4); 
            posFile->AddHeaderValue("bearing_colour_1", 330);
            posFile->AddHeaderValue("bearing_colour_2", 150);
            posFile->AddHeaderValue("bearing_colour_3", 0);
            posFile->AddHeaderValue("bearing_colour_4", 0);

            // not actually sure what they are supposed to mean. Presumably some of them
            // might need to be computed based on the observed data and set at the end of the trial
            // we also want the user to be able to configure some of them in the UX, somehow.
            posFile->AddHeaderValue("pixels_per_metre", posPixelsPerPos);
            posFile->AddHeaderValue("window_min_x", 0);
            posFile->AddHeaderValue("window_max_x", posWindowSize);
            posFile->AddHeaderValue("window_min_y", 0);
            posFile->AddHeaderValue("window_max_y", posWindowSize);
            posFile->AddHeaderValue("min_x", 0);
            posFile->AddHeaderValue("max_x", posWindowSize);
            posFile->AddHeaderValue("min_y", 0);
            posFile->AddHeaderValue("max_y", posWindowSize);
           
            posFile->AddHeaderPlaceholder("num_pos_samples");
            posSampCount = 0;
            posFirstTimestamp = TIMESTAMP_UNINITIALIZED; // gets initialised using the first continuous data below
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
            // no timestamps written in the EEG file at all

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
        } else if (mode == RecordMode::POS_ONLY) {

            if (posFirstTimestamp == TIMESTAMP_UNINITIALIZED && writeChannel == 0) { 
               posFirstTimestamp = dataBuffer[0];
               if (posFirstTimestamp > 4*60*60 /* 4 hours in seconds = 14400 */) {
                   LOGE("POS recording started with 32bit floating point timestamp: ", posFirstTimestamp, " seconds. That's quite large; you won't get many decimal places of precision. "
                       "Stopping acquistion now. When you restart acquisition, the timestamp will begin at 0seconds, which will work much better.");
                   // note this is talking about the timestamp we recieve from the Bonsai source; the timestamps we record below will always start from zero, but they will inherit
                   // the precision provided by the Bonsai source, hence the check here. Note we are checking at the start of the recording, so we use an even more conservative threshold.
                   CoreServices::setAcquisitionStatus(false);
                   return;
               }
            }

            // Note we are assuming that channels came in order so that when we get the 0th one first
            // and when we get the last one we can assume we've seen the all and they all had the same size.
            // Hopefully a safe assumption, but i haven't actually checked the existing implementation in the core codebase.

            if (writeChannel == 0) {
                posSamplesBuffer.resize(size);
                std::memset(posSamplesBuffer.data(), 0, size * sizeof(posSamplesBuffer));
                posSampCount += size;
                for (int i = 0; i < size; i++) {
                    posSamplesBuffer[i].timestamp = BSWAP32(
                        static_cast<int32_t>((dataBuffer[i] - posFirstTimestamp) * timestampTimebase));
                }
            } else {
                for (int i = 0; i < size; i++) {
                    posSamplesBuffer[i].xy_etc[writeChannel-1] = BSWAP16(static_cast<uint16_t>(dataBuffer[i]));
                }

                if (writeChannel == requiredPosChans - 1) {
                    posFile->WriteBinaryData(static_cast<void*>(posSamplesBuffer.data()), sizeof(PosSample) * size);
                }
            }
            
        }

    }

    void RecordEnginePlugin::writeEvent(int eventChannel, const EventPacket& event)
    {
        // We don't currently support writing TTL data here. If you want TTL data use another
        // record engine for that.
    }


    template <size_t N, int Min, int Max>
    inline void float32sToInt8s(const float* src, int8* dest) {
        static_assert(N == 40, "expected 40 samples per spike");
        static_assert(Min == -250 && Max == 250, "expected input range [-250,250]");

        // TODO: for N=40, use SIMD 512bit intrinsics three times, rather than a loop
        //       may need to copy into a properly aligned float[48] buffer, with zero padding at the end
        //       should be able to make that static as the zeros will never be overwritten
        //       can then use 16,16,16 float operations. Think there might even be a builtin way to "saturate" the cast
        for (int i = 0; i < N; i++) {
            int32 v = static_cast<int32>(src[i]) / 2; // see static_assert above regarding expected input range [-250,250]
            dest[i] = std::min(std::max(v, -128), 127);
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

        int8 spikeBuffer[totalBytes] = {}; // initialise with zeros

        // seems that getTimestampInSeconds() is from the start of recording not start of acquisition
        int32_t timestamp = BSWAP32(static_cast<int32_t>(spike->getTimestampInSeconds() * timestampTimebase));

        const float* voltageData = spike->getDataPointer();
        for (int i = 0; i < numChans; i++)
        {
            std::memcpy(&spikeBuffer[i * bytesPerChan], &timestamp, 4);
            float32sToInt8s<oeSampsPerSpike, -250, 250>(
                &voltageData[i * oeSampsPerSpike],
                &spikeBuffer[i * bytesPerChan + 4 /* timestamp bytes */]);
        }
        tetFiles[spike->getChannelIndex()]->WriteBinaryData(spikeBuffer, totalBytes);
        tetSpikeCount[spike->getChannelIndex()]++;
    }


    void RecordEnginePlugin::writeTimestampSyncText(
        uint64 streamId,
        int64 sampleNumber,
        float sourceSampleRate,
        String text)
    {
    }


}
