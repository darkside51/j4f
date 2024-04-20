// ♣♠♦♥
#include "Camera.h"
#include <algorithm>

namespace engine {

	void Frustum::calculate(const mat4f& clip) noexcept {
        // Gribb & Hartmann method for Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix
        // get A,B,C,D for right plane
        _frustum[0] = {clip[0][3] - clip[0][0], clip[1][3] - clip[1][0], clip[2][3] - clip[2][0],
                        clip[3][3] - clip[3][0]};
        // get A,B,C,D for left plane
        _frustum[1] = {clip[0][3] + clip[0][0], clip[1][3] + clip[1][0], clip[2][3] + clip[2][0],
                        clip[3][3] + clip[3][0]};
        // get A,B,C,D for bottom plane
        _frustum[2] = {clip[0][3] + clip[0][1], clip[1][3] + clip[1][1], clip[2][3] + clip[2][1],
                        clip[3][3] + clip[3][1]};
        // get A,B,C,D for top plane
        _frustum[3] = {clip[0][3] - clip[0][1], clip[1][3] - clip[1][1], clip[2][3] - clip[2][1],
                        clip[3][3] - clip[3][1]};
        // get A,B,C,D for back plane
        _frustum[4] = {clip[0][3] - clip[0][2], clip[1][3] - clip[1][2], clip[2][3] - clip[2][2],
                        clip[3][3] - clip[3][2]};
        // get A,B,C,D for front plane
        _frustum[5] = {clip[0][3] + clip[0][2], clip[1][3] + clip[1][2], clip[2][3] + clip[2][2],
                        clip[3][3] + clip[3][2]};

		_normalized = false;
	}

	void Frustum::normalize() noexcept { // normalize frustum planes
		if (_normalized) return;

		for (auto & plane : _frustum) { // приводим уравнения плоскостей к нормальному виду
			const float t = inv_sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
            plane *= t;
		}

		_normalized = true;
	}

	bool Frustum::isPointVisible(const vec3f& p) const noexcept {
		for (const auto & plane : _frustum) {
			if (glm::dot(plane, vec4f(p, 1.0f)) < 0.0f) {
				return false;
			}
		}

		return true;
	}

	bool Frustum::isSphereVisible(const vec3f& p, const float r) noexcept {
		if (_normalized == false) { // для определения расстояний до плоскостей нужна нормализация плоскостей
			normalize();
		}

		// проходим через все стороны пирамиды
		for (const auto & plane : _frustum) {
			if (glm::dot(plane, vec4f(p, 1.0f)) < -r) { // центр сферы дальше от плоскости, чем её радиус => вся сфера снаружи, возвращаем false
				return false;
			}
		}

		return true;
	}

	bool Frustum::isCubeVisible_classic(const vec3f& min, const vec3f& max) const noexcept {
		for (const auto & plane : _frustum) {
			if (plane.x * min.x + plane.y * min.y + plane.z * min.z + plane.w > 0.0f)
				continue;
			if (plane.x * max.x + plane.y * min.y + plane.z * min.z + plane.w > 0.0f)
				continue;
			if (plane.x * min.x + plane.y * max.y + plane.z * min.z + plane.w > 0.0f)
				continue;
			if (plane.x * max.x + plane.y * max.y + plane.z * min.z + plane.w > 0.0f)
				continue;
			if (plane.x * min.x + plane.y * min.y + plane.z * max.z + plane.w > 0.0f)
				continue;
			if (plane.x * max.x + plane.y * min.y + plane.z * max.z + plane.w > 0.0f)
				continue;
			if (plane.x * min.x + plane.y * max.y + plane.z * max.z + plane.w > 0.0f)
				continue;
			if (plane.x * max.x + plane.y * max.y + plane.z * max.z + plane.w > 0.0f)
				continue;

			// если мы дошли досюда, куб не внутри пирамиды.
			return false;
		}

		return true;
	}

	bool Frustum::isCubeVisible(const vec3f& min, const vec3f& max) const noexcept {
		// https://gamedev.ru/code/articles/FrustumCulling
		for (const auto & plane : _frustum) {
			const auto d = std::max(min.x * plane.x, max.x * plane.x)
				+ std::max(min.y * plane.y, max.y * plane.y)
				+ std::max(min.z * plane.z, max.z * plane.z)
				+ plane.z;
            if (d <= 0.0) {
                return false;
            }
		}
		return true;
	}

	Camera::Camera(const uint32_t w, const uint32_t h) :
		_projectionType(ProjectionType::PERSPECTIVE),
		_dirty(0u),
		_size(w, h),
		_frustum(nullptr),
		_rotationOrder(RotationsOrder::RO_XYZ),
		_scale(unitVec3),
		_rotation(emptyVec3),
		_position(emptyVec3),
        _padding(emptyVec2)
	{}

