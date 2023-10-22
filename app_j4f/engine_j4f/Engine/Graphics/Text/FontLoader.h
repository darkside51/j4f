#pragma once

#include "Font.h"
#include "../../Core/AssetManager.h"

#include <atomic>

namespace engine {

	template<>
	struct AssetLoadingParams<Font> : public AssetLoadingFlags {
		std::string file;
		AssetLoadingParams(const std::string& f) : file(f) {}
	};

	using FontLoadingParams = AssetLoadingParams<Font>;
	using FontLoadingCallback = AssetLoadingCallback<Font*>;

	class FontLoader {
	public:
        using asset_loader_type = FontLoader;
		using asset_type = Font*;
		static void loadAsset(Font*& v, const FontLoadingParams& params, const FontLoadingCallback& callback);
        static void cleanUp() noexcept {}
	private:
		static void addCallback(Font*, const FontLoadingCallback&);
		static void executeCallbacks(Font*, const AssetLoadingResult);

		inline static std::atomic_bool _callbacksLock;
		inline static std::unordered_map<Font*, std::vector<FontLoadingCallback>> _callbacks;
	};

}