#pragma once

#include <limits>
#include <vector>
#include <cstdint>

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

	struct TextureFrameBounds {
		float minx =  std::numeric_limits<float>::max();
		float miny =  std::numeric_limits<float>::max();
		float maxx = -std::numeric_limits<float>::max();
		float maxy = -std::numeric_limits<float>::max();

		TextureFrameBounds() = default;
		TextureFrameBounds(const TextureFrame* frame) {
			for (size_t i = 0, sz = frame->_vtx.size(); i < sz; ++i) {
				if (i & 1) {
					miny = std::min(miny, frame->_vtx[i]);
					maxy = std::max(maxy, frame->_vtx[i]);
				} else {
					minx = std::min(minx, frame->_vtx[i]);
					maxx = std::max(maxx, frame->_vtx[i]);
				}
			}
		}
	};
}