	Camera::Camera(const vec2f& sz) :
		_projectionType(ProjectionType::PERSPECTIVE),
		_dirty(0u),
		_size(sz),
		_frustum(nullptr),
		_rotationOrder(RotationsOrder::RO_XYZ),
		_scale(unitVec3),
		_rotation(emptyVec3),
		_position(emptyVec3),
        _padding(emptyVec2)
	{}

    Camera::Camera(Camera&& cam) noexcept :
            _projectionType(cam._projectionType),
            _dirty(cam._dirty),
            _size(cam._size),
            _near_far(cam._near_far),
            _transform(std::move(cam._transform)),
            _invTransform(std::move(cam._invTransform)),
            _invViewTransform(std::move(cam._invViewTransform)),
            _viewTransform(std::move(cam._viewTransform)),
            _projectionTransform(std::move(cam._projectionTransform)),
            _frustum(std::move(cam._frustum)),
            _rotationOrder(cam._rotationOrder),
            _scale(cam._scale),
            _rotation(cam._rotation),
            _position(cam._position),
            _padding(cam._padding) {

    }

	void Camera::makeProjection(const float fov, const float aspect, const float znear, const float zfar) noexcept {
		_projectionType = ProjectionType::PERSPECTIVE;
		_projectionTransform = glm::perspective(fov, aspect, znear, zfar);
		_projectionTransform[1][1] *= -1.0f;
		_near_far = vec2f(znear, zfar);
		_dirty->transform = 1u;

		/* // parameters calculation variant(считает кажется правильно, но точности не хватает немного)
		auto fov2 = 2.0f * atanf(1.0f / _projectionTransform[1][1]);
		auto near2 = _projectionTransform[3][2] / (_projectionTransform[2][3] - 1.0);
		auto far2 = near2 * (_projectionTransform[2][2] - 1.0f) / (_projectionTransform[2][2] + 1.0f);
		*/
	}

	void Camera::makeOrtho(const float left, const float right, const float bottom, const float top, const float znear, const float zfar) noexcept {
		_projectionType = ProjectionType::ORTHO;
		_projectionTransform = glm::ortho(left, right, bottom, top, znear, zfar);
		_projectionTransform[3][2] = 0.0f; // для большей точности в буфере глубины
		_projectionTransform[1][1] *= -1.0f;
		_near_far = vec2f(znear, zfar);
		_dirty->transform = 1u;
	}

	void Camera::resize(const float w, const float h) noexcept {
		switch (_projectionType) {
			case ProjectionType::PERSPECTIVE:
			{
				_projectionTransform[0][0] = -_projectionTransform[1][1] * (h / w);
			}
				break;
			case ProjectionType::ORTHO:
			{
				_projectionTransform[0][0] *= (_size.x / w);
				_projectionTransform[1][1] *= (_size.y / h);
			}
				break;
			default:
				break;
		}

		_size.x = w;
		_size.y = h;
		_dirty->transform = 1u;
	}

	bool Camera::calculateTransform() noexcept {
		if (_dirty->transform == 0u) return false;

		_viewTransform = mat4f(1.0f);
		
		if (_scale != unitVec3) {
			scaleMatrix(_viewTransform, _scale);
		}

		if (_rotation != emptyVec3) {
			rotateMatrix_byOrder(_viewTransform, _rotation, _rotationOrder);
		}

		if (_position != emptyVec3) {
			//translateMatrixTo(viewTransform, -_position);
			translateMatrix(_viewTransform, -_position);
		}

        if (_padding != emptyVec2) {
			constexpr float kViewportNormalizedDim = 2.0f;
            _viewTransform[3][0] += _padding.x * kViewportNormalizedDim;
            _viewTransform[3][1] += _padding.y * kViewportNormalizedDim;
        }

		_transform = _projectionTransform * _viewTransform;
		_dirty->transform = 0u;
		_dirty->invTransform = 1u;
		_dirty->invViewTransform = 1u;

		if (_frustum) {
			_frustum->calculate(_transform);
		}

		notifyObservers();

		return true;
	}

	vec2f Camera::worldToScreen(const vec3f& p) const noexcept {
		vec4f v = _transform * vec4f(p, 1.0f);
		v /= v.w;

		// for vulkan topdown viewport oy
		v.y *= -1.0f;

		v.x = (0.5f + v.x * 0.5f) * _size.x;
		v.y = (0.5f + v.y * 0.5f) * _size.y;

		return vec2f(v.x, v.y);
	}

	Ray Camera::screenToWorld(const vec2f& screenCoord) noexcept {
		vec4f v;
		v.x = (((2.0f * screenCoord.x) / _size.x) - 1.0f);
		v.y = (((2.0f * screenCoord.y) / _size.y) - 1.0f);
		v.z = 0.0f;
		v.w = 1.0f;

		// for vulkan topdown viewport oy
		v.y *= -1.0f;

		vec4f sp = getInvTransform() * v;
		sp /= sp.w;

		v.z = 1.0f;

		vec4f fp = _invTransform * v;
		fp /= fp.w;

		return Ray(sp.x, sp.y, sp.z, fp.x, fp.y, fp.z);
	}

}