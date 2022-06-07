#include "Json.h"

#include "../../Core/Engine.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Threads/ThreadPool.h"
#include "../../File/FileManager.h"
#include "../../Utils/Debug/Profiler.h"

namespace engine {

	void JsonLoader::loadAsset(Json& v, const JsonLoadingParams& params, const JsonLoadingCallback& callback) {
		auto&& engine = Engine::getInstance();

		if (params.flags->async) {
			engine.getModule<AssetManager>()->getThreadPool()->enqueue(TaskType::COMMON, 0, [params, callback](const CancellationToken& token) {
				PROFILE_TIME_SCOPED_M(jsonLoading, params.file)
				auto&& engine = Engine::getInstance();
				FileManager* fm = engine.getModule<FileManager>();
				size_t fsize;
				if (const char* data = fm->readFile(params.file, fsize)) {
					Json v = Json::parse(std::string(data));
					if (callback) { callback(v, AssetLoadingResult::LOADING_SUCCESS); }
					delete data;
				} else {
					if (callback) { callback(Json(), AssetLoadingResult::LOADING_ERROR); }
				}
			});
		} else {
			PROFILE_TIME_SCOPED_M(jsonLoading, params.file)
			FileManager* fm = engine.getModule<FileManager>();
			size_t fsize;
			if (const char* data = fm->readFile(params.file, fsize)) {
				v = Json::parse(std::string(data));
				if (callback) { callback(v, AssetLoadingResult::LOADING_SUCCESS); }
				delete data;
			} else {
				if (callback) { callback(v, AssetLoadingResult::LOADING_ERROR); }
			}
		}
	}

	/*
	//////// variant2 no templates
	IAssetData* JsonLoader2::loadAsset(const ILoadParams& params) const {
		auto&& engine = Engine::getInstance();
		FileManager* fm = engine.getModule<FileManager>();
		size_t fsize;
		const JsonLoadParams& jsParams = static_cast<const JsonLoadParams&>(params);
		if (const char* data = fm->readFile(jsParams.getFileName(), fsize)) {
			return new JsonLoadData(Json::parse(std::string(data)));
		}

		return nullptr;
	}
	*/
}