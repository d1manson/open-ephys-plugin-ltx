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
        LOGD("CUSTOM AXONA PLUGIN LOG")
    }

    RecordEnginePlugin::~RecordEnginePlugin()
    {

    }


    RecordEngineManager* RecordEnginePlugin::getEngineManager()
    {
        RecordEngineManager* man = new RecordEngineManager("AXONA", "Axona Format",
            &(engineFactory<RecordEnginePlugin>));

        return man;
    }

    String RecordEnginePlugin::getEngineId() const
    {
        return "AXONA_RECORD_ENGINE";
    }


    void RecordEnginePlugin::openFiles(File rootFolder, int experimentNumber, int recordingNumber)
    {

        String basePath(rootFolder.getParentDirectory().getFullPathName() + "_" + String(experimentNumber) + "_" + String(recordingNumber));
        
        openSetFile(basePath);

        spikeChannels.clear();


        // New file for each experiment, e.g. experiment1.nwb, epxperiment2.nwb, etc.
        String basepath = rootFolder.getFullPathName() +
         rootFolder.getSeparatorString() +
         "experiment" + String(experimentNumber); // TODO: ideally allow parameterization of the name


        for (int i = 0; i < getNumRecordedSpikeChannels(); i++){
            const  SpikeChannel* chan = getSpikeChannel(i);
            // TODO: assert chan->getNumChannels() == 4
            spikeChannels.add(chan);
            //: TODO: create a tet file for the given tetrode, with a header.
            //        and store the file handle in an array on the class
        }

    }

    void RecordEnginePlugin::closeFiles()
    {
        // TODO: this
        fclose(setFile);
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
        const SpikeChannel* channel = getSpikeChannel(electrodeIndex);

        // The aim is to write a 4 byte timestamp (trial starting at 0), and then 50x uint8 (?) of voltage data
        // the spike should contain the channel or something
    }


    void RecordEnginePlugin::writeTimestampSyncText(
        uint64 streamId,
        int64 timestamp,
        float sourceSampleRate,
        String text)
    {
        // maybe we could write this to the header of each file?
    }





    void RecordEnginePlugin::openSetFile(String basePath)
    {
        FILE* setFile_;

        String fullPath = basePath + ".set";

        LOGD("OPENING FILE: ", fullPath);

        File f = File(fullPath);

        diskWriteLock.enter();

        setFile_ = fopen(fullPath.toUTF8(), "wb");

        if (f.exists())
            fseek(setFile_, 0, SEEK_END);




        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);

        std::stringstream headerPt1;
        headerPt1 << "trial_date " << std::put_time(&tm, "%A, %d %b %Y")
            << "\ntrial_time " << std::put_time(&tm,"%H:%M")
            << "\ncreated_by open-ephys-axona-plugin";

        std::string data = headerPt1.str();
        fwrite(data.c_str(), sizeof(char), data.size(), setFile_);

        diskWriteLock.exit();
        setFile = setFile_;
    }

}
