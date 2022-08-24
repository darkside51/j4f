#pragma once

namespace engine {

	class Frustum;
	struct RenderDescriptor;

	class BoundingVolume {
	public:
		bool checkFrustum(const Frustum* f) const { return true; };

		inline RenderDescriptor* getRenderDescriptor() const { return nullptr; } // for debug draw
	private:
	};

}