#pragma once

#include "Mesh.hpp"
#include "Scene.hpp"

#include <random>
#include <set>

struct NPCCreator {
	// c++ random generation from stackoverflow: https://stackoverflow.com/questions/7560114/random-number-c-in-some-range
    std::random_device rd;
    std::mt19937 rng{rd()};

    size_t total_possible_npcs;

    // An existing tag of the NPCCreator instance must be a substring of each part
    std::vector < std::string > tags;
    // The parts of an NPC that this creator constructs
    // each string in parts must have a tag from tags as a substring
    // ideally each part is of the form: {tag}_...
    std::vector < std::string > part_names;

    // part name -> tag
    std::map < std::string, std::string > tag_of;
    // tag -> ceil(log_2(selection_to_mesh_name[tag].size(0)))
    std::map< std::string, uint8_t > bits_of;

    // tag -> (part selection -> mesh name)
    std::map< std::string, std::vector< std::string >> selection_to_mesh_name;

    // the offsets to recover part[i] from NPC.selection
    std::map< std::string, uint8_t > bit_offset_of;

    std::map< std::string, std::pair < const Mesh *, Scene::Transform * > > mesh_templates;

    struct NPCInfo {
        NPCCreator *creator;
        size_t selection;
        // part_name -> mesh_name
        std::map < std::string, std::string > mesh_names;
        uint8_t get_from_selection(std::string part);
        NPCInfo(NPCCreator *creator) : creator(creator) {};
    };

    struct NPC {
        std::map < std::string, Scene::Drawable * > drawables;
    };

    std::set< size_t > used_npcs;

    // I DONT THINK I USE THIS, SO DELETE IT LATER AFTER YOU ARE DONE WITH THIS TASK
	std::vector< NPCInfo > npcs;

    // The bits of selection are ordered as tags are ordered.
    NPCCreator(std::vector< std::string > tags, std::vector< std::string > part_names) : tags(tags), part_names(part_names) {};
    void initialize();
    void register_data(const std::map< std::string, Mesh > *meshes);
    std::vector< NPCInfo > create_npc_infos(size_t amount);
};