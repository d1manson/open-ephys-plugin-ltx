
#include "LTXFile.h" 

namespace LTX {
	LTXFile::LTXFile(const std::string& basePath, const std::string& extension, HighResTimePoint start_tm_):
		start_tm(start_tm_) 
	{
		std::lock_guard<std::mutex> lock(mut);

		std::string fullpath = basePath + extension;

		// TODO: log opening fullPath
		
		theFile = fopen(fullpath.c_str(), "wb");

		/*
		std::time_t t = std::time(nullptr);
        std::tm start_tm = *std::localtime(&t);
        
		"trial_date " << std::put_time(&start_tm, "%A, %d %b %Y")
		<< "\r\ntrial_time " << std::put_time(&start_tm, "%H:%M")
		<< "\r\ncreated_by open-ephys-plugin-ltx"
		<< "\r\nduration " << headerMarkerDuration

		*/
		// write default headers	
	}
	
	LTXFile::~LTXFile() {
		std::lock_guard<std::mutex> lock(mut);

		if (status != FileWriteStatus::CLOSED) {
			// todo: throw error - should have called finalise
			fclose(theFile);
		}
	}

	template <typename T>
	void LTXFile::AddHeaderValue(const std::string& key, T value) {
		std::lock_guard<std::mutex> lock(mut);

		if (status != FileWriteStatus::HEADERS) {
			// todo: throw an error
		}

		// todo: create string from key and value and newline
		diskWriteLock.enter();
		fwrite(setHeaderStr.c_str(), sizeof(char), setHeaderStr.size(), theFile);
		diskWriteLock.exit();


	}
	
	void LTXFile::AddHeaderPlaceholder(const std::string& key) {
		std::lock_guard<std::mutex> lock(mut);

		if (status != FileWriteStatus::HEADERS) {
			// todo: throw an error
		}
		if (headerOffsetCustom != 0) {
			// todo: throw an error
		}

		// todo: store current file position plus key length plus space as placeholderOffset[idx]
		// 
		// todo: create string from key and fixed size placeholder and newline
		
		//fwrite(setHeaderStr.c_str(), sizeof(char), setHeaderStr.size(), theFile);
	}
	
	template <typename T>
	void LTXFile::FinaliseHeaderPlaceholder(T value) {
		std::lock_guard<std::mutex> lock(mut);

		if (status == FileWriteStatus::HEADERS) {
			status = FileWriteStatus::EPILOGUE;
		} else if (status == FileWriteStatus::BINARY) {
			status = FileWriteStatus::EPILOGUE;
			// todo: write data_end
		}

		if (status != FileWriteStatus::EPILOGUE) {
			// todo: throw error
		} else if (headerOffsetCustom == 0) {
			// todo: throw error
		}

		// todo: seek to the spot in placeholderOffsets and write
	
		headerOffsetCustom = 0;
	}

	void LTXFile::WriteBinaryData(uint8_t* buffer, size_t totalBytes) {
		std::lock_guard<std::mutex> lock(mut);

		if (status == FileWriteStatus::HEADERS) {
			status = FileWriteStatus::BINARY;
			// todo: write data_start
		}

		if (status != FileWriteStatus::BINARY) {
			// todo: throw error
		}

		fwrite(buffer, 1, totalBytes, theFile);
	}


	void LTXFile::FinaliseFile(HighResTimePoint end_tm) {
		std::lock_guard<std::mutex> lock(mut);

		if (status == FileWriteStatus::CLOSED) {
			// todo: throw error
		}

		if (status == FileWriteStatus::BINARY) {
			// todo: write data_end
		}

		status = FileWriteStatus::CLOSED;

		/*
		std::chrono::seconds durationSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - startTime);
		sprintf(durationStr, headerMarkerFormat, durationSeconds);
		LOGC("Finalising headers, with durationStr = ", durationStr);
		*/

		// todo: finalise the duration placeholder
		fclose(theFile); // something like that
	}

}