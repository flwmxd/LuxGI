/*
 * Copyright (C) 2015, Nils Moehrle
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef ACC_KDTREE_HEADER
#define ACC_KDTREE_HEADER

#include <queue>
#include <stack>
#include <limits>
#include <atomic>
#include <thread>
#include <algorithm>

#include <math/vector.h>

#include "defines.h"

ACC_NAMESPACE_BEGIN

template <uint16_t K, typename IdxType = unsigned>
class KDTree {
public:
    #define NAI std::numeric_limits<IdxType>::max()
private:

    std::vector<math::Vector<float, K> > const & vertices;
    struct Node {
        typedef IdxType ID;
        decltype(K) d;
        IdxType first;
        IdxType last;
        IdxType vertex_id;
        Node::ID left;
        Node::ID right;
    };

    std::atomic<IdxType> num_nodes;
    std::vector<Node> nodes;
    typename Node::ID create_node(decltype(K) d, IdxType first, IdxType last) {
        typename Node::ID node_id = num_nodes++;
        Node & node = nodes[node_id];
        node.first = first;
        node.last = last;
        node.left = NAI;
        node.right = NAI;
        node.vertex_id = NAI;
        node.d = d;
        return node_id;
    }

    std::pair<typename Node::ID, typename Node::ID>
    ssplit(typename Node::ID node_id, std::vector<IdxType> * indices);

    void split(typename Node::ID node_id, std::vector<IdxType> * indices,
        std::atomic<int> * num_threads);
public:
    KDTree(std::vector<math::Vector<float, K> > const & vertices,
        int max_threads = std::thread::hardware_concurrency());

    std::pair<IdxType, float>
    find_nn(math::Vector<float, K> point,
        float max_dist = std::numeric_limits<float>::infinity()) const;

    std::vector<std::pair<IdxType, float> >
    find_nns(math::Vector<float, K> point, std::size_t n,
        float max_dist = std::numeric_limits<float>::infinity()) const;
};

template <uint16_t K, typename IdxType>
KDTree<K, IdxType>::KDTree(std::vector<math::Vector<float, K> > const & vertices,
    int max_threads)
    : vertices(vertices), num_nodes(0) {

    std::size_t num_vertices = vertices.size();
    nodes.resize(num_vertices);

    std::vector<IdxType> indices(num_vertices);
    for (std::size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    std::atomic<int> num_threads(max_threads);
    split(create_node(0, 0, num_vertices), &indices, &num_threads);
}

template <uint16_t K, typename IdxType>
void KDTree<K, IdxType>::split(typename Node::ID node_id, std::vector<IdxType> * indices, std::atomic<int> * num_threads) {
    typename Node::ID left, right;
    if ((*num_threads -= 1) >= 1) {
        std::tie(left, right) = ssplit(node_id, indices);
        if (left != NAI && right != NAI) {
            std::thread other(&KDTree::split, this, left, indices, num_threads);
            split(right, indices, num_threads);
            other.join();
        } else {
            if (left != NAI) split(left, indices, num_threads);
            if (right != NAI) split(right, indices, num_threads);
        }
    } else {
        std::deque<typename Node::ID> queue;
        queue.push_back(node_id);
        while (!queue.empty()) {
            typename Node::ID node_id = queue.front(); queue.pop_front();
            std::tie(left, right) = ssplit(node_id, indices);
            if (left != NAI) queue.push_back(left);
            if (right != NAI) queue.push_back(right);
        }
    }
    *num_threads += 1;
}

template <uint16_t K, typename IdxType>
std::pair<typename KDTree<K, IdxType>::Node::ID, typename KDTree<K, IdxType>::Node::ID>
KDTree<K, IdxType>::ssplit(typename Node::ID node_id, std::vector<IdxType> * indices) {
    Node & node = nodes[node_id];
    decltype(K) d = node.d;
    std::sort(indices->data() + node.first, indices->data() + node.last,
        [this, d] (IdxType a, IdxType b) -> bool {
            return vertices[a][d] < vertices[b][d];
        }
    );
    d = (d + 1) % K;
    IdxType mid = (node.last + node.first) / 2;
    node.vertex_id = indices->at(mid);
    if (mid - node.first > 0) {
        node.left = create_node(d, node.first, mid);
    }
    if (node.last - (mid + 1) > 0) {
        node.right = create_node(d, mid + 1, node.last);
    }
    return std::make_pair(node.left, node.right);
}

template <uint16_t K, typename IdxType>
std::pair<IdxType, float>
KDTree<K, IdxType>::find_nn(math::Vector<float, K> point, float max_dist) const {
    return find_nns(point, 1, max_dist)[0];
}

template <uint16_t K, typename IdxType>
std::vector<std::pair<IdxType, float> >
KDTree<K, IdxType>::find_nns(math::Vector<float, K> vertex, std::size_t n, float max_dist) const {

    std::pair<IdxType, float> nn = std::make_pair(NAI, max_dist);
    std::vector<std::pair<IdxType, float> > nns(n, nn);

    std::stack<std::pair<typename Node::ID, bool> > s;
    s.emplace(0, true);
    while (!s.empty()) {
        typename Node::ID node_id;
        bool down;
        std::tie(node_id, down) = s.top();
        s.pop();

        if (node_id == NAI) continue;

        Node const & node = nodes[node_id];

        float diff = vertex[node.d] - vertices[node.vertex_id][node.d];
        if (down) {
            float dist = (vertex - vertices[node.vertex_id]).norm();
            if (dist < max_dist) {
                nns.emplace_back(node.vertex_id, dist);
                std::sort(nns.begin(), nns.end(),
                    [] (std::pair<IdxType, float> a, std::pair<IdxType, float> b) -> bool {
                        return a.second < b.second;
                    }
                );
                nns.pop_back();
                max_dist = nns.back().second;
            }

            if (node.left == NAI && node.right == NAI) continue;

            s.emplace(node_id, false);
            if (diff < 0.0f) {
                s.emplace(node.left, true);
            } else {
                s.emplace(node.right, true);
            }
        } else {
            if (std::abs(diff) >= max_dist) continue;

            if (diff < 0.0f) {
                s.emplace(node.right, true);
            } else {
                s.emplace(node.left, true);
            }
        }
    }
    return nns;
}

ACC_NAMESPACE_END

#endif /* ACC_KDTREE_HEADER */
