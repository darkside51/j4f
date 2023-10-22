#pragma once

#include "json.hpp"
#include "../../Core/AssetManager.h"

namespace engine {
	using Json = nlohmann::json;

	template<>
	struct AssetLoadingParams<Json> : public AssetLoadingFlags {
		std::string file = "";
		AssetLoadingParams(const std::string& f) : file(f) {}
	};

	using JsonLoadingParams = AssetLoadingParams<Json>;
	using JsonLoadingCallback = AssetLoadingCallback<Json>;

	class JsonLoader {
	public:
        using asset_loader_type = JsonLoader;
		using asset_type = Json;
		static void loadAsset(Json& v, const JsonLoadingParams& params, const JsonLoadingCallback& callback);
        static void cleanUp() noexcept {}
	private:
	};


	/*
	//////// variant2 no templates
	struct JsonLoadData : public IAssetData {
		JsonLoadData(Json&& v) : value(std::move(v)) {}
		Json value;
	};
	
	class JsonLoadParams : public ILoadParams {
	public:
		size_t loaderId() const override { return typeid(Json).hash_code(); }
		JsonLoadParams(const std::string& f) : _file(f) {}
		const std::string& getFileName() const { return _file; }
	private:
		std::string _file;
	};

	struct JsonLoader2 : public IAssetLoader2 {
		size_t loaderId() const override { return typeid(Json).hash_code(); }
		IAssetData* loadAsset(const ILoadParams& params) const override;
	};
	*/
}