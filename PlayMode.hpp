#include "Mode.hpp"

#include "Scene.hpp"
#include "Mesh.hpp"
#include "NPC.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <set>

struct Player {
	Scene::Drawable *drawable;
	Scene::Transform *transform;
};

struct CameraInfo {
	float yaw = 0;
	float pitch = 0;

	float zoom_speed = 2.f;
	float dist_from_player = 0;
	const float MIN_DIST_FROM_PLAYER = 2.f;
	const float MAX_DIST_FROM_PLAYER = 30.0f;
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

	struct Wheel {
		float velocity = 0;
		int scrolled = 0;
	} wheel;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	std::list< NPCCreator::NPC > npcs;
	
	//camera:
	Scene::Camera *camera = nullptr;
	CameraInfo cam_info;

	Player player;

	//player

};
