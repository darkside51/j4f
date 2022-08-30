#pragma once

#include <vector>
#include<cstdint>

namespace engine {
	struct TextureFrame {
		std::vector<float> _vtx;
		std::vector<float> _uv; // (ux,uy), (ux, uy), ...
		std::vector<uint32_t> _idx;

		TextureFrame() = default;
		TextureFrame(const std::vector<float>& uv) : _uv(uv) {}
		TextureFrame(std::vector<float>&& uv) : _uv(std::move(uv)) {}
		TextureFrame(const std::vector<float>& vtx, const std::vector<float>& uv, const std::vector<uint32_t>& idx) : _vtx(vtx), _uv(uv), _idx(idx) {}
		TextureFrame(std::vector<float>&& vtx, std::vector<float>&& uv, std::vector<uint32_t>&& idx) : _vtx(std::move(vtx)), _uv(std::move(uv)), _idx(std::move(idx)) {}
	};
}