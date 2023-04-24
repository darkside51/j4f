#pragma once

#include "../Core/Common.h"
#include "../Core/EngineModule.h"
#include "../Core/Threads/Synchronisations.h"
#include "FileSystem.h"

#include <cstddef>
#include <unordered_map>

namespace engine {

	class FileManager : public IEngineModule { // todo: syncronisations
	public:
		FileManager() {}

		template<typename T, typename... Args>
		T* createFileSystem(Args&& ...args) {
			T* fs = new T(std::forward<Args>(args)...);
			fs->_uniqueId = UniqueTypeId<FileSystem>::getUniqueId<T>();
			_fileSystems.push_back(fs);
			return fs;
		}

		void removeFileSystem(const FileSystem* fs) {
			unmapFileSystem(fs);
			_fileSystems.erase(std::remove(_fileSystems.begin(), _fileSystems.end(), fs), _fileSystems.end());
			delete fs;
		}

		template<typename T>
		T* getFileSystem() {
			const uint16_t id = UniqueTypeId<FileSystem>::getUniqueId<T>();
			for (auto&& fs : _fileSystems) {
				if (fs->_uniqueId == id) { return static_cast<T*>(fs); }
			}

			return nullptr;
		}

		~FileManager() {
			for (auto&& fs : _fileSystems) {
				delete fs;
			}

			_fileSystems.clear();
			_mappedFiles.clear();
		}

		const std::vector<FileSystem*>& getFileSystems() const { return _fileSystems; }

		inline const FileSystem* getFileSystemByFilePath(const std::string& path) const {
			auto it = _mappedFiles.find(path);
			if (it != _mappedFiles.end()) {
				return it->second[0];
			}

			for (FileSystem* fs : _fileSystems) {
				if (fs->hasFile(path)) {
					return fs;
				}
			}
			return nullptr;
		}

		bool hasFile(const std::string& path) const {
			return getFileSystemByFilePath(path) != nullptr;
		}

		size_t lengthFile(const std::string& path) const {
			if (const FileSystem* fs = getFileSystemByFilePath(path)) {
				return fs->lengthFile(path);
			}
			return 0;
		}

		std::string getFullPath(const std::string& path) const {
			if (const FileSystem* fs = getFileSystemByFilePath(path)) {
				return fs->fullPath(path);
			}
			return path;
		}

		char* readFile(const std::string& path, size_t& fileSize) const {
			if (const FileSystem* fs = getFileSystemByFilePath(path)) {
				return fs->readFile(path, fileSize);
			}
			return nullptr;
		}

		bool readFile(const std::string& path, std::vector<char>& data) const {
			if (const FileSystem* fs = getFileSystemByFilePath(path)) {
				return fs->readFile(path, data);
			}
			return false;
		}

		bool readFile(const std::string& path, std::vector<std::byte>& data) const {
			if (const FileSystem* fs = getFileSystemByFilePath(path)) {
				return fs->readFile(path, data);
			}
			return false;
		}

		const FileSystem* writeFile(const std::string& path, const void* data, const size_t sz, const char* mode) const {
			for (FileSystem* fs : _fileSystems) {
				if (fs->writeFile(path, data, sz, mode)) {
					return fs;
				}
			}
			return nullptr;
		}

		void mapFileSystem(const FileSystem* fs) {
			std::vector<std::string> files = fs->getFiles();
			for (const std::string& s : files) {
				_mappedFiles[s].push_back(fs);
			}
		}

		void unmapFileSystem(const FileSystem* fs) {
			for (auto it = _mappedFiles.begin(); it != _mappedFiles.end(); ) {
				std::vector<const FileSystem*>& v = it->second;
				v.erase(std::remove(v.begin(), v.end(), fs), v.end());
				if (v.empty()) {
					it = _mappedFiles.erase(it);
				} else {
					++it;
				}
			}
		}

	private:
		// hmmm... use shared pointers for this????
		std::vector<FileSystem*> _fileSystems;
		std::unordered_map<std::string, std::vector<const FileSystem*>> _mappedFiles;
	};
}