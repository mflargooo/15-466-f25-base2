#include "Mode.hpp"

#include "Scene.hpp"
#include "Mesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <set>

struct NPC {
	size_t head_bits, body_bits, arm_bits, leg_bits, hat_bits;
	size_t selection;

	Scene::Transform *head_transform = nullptr;
	Scene::Transform *body_transform = nullptr;
	Scene::Transform *arm_l_transform = nullptr;
	Scene::Transform *arm_r_transform = nullptr;
	Scene::Transform *legs_transform = nullptr;
	Scene::Transform *hat_transform = nullptr;

	size_t get_head() {
		return selection & ~(~(size_t)0x0 << head_bits);
	}

	size_t get_body() {
		size_t offset = head_bits;
		return (selection & (~(~(size_t)0x0 << body_bits) << offset)) >> offset;
	}

	size_t get_arm_l() {
		size_t offset = head_bits + body_bits;
		return (selection & (~(~(size_t)0x0 << arm_bits) << offset)) >> offset;
	}

	size_t get_arm_r() {
		size_t offset = head_bits + body_bits + arm_bits;
		return (selection & (~(~(size_t)0x0 << arm_bits) << offset)) >> offset;
	}

	size_t get_legs() {
		size_t offset = head_bits + body_bits + arm_bits * 2;
		return (selection & (~(~(size_t)0x0 << leg_bits) << offset)) >> offset;
	}

	size_t get_hat() {
		size_t offset = head_bits + body_bits + arm_bits * 2 + leg_bits;
		return (selection & (~(~(size_t)0x0 << hat_bits) << offset)) >> offset;
	}

	NPC(size_t head_bits, size_t body_bits, size_t arm_bits, size_t leg_bits, size_t hat_bits) :
		head_bits(head_bits), body_bits(body_bits), arm_bits(arm_bits), leg_bits(leg_bits), hat_bits(hat_bits) {};
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	std::set< size_t > used_npcs;
	std::vector< NPC * > npcs;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
