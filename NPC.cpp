#include "NPC.hpp"

#include <iostream>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <thread>

uint8_t NPCCreator::NPC::get_from_selection(std::string part) {
    if (creator->bit_offset_of.find(part) == creator->bit_offset_of.end()){
        throw new std::runtime_error("Part " + part + " does not exist in this NPCCreator");
    }

    size_t bits = ~(~(size_t)0x0 << (size_t)(creator->bits_of[creator->tag_of[part]])); 
    size_t offset = (size_t)(creator->bit_offset_of[part]);
    return (uint8_t) ((selection & (bits << offset)) >> offset);
}

void NPCCreator::initialize() {
    for (auto part : part_names) {
        for (auto tag : tags) {
            if (part.find(tag) != std::string::npos) {
                tag_of[part] = tag;
                break;
            }
        }
        if (tag_of[part] == "") {
            throw new std::runtime_error("Invalid part_names list! Part " + part + " has no corresponding tag!");
        }
    }
}

void NPCCreator::register_data(const std::map< std::string, Mesh > *meshes, std::map< std::string, Scene::Transform * > *transforms) {
    for (auto mesh : *meshes) {
        bool categorized = false;
        for (auto tag : tags) {
            if (mesh.first.find(tag) == std::string::npos) continue;

            auto transform = (*transforms).find(mesh.first);
            if (transform != (*transforms).end()) {
                data[tag][mesh.first].first = &mesh.second;
                data[tag][mesh.first].second = transform->second;
                std::cout << mesh.first << std::endl;
                selection_to_mesh_name[tag][(uint8_t)(selection_to_mesh_name[tag].size())] = mesh.first;

                if (num_of.find(tag) == num_of.end()) num_of[tag] = 0;
                num_of[tag] += 1;
                categorized = true;
                break;
            } else {
                throw new std::runtime_error("Cannot categorize mesh named " + mesh.first + ". Has no corresponding transform!");
            }
        }

        if (!categorized) {
            std::cout << "Skipping mesh " << mesh.first << " since no patterns were found!" << std::endl;
        }
    }

    for (auto pair : num_of) {
        bits_of[pair.first] = (uint8_t) (std::ceilf(std::log2f((float)(pair.second))));
        if (bits_of[pair.first] <= 0) bits_of[pair.first] = 1;
        
        if (total_possible_npcs == 0) total_possible_npcs = 1;
        total_possible_npcs *= pair.second;
    }

    bit_offset_of[part_names[0]] = 0;
    for (size_t i = 1; i < part_names.size(); i++) {
        bit_offset_of[part_names[i]] = bit_offset_of[part_names[i-1]] + bits_of[tag_of[part_names[i-1]]];
    }
}

std::vector< NPCCreator::NPC > *NPCCreator::create_npcs(size_t amount) {
    assert(total_possible_npcs > 0 && amount + used_npcs.size() < total_possible_npcs);

    std::vector< NPC > *ret = new std::vector< NPC >();
    ret->reserve(amount);
    
    for (size_t i = 0; i < amount; i++) {
        ret->emplace_back(this);
        NPC *npc = &(ret->back());
        assert(npc);

        size_t selection;
        std::string part;
        std::string tag;
        do  {
            selection = 0;
            for (size_t j = 0; j < part_names.size(); j++) {
                part = part_names[part_names.size() - j - 1];
                tag = tag_of[part_names[part_names.size() - j - 1]];
                std::uniform_int_distribution< size_t > npc_dist = std::uniform_int_distribution< size_t >(0, (size_t)(num_of[tag] - 1));
                selection = (selection << (size_t)bits_of[tag]) | (npc_dist(rng) & ~(~(size_t)0 << (size_t)bits_of[tag]));
            }
        } while (used_npcs.find(selection) != used_npcs.end());
        
        npc->selection = selection;
        used_npcs.emplace(npc->selection);

        std::cout << "we are here" << std::endl;
        for (size_t j = 0; j < part_names.size(); j++) {
            part = part_names[j];
            tag = tag_of[part];

            auto mesh_name = selection_to_mesh_name[tag][npc->get_from_selection(part)];
            auto part_info = npc->creator->data[tag][mesh_name];
            
            assert(part_info.first && part_info.second);

            npc->parts[part].first = part_info.first;
			std::cout << part << ", " << mesh_name << ": " << part_info.first->type << ", " << part_info.first->start << ", " << part_info.first->count << std::endl;
            npc->parts[part].second = new Scene::Transform();
            
            *(npc->parts[part].second) = *part_info.second;
        }
            std::cout << "we are not here :(" << std::endl;
    }

    return ret;
}