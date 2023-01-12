/*
 * Copyright (C) 2015, Nils Moehrle
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef ACC_PRIMITIVES_HEADER
#define ACC_PRIMITIVES_HEADER

#include <limits>

#include "defines.h"

ACC_NAMESPACE_BEGIN

template <typename Vec3fType>
struct AABB
{
	Vec3fType min;
	Vec3fType max;
};

template <typename Vec3fType>
struct Tri
{
	Vec3fType a;
	Vec3fType b;
	Vec3fType c;
};

template <typename Vec3fType>
struct Ray
{
	Vec3fType origin;
	Vec3fType dir;
	float     tmin;
	float     tmax;
};

template <typename Vec3fType>
inline AABB<Vec3fType> operator+(AABB<Vec3fType> const &a, AABB<Vec3fType> const &b)
{
	AABB<Vec3fType> aabb;
	for (std::size_t i = 0; i < 3; ++i)
	{
		aabb.min[i] = std::min(a.min[i], b.min[i]);
		aabb.max[i] = std::max(a.max[i], b.max[i]);
	}
	return aabb;
}

template <typename Vec3fType>
inline void operator+=(AABB<Vec3fType> &a, AABB<Vec3fType> const &b)
{
	for (int i = 0; i < 3; ++i)
	{
		a.min[i] = std::min(a.min[i], b.min[i]);
		a.max[i] = std::max(a.max[i], b.max[i]);
	}
}

template <typename Vec3fType>
inline void calculate_aabb(Tri<Vec3fType> const &tri, AABB<Vec3fType> *aabb)
{
	for (int i = 0; i < 3; ++i)
	{
		aabb->min[i] = std::min(tri.a[i], std::min(tri.b[i], tri.c[i]));
		aabb->max[i] = std::max(tri.a[i], std::max(tri.b[i], tri.c[i]));
	}
}

template <typename Vec3fType>
inline float surface_area(AABB<Vec3fType> const &aabb)
{
	float e0 = aabb.max[0] - aabb.min[0];
	float e1 = aabb.max[1] - aabb.min[1];
	float e2 = aabb.max[2] - aabb.min[2];
	return 2.0f * (e0 * e1 + e1 * e2 + e2 * e0);
}

template <typename Vec3fType>
inline float mid(AABB<Vec3fType> const &aabb, std::size_t d)
{
	return (aabb.min[d] + aabb.max[d]) / 2.0f;
}

constexpr float inf     = std::numeric_limits<float>::infinity();
constexpr float flt_eps = std::numeric_limits<float>::epsilon();

/* Derived form Tavian Barnes implementation posted in
 * http://tavianator.com/fast-branchless-raybounding-box-intersections-part-2-nans/
 * on 23rd March 2015 */
template <typename Vec3fType>
inline bool intersect(Ray<Vec3fType> const &ray, AABB<Vec3fType> const &aabb, float *tmin_ptr)
{
	float tmin = ray.tmin, tmax = ray.tmax;
	for (int i = 0; i < 3; ++i)
	{
		float t1 = (aabb.min[i] - ray.origin[i]) / ray.dir[i];
		float t2 = (aabb.max[i] - ray.origin[i]) / ray.dir[i];

		tmin = std::max(tmin, std::min(std::min(t1, t2), inf));
		tmax = std::min(tmax, std::max(std::max(t1, t2), -inf));
	}
	*tmin_ptr = tmin;
	return tmax >= std::max(tmin, 0.0f);
}

template <typename Vec3fType>
inline Vec3fType barycentric_coordinates(Vec3fType const &v0, Vec3fType const &v1,
                                         Vec3fType const &v2)
{
	/* Derived from the book "Real-Time Collision Detection"
     * by Christer Ericson published by Morgan Kaufmann in 2005 */
	float  d00   = glm::dot(v0, v0);
	float  d01   = glm::dot(v0, v1);
	float  d11   = glm::dot(v1, v1);
	float  d20   = glm::dot(v2, v0);
	float  d21   = glm::dot(v2, v1);
	double denom = d00 * d11 - d01 * d01;

	Vec3fType bcoords;
	bcoords[1] = (d11 * d20 - d01 * d21) / denom;
	bcoords[2] = (d00 * d21 - d01 * d20) / denom;
	bcoords[0] = 1.0f - bcoords[1] - bcoords[2];

	return bcoords;
}

