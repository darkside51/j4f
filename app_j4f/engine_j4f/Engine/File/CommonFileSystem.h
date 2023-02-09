#pragma once

#include "FileSystem.h"

namespace engine {

	class CommonFileSystem : public FileSystem {
	public:
		CommonFileSystem();

		File* file_open(const char* path, const char* mode) const override;
		void file_close(const File* f) const override;
		int file_seek(File* f, uint64_t offset, const Seek_File seek) const override;
		size_t file_read(File* f, const size_t size, const size_t count, void* ptr) const override;
		size_t file_write(File* f, const size_t size, const size_t count, const void* ptr) const override;
		uint64_t file_tell(File* f) const override;

		//
		std::vector<std::string> getFiles() const override;
		bool hasFile(const std::string& path) const override;
		size_t lengthFile(const std::string& path) const override;
		char* readFile(const std::string& path, size_t& fileSize) const override;
		bool readFile(const std::string& path, std::vector<char>& data) const override;
		bool writeFile(const std::string& path, const void* data, const size_t sz, const char* mode = "wb") const override;

		inline std::string fullPath(const std::string& path) const override { return _root + path; }

	private:
		size_t lengthFile_fullPath(const std::string& path) const;
		std::string _root;
	};

}