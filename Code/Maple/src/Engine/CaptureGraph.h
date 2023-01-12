//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RHI/Texture.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace maple
{
	namespace capture_graph
	{
		enum class NodeType
		{
			RenderPass,
			Image,
		};

		struct GraphNode
		{
			std::string                                    name;
			NodeType                                       nodeType;
			std::unordered_set<std::shared_ptr<GraphNode>> inputs;
			std::unordered_set<std::shared_ptr<GraphNode>> outputs;
			std::shared_ptr<Texture>                       texture;
		};

		namespace component
		{
			struct RenderGraph
			{
				std::unordered_map<std::string, std::shared_ptr<GraphNode>> nodes;
			};
		}        // namespace component

		inline auto getRenderPassNode(const std::string &name, component::RenderGraph &graph)
		{
			if (auto node = graph.nodes.find(name); node != graph.nodes.end())
			{
				return node->second;
			}
			auto node      = std::make_shared<GraphNode>();
			node->nodeType = NodeType::RenderPass;
			node->name     = name;
			return graph.nodes.emplace(name, node).first->second;
		}

		inline auto getImageNode(const std::shared_ptr<Texture> &texture, component::RenderGraph &graph)
		{
			if (auto node = graph.nodes.find(texture->getName()); node != graph.nodes.end())
			{
				return node->second;
			}
			auto node      = std::make_shared<GraphNode>();
			node->nodeType = NodeType::Image;
			node->texture  = texture;
			node->name     = texture->getName();
			return graph.nodes.emplace(texture->getName(), node).first->second;
		}

		inline auto input(const std::string &name, component::RenderGraph &graph, const std::vector<std::shared_ptr<Texture>> &lists) -> void
		{
			auto renderPassNode = getRenderPassNode(name, graph);
			for (auto &t : lists)
			{
				if (t != nullptr && t->getType() != TextureType::Color3D && t->getType() != TextureType::Cube)
					renderPassNode->inputs.emplace(getImageNode(t, graph));
			}
		}

		inline auto output(const std::string &name, component::RenderGraph &graph, const std::vector<std::shared_ptr<Texture>> &lists) -> void
		{
			auto renderPassNode = getRenderPassNode(name, graph);
			for (auto &t : lists)
			{
				if (t != nullptr && t->getType() != TextureType::Color3D && t->getType() != TextureType::Cube)
					renderPassNode->outputs.emplace(getImageNode(t, graph));
			}
		}
	}        // namespace capture_graph
};           // namespace maple
