
#include "LTXFile.h" 
#include "LTXRecordEnginePlugin.h" // only needed for LOG* methods
#include <cstring>
#include <cstdio>

namespace LTX {
	constexpr char* data_start_token = "\r\ndata_start";
	constexpr char* data_end_token = "\r\ndata_end";
	constexpr char* placeholder_token = "              ";

	LTXFile::LTXFile(const std::string& basePath, const std::string& extension, std::chrono::system_clock::time_point start_tm_):
		start_tm(start_tm_) 
	{
		std::lock_guard<std::mutex> lock(mut);

		std::string fullpath = basePath + extension;

		LOGC("Opening file: ", fullpath);
		
		theFile = fopen(fullpath.c_str(), "wb");

		// headers: trial date and time
		std::time_t start_tm_t = std::chrono::system_clock::to_time_t(start_tm);
        std::tm start_tm_lt = *std::localtime(&start_tm_t); 
		
		char strftime_output[50];
		std::strftime(strftime_output, sizeof(strftime_output), "%A, %d %b %Y", &start_tm_lt);
		fprintf(theFile, "trial_date %s", strftime_output);
		std::strftime(strftime_output, sizeof(strftime_output), "%H:%M", &start_tm_lt);
		fprintf(theFile, "\r\ntrial_time %s", strftime_output);

		// headers: hard-coded values
		fprintf(theFile, "\r\ncreated_by open-ephys-plugin-ltx");

		// headers: placeholder for duration
		fprintf(theFile, "\r\nduration %s", placeholder_token);
		headerOffsetDuration = ftell(theFile) - strlen(placeholder_token);
	}
	
	LTXFile::~LTXFile() {
		std::lock_guard<std::mutex> lock(mut);

		if (status != FileWriteStatus::CLOSED) {
			LOGE("LTXFile destructor called before calling FinaliseFile().");
			fclose(theFile);
		}
	}

	template <typename T>
	void LTXFile::AddHeaderValue(const std::string& key, T value) {
		std::lock_guard<std::mutex> lock(mut);

		if (status != FileWriteStatus::HEADERS) {
			LOGE("AddHeaderValue with status not set to FileWriteStatus::HEADERS (status: ", status, ").");
			return;
		}
		
		// format the value as a string
		std::ostringstream oss;
		oss << value;

		fprintf(theFile, "\r\n%s %s", key.c_str(), oss.str().c_str());
	}
	
	template void LTXFile::AddHeaderValue<int>(const std::string&, int);
	template void LTXFile::AddHeaderValue<char const*>(const std::string&, char const*);
	template void LTXFile::AddHeaderValue<std::string>(const std::string&, std::string);
	
	void LTXFile::AddHeaderPlaceholder(const std::string& key) {
		std::lock_guard<std::mutex> lock(mut);

		if (status != FileWriteStatus::HEADERS) {
			LOGE("AddHeaderPlaceholder with status not set to FileWriteStatus::HEADERS (status: ", status, ").");
			return;
		}
		if (headerOffsetCustom != 0) {
			LOGE("AddHeaderPlaceholder more than once.");
			return;
		}

		fprintf(theFile, "\r\n%s %s", key.c_str(), placeholder_token);
		headerOffsetCustom = ftell(theFile) - strlen(placeholder_token);
	}
	
	template <typename T>
	void LTXFile::FinaliseHeaderPlaceholder(T value) {
		std::lock_guard<std::mutex> lock(mut);

		if (status == FileWriteStatus::HEADERS) {
			status = FileWriteStatus::EPILOGUE;
		} else if (status == FileWriteStatus::BINARY) {
			status = FileWriteStatus::EPILOGUE;
			fprintf(theFile, data_end_token);
		}

		if (status != FileWriteStatus::EPILOGUE) {
			LOGE("FinaliseHeaderPlaceholder called with bad status: ", status);
			return;
		} else if (headerOffsetCustom == 0) {
			LOGE("FinaliseHeaderPlaceholder either called twice, or without prior to AddHeaderPlaceholder.");
			return;
		}

		fseek(theFile, headerOffsetCustom, SEEK_SET);
		
		std::ostringstream oss;
		oss << value;
		
		// todo: maybe should check the string is less than the length of the placeholder?

		fprintf(theFile, oss.str().c_str());

		headerOffsetCustom = 0;
	}

	template void LTXFile::FinaliseHeaderPlaceholder<uint64_t>(uint64_t);


	void LTXFile::WriteBinaryData(void* buffer, size_t totalBytes) {
		std::lock_guard<std::mutex> lock(mut);

		if (status == FileWriteStatus::HEADERS) {
			status = FileWriteStatus::BINARY;
			fprintf(theFile, data_start_token);
		}

		if (status != FileWriteStatus::BINARY) {
			LOGE("WriteBinaryData called with status not set to HEADERS or BINARY (status: ", status, ")");
			return;
		}

		fwrite(buffer, 1, totalBytes, theFile);
	}


	void LTXFile::FinaliseFile(std::chrono::system_clock::time_point end_tm) {
		std::lock_guard<std::mutex> lock(mut);

		if (status == FileWriteStatus::CLOSED) {
			LOGE("FinaliseFile called twice.");
			return;

		}

		if (status == FileWriteStatus::BINARY) {
			fprintf(theFile, data_end_token);
		}

		status = FileWriteStatus::CLOSED;

		// finalise the duration header
		fseek(theFile, headerOffsetDuration, SEEK_SET);
		std::chrono::seconds durationSeconds = std::chrono::duration_cast<std::chrono::seconds>(end_tm - start_tm);
		fprintf(theFile, "%lld", durationSeconds.count());

		fclose(theFile);
	}



}