template <typename Vec3fType>
inline Vec3fType closest_point(Vec3fType const &v, AABB<Vec3fType> const &aabb)
{
	Vec3fType ret;
	for (int i = 0; i < 3; ++i)
	{
		ret[i] = std::max(aabb.min[i], std::min(v[i], aabb.max[i]));
	}
	return ret;
}

template <typename Vec3fType>
inline Vec3fType closest_point(Vec3fType const &vertex, Tri<Vec3fType> const &tri)
{
	Vec3fType ab     = tri.b - tri.a;
	Vec3fType ac     = tri.c - tri.a;
	Vec3fType normal = glm::cross(ac, ab);
	//ac.cross(ab);

	float n = glm::length(normal);
	//.norm();
	//if (n < flt_eps) return false;

	normal /= n;

	Vec3fType p  = vertex - glm::dot(normal, vertex - tri.a) * normal;
	Vec3fType ap = p - tri.a;

	Vec3fType bcoords = barycentric_coordinates(ab, ac, ap);

	if (bcoords[0] < 0.0f)
	{
		Vec3fType bc = tri.c - tri.b;
		float     n  = glm::length(bc);
		//bc.norm();
		float t = std::max(0.0f, std::min(glm::dot(bc, p - tri.b) / n, n));
		return tri.b + t / n * bc;
	}

	if (bcoords[1] < 0.0f)
	{
		Vec3fType ca = tri.a - tri.c;
		float     n  = glm::length(ca);
		float     t  = std::max(0.0f, std::min(glm::dot(ca,p - tri.c) / n, n));
		return tri.c + t / n * ca;
	}

	if (bcoords[2] < 0.0f)
	{
		//Vec3fType ab = tri.b - tri.a;
		float n = glm::length(ab);
		float t = std::max(0.0f, std::min(glm::dot(ab,p - tri.a) / n, n));
		return tri.a + t / n * ab;
	}

	return tri.a * bcoords[0] + tri.b * bcoords[1] + tri.c * bcoords[2];
}

template <typename Vec3fType>
inline bool intersect(Ray<Vec3fType> const &ray, Tri<Vec3fType> const &tri,
                      float *t_ptr, Vec3fType *bcoords_ptr)
{
	Vec3fType ab     = tri.b - tri.a;
	Vec3fType ac     = tri.c - tri.a;
	Vec3fType normal = glm::cross(ac, ab);
	//ac.cross(ab);

	float n = glm::length(normal);
	//normal.norm();
	if (n < flt_eps)
		return false;

	normal /= n;

	float cosine = glm::dot(normal, ray.dir);
	//normal.dot(ray.dir);
	if (std::abs(cosine) < flt_eps)
		return false;

	float t = glm::dot(-normal, ray.origin - tri.a) / cosine;
	//-normal.dot(ray.origin - tri.a) / cosine;

	if (t < ray.tmin || ray.tmax < t)
		return false;

	Vec3fType p  = ray.origin + t * ray.dir;
	Vec3fType ap = p - tri.a;

	Vec3fType bcoords = barycentric_coordinates(ab, ac, ap);

	constexpr float eps = 1e-3f;
	if (-eps > bcoords[0] || bcoords[0] > 1.0f + eps)
		return false;
	if (-eps > bcoords[1] || bcoords[1] > 1.0f + eps)
		return false;
	if (-eps > bcoords[2] || bcoords[2] > 1.0f + eps)
		return false;

	if (t_ptr != nullptr)
		*t_ptr = t;
	if (bcoords_ptr != nullptr)
		*bcoords_ptr = bcoords;

	return true;
}

ACC_NAMESPACE_END

#endif /* ACC_PRIMITIVES_HEADER */
