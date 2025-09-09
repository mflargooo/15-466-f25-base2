#include "Mode.hpp"

#include "Scene.hpp"
#include "Mesh.hpp"
#include "NPC.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <set>

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
	const float UI_SPACING = 1.2f;
	struct Targets {
		std::list< std::map< std::string, Scene::Drawable * >> drawables;
		std::list< Scene::Transform *> ui_transforms;
		std::set< size_t > indices;
	} targets;
	
	//camera:
	Scene::Camera *camera = nullptr;
	struct CameraInfo {
		float yaw = 0;
		float pitch = 0;

		float zoom_speed = 5.f;
		float dist_from_player = 0.f;
		const float MIN_DIST_FROM_PLAYER = 2.f;
		const float MAX_DIST_FROM_PLAYER = 100.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0.0f, 3.0f);
	} cam_info;

	//player
	struct Player {
		Scene::Drawable *drawable;
		Scene::Transform *transform;
		float collision_radius = 2.f;
		bool dead = false;
	} player;


};
