#include "CommonFileSystem.h"
#include "../Utils/StringHelper.h"
#include <filesystem>
#include <functional>
#include <cstdio>
#include <sys/stat.h>

#include <Platform_inc.h>

#ifdef j4f_PLATFORM_WINDOWS
#pragma warning(disable : 4996)

#include <Windows.h>

std::string getCurrentDirectory() {
	char buffer[260];
	GetModuleFileNameA(NULL, buffer, 260);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	std::string result = std::string(buffer).substr(0, pos) + '/';
	engine::stringReplace(result, "\\", "/");
	return result;
}
#elif defined(j4f_PLATFORM_LINUX)
std::string getCurrentDirectory() {
    char buff[FILENAME_MAX];
    getcwd( buff, FILENAME_MAX );
    return std::string(buff) + "/";
}
#endif

namespace engine {

	CommonFileSystem::CommonFileSystem() {
		_root = getCurrentDirectory();
	}

	File* CommonFileSystem::file_open(const char* path, const char* mode) const {
		return reinterpret_cast<File*>(std::fopen(path, mode));
	}

	void CommonFileSystem::file_close(const File* f) const {
		std::fclose(reinterpret_cast<FILE*>(const_cast<File*>(f)));
	}

	int CommonFileSystem::file_seek(File* f, uint64_t offset, const Seek_File seek) const {
		return std::fseek(reinterpret_cast<FILE*>(f), offset, static_cast<int>(seek));
	}

	size_t CommonFileSystem::file_read(File* f, const size_t size, const size_t count, void* ptr) const {
		return std::fread(ptr, size, count, reinterpret_cast<FILE*>(f));
	}

	size_t CommonFileSystem::file_write(File* f, const size_t size, const size_t count, const void* ptr) const {
		return std::fwrite(ptr, size, count, reinterpret_cast<FILE*>(f));
	}

	uint64_t CommonFileSystem::file_tell(File* f) const {
		return std::ftell(reinterpret_cast<FILE*>(f));
	}

	std::vector<std::string> CommonFileSystem::getFiles() const {
		/*auto&& current_path = std::filesystem::current_path();
		std::string root_path = current_path.u8string() + '/';
		stringReplace(root_path, "\\", "/");*/

		std::vector<std::string> files;

		const std::function<void(const std::filesystem::path&, std::vector<std::string>&)> round = [&round, this](const std::filesystem::path& path, std::vector<std::string>& files) {
			for (const auto& entry : std::filesystem::directory_iterator(path)) {
				if (std::filesystem::is_directory(entry.status())) {
					round(entry, files);
				} else if (std::filesystem::is_regular_file(entry.status())) {
					//std::string file_path = entry.path().u8string();
					std::string file_path = entry.path().string();

					const size_t sz = _root.length();
					file_path = file_path.substr(sz, file_path.length() - sz);
					stringReplace(file_path, "\\", "/");

					files.push_back(file_path);
				}	
			}
		};

		round(std::filesystem::path(_root), files);

		return files;
	}

	bool CommonFileSystem::hasFile(const std::string& path) const {
		struct stat s;
		return (stat(fullPath(path).c_str(), &s) == 0);
	}

	size_t CommonFileSystem::lengthFile(const std::string& path) const {
		return lengthFile(path, false);
	}

	size_t CommonFileSystem::lengthFile(const std::string& path, const bool isFullPath) const {
		struct stat s;
		if (isFullPath) {
			if (stat(path.c_str(), &s) == 0) { return s.st_size; }
		} else {
			if (stat(fullPath(path).c_str(), &s) == 0) { return s.st_size; }
		}
		return 0;
	}

	char* CommonFileSystem::readFile(const std::string& path, size_t& fileSize) const {
		const std::string full_path = fullPath(path);
		fileSize = lengthFile(full_path, true);

		if (fileSize == 0) return nullptr;

		if (File* file = file_open(full_path.c_str(), "rb")) {
			char* data = new char[fileSize + 1];

			if (file_read(file, fileSize, 1, data)) {
				file_close(file);
				data[fileSize] = '\0';
				return data;
			}

			file_close(file);
			delete[] data;
		}

		return nullptr;
	}

	template <typename T>
	bool readFileImpl(const CommonFileSystem* fs, const std::string& path, T& data) {
		const std::string full_path = fs->fullPath(path);
		const size_t fileLength = fs->lengthFile(full_path, true);
		if (fileLength == 0) return false;

		if (File* file = fs->file_open(full_path.c_str(), "rb")) {
			data.resize(fileLength);
			if (fs->file_read(file, fileLength, 1, data.data())) {
				fs->file_close(file);
				return true;
			}
			fs->file_close(file);
		}

		return false;
	}

	bool CommonFileSystem::readFile(const std::string& path, std::vector<char>& data) const {
		return readFileImpl(this, path, data);
	}

	bool CommonFileSystem::readFile(const std::string& path, std::vector<std::byte>& data) const {
		return readFileImpl(this, path, data);
	}

	bool CommonFileSystem::writeFile(const std::string& path, const void* data, const size_t sz, const char* mode) const {
		const std::string full_path = fullPath(path);
		const std::filesystem::path pathToFile{ full_path };
		std::filesystem::create_directories(pathToFile.parent_path()); // create path folders

		// write file data
		if (File* file = file_open(full_path.c_str(), mode)) {
			if (file_write(file, sz, 1, data)) {
				file_close(file);
				return true;
			}
			file_close(file);
		}
		return false;
	}

}