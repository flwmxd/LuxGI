/*
 * Copyright (C) 2015, Nils Moehrle
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef ACC_BVHTREE_HEADER
#define ACC_BVHTREE_HEADER

#include <array>
#include <deque>
#include <stack>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <thread>
#include <limits>

#include "primitives.h"

ACC_NAMESPACE_BEGIN

template <typename IdxType, typename Vec3fType>
class BVHTree {
public:
    typedef std::shared_ptr<BVHTree<IdxType, Vec3fType> > Ptr;
    typedef std::shared_ptr<const BVHTree<IdxType, Vec3fType> > ConstPtr;

    typedef acc::Ray<Vec3fType> Ray;
    struct Hit {
        /* Parameter of the ray (distance of hit location). */
        float t;
        /* Index of the struck triangle. */
        IdxType idx;
        /* Barycentric coordinates of hit location w.r.t. the triangle. */
        Vec3fType bcoords;
    };

private:
    static constexpr IdxType NAI = std::numeric_limits<IdxType>::max();

    typedef acc::AABB<Vec3fType> AABB;
    typedef acc::Tri<Vec3fType> Tri;

    struct Node {
        typedef IdxType ID;
        IdxType first;
        IdxType last;
        ID left;
        ID right;
        AABB aabb;
    };

    struct Bin {
        IdxType n;
        AABB aabb;
    };

    std::vector<IdxType> indices;
    std::vector<Tri> tris;

    std::atomic<IdxType> num_nodes;
    std::vector<Node> nodes;
    typename Node::ID create_node(IdxType first, IdxType last) {
        typename Node::ID node_id = num_nodes++;
        Node & node = nodes[node_id];
        node.first = first;
        node.last = last;
        node.left = NAI;
        node.right = NAI;
        node.aabb.min = Vec3fType(inf);
        node.aabb.max = Vec3fType(-inf);
        return node_id;
    }

    std::pair<typename Node::ID, typename Node::ID> sbsplit(typename Node::ID node_id,
        std::vector<AABB> const & aabbs);
    std::pair<typename Node::ID, typename Node::ID> bsplit(typename Node::ID node_id,
        std::vector<AABB> const & aabbs);
    std::pair<typename Node::ID, typename Node::ID> ssplit(typename Node::ID node_id,
        std::vector<AABB> const & aabbs);
    void split(typename Node::ID, std::vector<AABB> const & aabbs,
        std::atomic<int> * num_threads);

    bool intersect(Ray const & ray, typename Node::ID node_id, Hit * hit) const;
    Vec3fType closest_point(Vec3fType vertex, typename Node::ID node_id) const;

public:
    static
    Ptr create(std::vector<IdxType> const & faces,
        std::vector<Vec3fType> const & vertices,
        int max_threads = std::thread::hardware_concurrency()) {
        return Ptr(new BVHTree(faces, vertices, max_threads));
    }

    template <class C>
    static C convert(BVHTree const & bvh_tree);

    /* Constructs the BVH tree using the Surface Area Heuristic as
     * published in
     * "On fast Construction of SAH-based Bounding Volume Hierarchies"
     * by Ingo Wald (IEEE Symposium on Interactive Ray Tracing 2007)
     *
     * The mesh should be given as triangle index list and
     * a vector containing the 3D positions. */
    BVHTree(std::vector<IdxType> const & faces,
        std::vector<Vec3fType> const & vertices,
        int max_threads = std::thread::hardware_concurrency());

    bool intersect(Ray ray, Hit * hit_ptr = nullptr) const;
    Vec3fType closest_point(Vec3fType vertex, float max_dist = inf) const;
};


#define NUM_BINS 64
template <typename IdxType, typename Vec3fType>
void BVHTree<IdxType, Vec3fType>::split(typename Node::ID node,
        std::vector<AABB> const & aabbs,
        std::atomic<int> * num_threads) {

    typename Node::ID left, right;
    if ((*num_threads -= 1) >= 1) {
        std::tie(left, right) = sbsplit(node, aabbs);
        if (left != NAI && right != NAI) {
            //std::thread other(&BVHTree::split, this, left, std::cref(aabbs), num_threads);
            std::thread other(&BVHTree::split, this, left, aabbs, num_threads);
            split(right, aabbs, num_threads);
            other.join();
        }
    } else {
        std::deque<typename Node::ID> queue;
        queue.push_back(node);
        while (!queue.empty()) {
            typename Node::ID node = queue.back(); queue.pop_back();
            std::tie(left, right) = sbsplit(node, aabbs);
            if (left != NAI && right != NAI) {
                queue.push_back(left);
                queue.push_back(right);
            }
        }
    }
    *num_threads += 1;
}

