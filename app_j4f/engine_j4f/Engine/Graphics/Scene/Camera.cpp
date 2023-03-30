// ♣♠♦♥
#include "Camera.h"
#include <algorithm>

namespace engine {

	void Frustum::calculate(const glm::mat4& clip) noexcept {
		//находим A,B,C,D для правой плоскости
		_frustum[0][0] = clip[0][3] - clip[0][0];
		_frustum[0][1] = clip[1][3] - clip[1][0];
		_frustum[0][2] = clip[2][3] - clip[2][0];
		_frustum[0][3] = clip[3][3] - clip[3][0];

		//находим A,B,C,D для левой плоскости
		_frustum[1][0] = clip[0][3] + clip[0][0];
		_frustum[1][1] = clip[1][3] + clip[1][0];
		_frustum[1][2] = clip[2][3] + clip[2][0];
		_frustum[1][3] = clip[3][3] + clip[3][0];

		//находим A,B,C,D для нижней плоскости
		_frustum[2][0] = clip[0][3] + clip[0][1];
		_frustum[2][1] = clip[1][3] + clip[1][1];
		_frustum[2][2] = clip[2][3] + clip[2][1];
		_frustum[2][3] = clip[3][3] + clip[3][1];

		//находим A,B,C,D для верхней плоскости
		_frustum[3][0] = clip[0][3] - clip[0][1];
		_frustum[3][1] = clip[1][3] - clip[1][1];
		_frustum[3][2] = clip[2][3] - clip[2][1];
		_frustum[3][3] = clip[3][3] - clip[3][1];

		//находим A,B,C,D для задней плоскости
		_frustum[4][0] = clip[0][3] - clip[0][2];
		_frustum[4][1] = clip[1][3] - clip[1][2];
		_frustum[4][2] = clip[2][3] - clip[2][2];
		_frustum[4][3] = clip[3][3] - clip[3][2];

		//находим A,B,C,D для передней плоскости
		_frustum[5][0] = clip[0][3] + clip[0][2];
		_frustum[5][1] = clip[1][3] + clip[1][2];
		_frustum[5][2] = clip[2][3] + clip[2][2];
		_frustum[5][3] = clip[3][3] + clip[3][2];

		_normalized = false;
	}

	void Frustum::normalize() noexcept { // normalize frustum plains
		if (_normalized) return;

		for (uint8_t i = 0; i < 6; ++i) { // приводим уравнения плоскостей к нормальному виду
			const float t = inv_sqrt(_frustum[i][0] * _frustum[i][0] + _frustum[i][1] * _frustum[i][1] + _frustum[i][2] * _frustum[i][2]);
			_frustum[i][0] *= t;
			_frustum[i][1] *= t;
			_frustum[i][2] *= t;
			_frustum[i][3] *= t;
		}

		_normalized = true;
	}

	bool Frustum::isPointVisible(const glm::vec3& p) const noexcept {
		for (uint8_t i = 0; i < 6; ++i) {
			if (_frustum[i][0] * p.x + _frustum[i][1] * p.y + _frustum[i][2] * p.z + _frustum[i][3] < 0.0f) {
				return false;
			}
		}

		return true;
	}

	bool Frustum::isSphereVisible(const glm::vec3& p, const float r) noexcept {
		if (_normalized == false) { // для определения расстояний до плоскостей нужна нормализация плоскостей
			normalize();
		}

		// Проходим через все стороны пирамиды
		for (uint8_t i = 0; i < 6; ++i) {
			if (_frustum[i][0] * p.x + _frustum[i][1] * p.y + _frustum[i][2] * p.z + _frustum[i][3] < -r) { // центр сферы дальше от плоскости, чем её радиус => вся сфера снаружи, возвращаем false
				return false;
			}
		}

		return true;
	}

