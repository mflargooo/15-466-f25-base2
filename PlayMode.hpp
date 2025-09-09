#include "Mode.hpp"

#include "Scene.hpp"
#include "Mesh.hpp"
#include "NPC.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <set>
#include <random>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// c++ random generation from stackoverflow: https://stackoverflow.com/questions/7560114/random-number-c-in-some-range
    std::random_device rd;
    std::mt19937 rng{rd()};

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	struct Wheel {
		float velocity = 0;
		int scrolled = 0;
	} wheel;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// npcs

	std::vector< NPCCreator::NPC > npcs;
	std::vector< NPCCreator::NPCInfo > npc_infos;
	std::vector< float > npc_radii;

	const uint8_t TARGET_COUNT = 5;
	const float UI_SPACING = 1.5f;
	struct Targets {
		std::list< std::map< std::string, Scene::Drawable * >> drawables;
		std::vector< Scene::Transform *> ui_transforms;
		std::vector< size_t > npc_idxs;
		std::vector< bool > alive;
	} targets;
	
	//camera:
	Scene::Camera *camera = nullptr;
	struct CameraInfo {
		float yaw = 0.f;
		float pitch = 0.f;

		float zoom_speed = 5.f;
		float dist_from_player = 0.f;
		const float MIN_DIST_FROM_PLAYER = 2.f;
		const float MAX_DIST_FROM_PLAYER = 100.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0.0f, 3.0f);
	} cam_info;

	//player
	struct Player {
		Scene::Drawable *drawable;
		Scene::Drawable *saucer_drawable;
		Scene::Transform *transform;
		float collision_radius = 2.f;
		bool dead = false;
	} player;

	bool score_displayed = false;
};