template <typename IdxType, typename Vec3fType>
std::pair<typename BVHTree<IdxType, Vec3fType>::Node::ID, typename BVHTree<IdxType, Vec3fType>::Node::ID>
BVHTree<IdxType, Vec3fType>::sbsplit(typename Node::ID node_id,
        std::vector<AABB> const & aabbs) {
    Node const & node = nodes[node_id];
    IdxType n = node.last - node.first;
    if (n > NUM_BINS) {
        return bsplit(node_id, aabbs);
    } else {
        return ssplit(node_id, aabbs);
    }
}

template <typename IdxType, typename Vec3fType>
std::pair<typename BVHTree<IdxType, Vec3fType>::Node::ID, typename BVHTree<IdxType, Vec3fType>::Node::ID>
BVHTree<IdxType, Vec3fType>::bsplit(typename Node::ID node_id,
        std::vector<AABB> const & aabbs) {
    Node & node = nodes[node_id];
    IdxType n = node.last - node.first;

    std::array<Bin, NUM_BINS> bins;
    std::array<AABB, NUM_BINS> right_aabbs;
    std::vector<IdxType> bin(n);

    float min_cost = inf;
    std::pair<IdxType, char> split;
    for (char d = 0; d < 3; ++d) {
        float min = node.aabb.min[d];
        float max = node.aabb.max[d];
        for (Bin & bin : bins) {
            bin = {0, {Vec3fType(inf), Vec3fType(-inf)}};
        }
        for (std::size_t i = node.first; i < node.last; ++i) {
            AABB const & aabb = aabbs[indices[i]];
            char idx = ((mid(aabb, d) - min) / (max - min)) * (NUM_BINS - 1);
            bins[idx].aabb += aabb;
            bins[idx].n += 1;
            bin[i - node.first] = idx;
        }

        right_aabbs[NUM_BINS - 1] = bins[NUM_BINS - 1].aabb;
        for (std::size_t i = NUM_BINS - 1; i > 0; --i) {
            right_aabbs[i - 1] = bins[i - 1].aabb + right_aabbs[i];
        }

        AABB left_aabb = bins[0].aabb;
        std::size_t nl = bins[0].n;
        for (std::size_t idx = 1; idx < NUM_BINS; ++idx) {
            std::size_t nr = n - nl;
            float cost = (surface_area(left_aabb) / surface_area(node.aabb) * nl
            + surface_area(right_aabbs[idx]) / surface_area(node.aabb) * nr);
            if (cost <= min_cost) {
                min_cost = cost;
                split = std::make_pair(d, idx);
            }

            nl += bins[idx].n;
            left_aabb += bins[idx].aabb;
        }
    }

    if (min_cost >= n) return std::make_pair(NAI, NAI);

    char d;
    IdxType sidx;
    std::tie(d, sidx) = split;

    float min = node.aabb.min[d];
    float max = node.aabb.max[d];
    for (Bin & bin : bins) {
        bin = {0, {Vec3fType(inf), Vec3fType(-inf)}};
    }
    for (std::size_t i = node.first; i < node.last; ++i) {
        AABB const & aabb = aabbs[indices[i]];
        char idx = ((mid(aabb, d) - min) / (max - min)) * (NUM_BINS - 1);
        bins[idx].aabb += aabb;
        bins[idx].n += 1;
        bin[i - node.first] = idx;
    }

    IdxType l = node.first;
    IdxType r = node.last - 1;
    while (l < r) {
        if (bin[l - node.first] < sidx) {
            l += 1;
            continue;
        }
        if (bin[r - node.first] >= sidx) {
            r -= 1;
            continue;
        }
        std::swap(bin[l - node.first], bin[r - node.first]);
        std::swap(indices[l], indices[r]);
    }
    assert(l == r);
    std::size_t m = bin[(l&r) - node.first] >= sidx ? (l&r) : (l&r) + 1;

    node.left = create_node(node.first, m);
    node.right = create_node(m, node.last);
    for (std::size_t idx = 0; idx < NUM_BINS; ++idx) {
        if (idx < sidx) {
            nodes[node.left].aabb += bins[idx].aabb;
        } else {
            nodes[node.right].aabb += bins[idx].aabb;
        }
    }

    return std::make_pair(node.left, node.right);
}