	bool Frustum::isCubeVisible_classic(const glm::vec3& min, const glm::vec3& max) const noexcept {
		for (uint8_t i = 0; i < 6; ++i) {
			if (_frustum[i][0] * min.x + _frustum[i][1] * min.y +
				_frustum[i][2] * min.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * max.x + _frustum[i][1] * min.y +
				_frustum[i][2] * min.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * min.x + _frustum[i][1] * max.y +
				_frustum[i][2] * min.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * max.x + _frustum[i][1] * max.y +
				_frustum[i][2] * min.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * min.x + _frustum[i][1] * min.y +
				_frustum[i][2] * max.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * max.x + _frustum[i][1] * min.y +
				_frustum[i][2] * max.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * min.x + _frustum[i][1] * max.y +
				_frustum[i][2] * max.z + _frustum[i][3] > 0.0f)
				continue;
			if (_frustum[i][0] * max.x + _frustum[i][1] * max.y +
				_frustum[i][2] * max.z + _frustum[i][3] > 0.0f)
				continue;

			// если мы дошли досюда, куб не внутри пирамиды.
			return false;
		}

		return true;
	}

	bool Frustum::isCubeVisible(const glm::vec3& min, const glm::vec3& max) const noexcept {
		// https://gamedev.ru/code/articles/FrustumCulling
		bool inside = true;
		for (int i = 0; (inside && (i < 6)); ++i) {
			const float d = std::max(min.x * _frustum[i][0], max.x * _frustum[i][0])
				+ std::max(min.y * _frustum[i][1], max.y * _frustum[i][1])
				+ std::max(min.z * _frustum[i][2], max.z * _frustum[i][2])
				+ _frustum[i][3];
			inside &= (d > 0.0f);
		}
		return inside;
	}

	Camera::Camera(const uint32_t w, const uint32_t h) :
		_projectionType(ProjectionType::PERSPECTIVE),
		_dirty(0),
		_size(w, h),
		_frustum(nullptr),
		_rotationOrder(RotationsOrder::RO_XYZ),
		_scale(unitVec3),
		_rotation(emptyVec3),
		_position(emptyVec3)
	{}

	Camera::Camera(const glm::vec2& sz) :
		_projectionType(ProjectionType::PERSPECTIVE),
		_dirty(0),
		_size(sz),
		_frustum(nullptr),
		_rotationOrder(RotationsOrder::RO_XYZ),
		_scale(unitVec3),
		_rotation(emptyVec3),
		_position(emptyVec3)
	{}

	void Camera::makeProjection(const float fov, const float aspect, const float znear, const float zfar) noexcept {
		_projectionType = ProjectionType::PERSPECTIVE;
		_projectionTransform = glm::perspective(fov, aspect, znear, zfar);
		_projectionTransform[1][1] *= -1.0f;
		_near_far = glm::vec2(znear, zfar);
		_dirty->transform = 1;

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
		_near_far = glm::vec2(znear, zfar);
		_dirty->transform = 1;
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
		_dirty->transform = 1;
	}

	bool Camera::calculateTransform() noexcept {
		if (_dirty->transform == 0) return false;

		_viewTransform = glm::mat4(1.0f);
		
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

		_transform = _projectionTransform * _viewTransform;
		_dirty->transform = 0;
		_dirty->invTransform = 1;
		_dirty->invViewTransform = 1;

		if (_frustum) {
			_frustum->calculate(_transform);
		}

		notifyObservers();

		return true;
	}

	glm::vec2 Camera::worldToScreen(const glm::vec3& p) const noexcept {
		glm::vec4 v = _transform * glm::vec4(p, 1.0f);
		v /= v.w;

		v.x = (0.5f + v.x * 0.5f) * _size.x;
		v.y = (0.5f + v.y * 0.5f) * _size.y;

		return glm::vec2(v.x, v.y);
	}

	Ray Camera::screenToWorld(const glm::vec2& screenCoord) noexcept {
		glm::vec4 v;
		v.x = (((2.0f * screenCoord.x) / _size.x) - 1.0f);
		v.y = (((2.0f * screenCoord.y) / _size.y) - 1.0f);
		v.z = 0.0f;
		v.w = 1.0f;

		glm::vec4 sp = getInvMatrix() * v;
		sp /= sp.w;

		v.z = 1.0f;

		glm::vec4 fp = _invTransform * v;
		fp /= fp.w;

		return Ray(sp.x, sp.y, sp.z, fp.x, fp.y, fp.z);
	}

}