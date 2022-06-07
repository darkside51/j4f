#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace engine {
	struct File;
	class FileManager;

	enum class Seek_File {
		SET = 0, // set file offset to offset
		CUR = 1, // set file offset to current plus offset
		END = 2  // set file offset to EOF plus offset
	};

	class FileSystem {
		friend class FileManager;
	public:
		virtual ~FileSystem() = default;

		// file works
		virtual File* file_open(const char* path, const char* mode) const = 0;
		virtual void file_close(const File* f) const = 0;
		virtual int file_seek(File* f, uint64_t offset, const Seek_File seek) const = 0;
		virtual size_t file_read(File* f, const size_t size, const size_t count, void* ptr) const = 0;
		virtual size_t file_write(File* f, const size_t size, const size_t count, const void* ptr) const = 0;
		virtual uint64_t file_tell(File* f) const = 0;

		//
		virtual std::vector<std::string> getFiles() const = 0;
		virtual bool hasFile(const std::string& path) const = 0;
		virtual size_t lengthFile(const std::string& path) const = 0;
		virtual char* readFile(const std::string& path, size_t& fileSize) const = 0;
		virtual bool readFile(const std::string& path, std::vector<char>& data) const = 0;
		virtual bool writeFile(const std::string& path, const void* data, const size_t sz, const char* mode = "wb") const = 0;

		virtual std::string fullPath(const std::string& path) const = 0;
	private:
		uint16_t _uniqueId = 0;
	};
}