template <typename IdxType, typename Vec3fType>
std::pair<typename BVHTree<IdxType, Vec3fType>::Node::ID, typename BVHTree<IdxType, Vec3fType>::Node::ID>
BVHTree<IdxType, Vec3fType>::ssplit(typename Node::ID node_id, std::vector<AABB> const & aabbs) {
    Node & node = nodes[node_id];
    IdxType n = node.last - node.first;

    float min_cost = inf;
    std::pair<char, IdxType> split;
    std::vector<AABB> right_aabbs(n);
    for (char d = 0; d < 3; ++d) {
        std::sort(indices.begin() + node.first, indices.begin() + node.last,
            [&aabbs, d] (IdxType first, IdxType second) -> bool {
                return mid(aabbs[first], d) < mid(aabbs[second], d)
                    || (mid(aabbs[first], d) == mid(aabbs[second], d)
                        && first < second);
            }
        );

        right_aabbs[n - 1] = aabbs[indices[node.last - 1]];
        for (IdxType i = node.last - 1; i > node.first; --i) {
            right_aabbs[i - 1 - node.first] = aabbs[indices[i - 1]]
                + right_aabbs[i - node.first];
        }
        node.aabb = right_aabbs[0];

        AABB left_aabb = aabbs[indices[node.first]];
        for (IdxType i = node.first + 1; i < node.last; ++i) {
            IdxType nl = i - node.first;
            IdxType nr = n - nl;
            float cost = (surface_area(left_aabb) / surface_area(node.aabb) * nl
            + surface_area(right_aabbs[nl]) / surface_area(node.aabb) * nr);
            if (cost <= min_cost) {
                min_cost = cost;
                split = std::make_pair(d, i);
            }

            left_aabb += aabbs[indices[i]];
        }
    }

    if (min_cost >= n) return std::make_pair(NAI, NAI);

    char d;
    IdxType i;
    std::tie(d, i) = split;
    std::sort(indices.begin() + node.first, indices.begin() + node.last,
        [&aabbs, d] (std::size_t first, std::size_t second) -> bool {
            return mid(aabbs[first], d) < mid(aabbs[second], d)
                || (mid(aabbs[first], d) == mid(aabbs[second], d)
                    && first < second);
        }
    );

    node.left = create_node(node.first, i);
    node.right = create_node(i, node.last);
    return std::make_pair(node.left, node.right);
}

template <typename IdxType, typename Vec3fType>
BVHTree<IdxType, Vec3fType>::BVHTree(std::vector<IdxType> const & faces,
    std::vector<Vec3fType> const & vertices, int max_threads) : num_nodes(0) {

    std::size_t num_faces = faces.size() / 3;
    std::vector<AABB> aabbs(num_faces);
    std::vector<Tri> ttris(num_faces);

    /* Initialize vector with upper bound of nodes. */
    nodes.resize(2 * num_faces - 1);

    /* Initialize root node. */
    Node & root = nodes[create_node(0, num_faces)];
    for (std::size_t i = 0; i < aabbs.size(); ++i) {
        ttris[i].a = vertices[faces[i * 3 + 0]];
        ttris[i].b = vertices[faces[i * 3 + 1]];
        ttris[i].c = vertices[faces[i * 3 + 2]];

        calculate_aabb(ttris[i], &aabbs[i]);
        root.aabb += aabbs[i];
    }
    indices.resize(aabbs.size());
    for (std::size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    std::atomic<int> num_threads(max_threads);
    split(0, aabbs, &num_threads);

    tris.resize(ttris.size());
    for (std::size_t i = 0; i < indices.size(); ++i) {
        tris[i] = ttris[indices[i]];
    }

    nodes.resize(num_nodes);
}

template <typename IdxType, typename Vec3fType> bool
BVHTree<IdxType, Vec3fType>::intersect(Ray const & ray, typename Node::ID node_id, Hit * hit) const {
    Node const & node = nodes[node_id];
    bool ret = false;
    for (std::size_t i = node.first; i < node.last; ++i) {
        float t;
        Vec3fType bcoords;
        if (acc::intersect(ray, tris[i], &t, &bcoords)) {
            if (t > hit->t) continue;
            hit->idx = indices[i];
            hit->t = t;
            hit->bcoords = bcoords;
            ret = true;
        }
    }
    return ret;
}

template <typename IdxType, typename Vec3fType> bool
BVHTree<IdxType, Vec3fType>::intersect(Ray ray, Hit * hit_ptr) const {
    Hit hit;
    hit.t = inf;

    typename Node::ID node_id = 0;
    std::stack<typename Node::ID> s;
    while (true) {
        Node const & node = nodes[node_id];
        if (node.left != NAI && node.right != NAI) {
            float tmin_left, tmin_right;
            bool left = acc::intersect(ray, nodes[node.left].aabb, &tmin_left);
            bool right = acc::intersect(ray, nodes[node.right].aabb, &tmin_right);
            if (left && right) {
                if (tmin_left < tmin_right) {
                    s.push(node.right);
                    node_id = node.left;
                } else {
                    s.push(node.left);
                    node_id = node.right;
                }
            } else {
                if (right) node_id = node.right;
                if (left) node_id = node.left;
            }

            if (!left && !right) {
                if (s.empty()) break;
                node_id = s.top(); s.pop();
            }
        } else {
            if (intersect(ray, node_id, &hit)) {
                ray.tmax = hit.t;
            }

            if (s.empty()) break;
            node_id = s.top(); s.pop();
        }
    }

    if (hit.t < inf) {
        if (hit_ptr != nullptr) {
            *hit_ptr = hit;
        }
        return true;
    } else {
        return false;
    }
}

template <typename IdxType, typename Vec3fType> Vec3fType
BVHTree<IdxType, Vec3fType>::closest_point(Vec3fType vertex, typename Node::ID node_id) const {
    Node const & node = nodes[node_id];

    Vec3fType closest;
    float dist = inf;

    for (std::size_t i = node.first; i < node.last; ++i) {
        Vec3fType closest_tri = acc::closest_point(vertex, tris[i]);
		float     dist_tri    = glm::length2(closest_tri - vertex);
		//.square_norm();
        if (dist_tri < dist) {
            closest = closest_tri;
            dist = dist_tri;
        }
    }

    return closest;
}

template <typename IdxType, typename Vec3fType> Vec3fType
BVHTree<IdxType, Vec3fType>::closest_point(Vec3fType vertex, float max_dist) const {

    float dist = max_dist * max_dist;
    Vec3fType closest;

    typename Node::ID node_id = 0;
    std::stack<typename Node::ID> s;
    while (true) {
        Node const & node = nodes[node_id];
        if (node.left != NAI && node.right != NAI) {
            Vec3fType closest_left = acc::closest_point(vertex, nodes[node.left].aabb);
            Vec3fType closest_right = acc::closest_point(vertex, nodes[node.right].aabb);
			float     dmin_left     = glm::length2(closest_left - vertex);
			float dmin_right = glm::length2(closest_right - vertex);//.square_norm();
            bool left = dmin_left < dist;
            bool right = dmin_right < dist;
            if (left && right) {
                if (dmin_left < dmin_right) {
                    s.push(node.right);
                    node_id = node.left;
                } else {
                    s.push(node.left);
                    node_id = node.right;
                }
            } else {
                if (right) node_id = node.right;
                if (left) node_id = node.left;
            }

            if (!left && !right) {
                if (s.empty()) break;
                node_id = s.top(); s.pop();
            }
        } else {
            Vec3fType closest_leaf = closest_point(vertex, node_id);
			float     dist_leaf    = glm::length2(closest_leaf - vertex);
			//.square_norm();
            if (dist_leaf < dist) {
                dist = dist_leaf;
                closest = closest_leaf;
            }

            if (s.empty()) break;
            node_id = s.top(); s.pop();
        }
    }

    return closest;
}

ACC_NAMESPACE_END

#endif /* ACC_BVHTREE_HEADER */
