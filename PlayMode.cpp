#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <vector>

std::vector< Mesh >heads;
size_t head_bits = 0;
std::vector< Mesh >bodies;
size_t body_bits = 0;
std::vector< Mesh >arms;
size_t arm_bits = 0;
std::vector< Mesh >legs;
size_t leg_bits = 0;
std::vector< Mesh >hats;
size_t hat_bits = 0;

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > npc_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("npcs.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > npcs_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("npcs.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = npc_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		glBindVertexArray(drawable.pipeline.vao);
	});
});

PlayMode::PlayMode() : scene(*npcs_scene) {
	//get references to mesh information for convenience
	for (auto data : npc_meshes->meshes) {
		std::cout << data.first << std::endl;
		if (data.first.find("head") != std::string::npos) {
			heads.emplace_back(data.second);
		} else if (data.first.find("body") != std::string::npos) {
			bodies.emplace_back(data.second);
		} else if (data.first.find("arm") != std::string::npos) {
			arms.emplace_back(data.second);
		} else if (data.first.find("leg") != std::string::npos) {
			legs.emplace_back(data.second);
		} else if (data.first.find("hat") != std::string::npos) {
			hats.emplace_back(data.second);
		} else if (data.first.find("neck") != std::string::npos) {
			continue;
		} else {
			throw new std::runtime_error("Cannot categorize mesh named " + data.first);
		}
	}

	head_bits = (size_t)std::ceilf(std::log2f((float) heads.size()));
	body_bits = (size_t)std::ceilf(std::log2f((float) bodies.size()));
	arm_bits = (size_t)std::ceilf(std::log2f((float) arms.size()));
	leg_bits = (size_t)std::ceilf(std::log2f((float) legs.size()));
	hat_bits = (size_t)std::ceilf(std::log2f((float) hats.size()));

	std::cout << head_bits + body_bits + arm_bits + leg_bits + hat_bits << std::endl;
	size_t total_npcs = heads.size() * bodies.size() * arms.size() * arms.size() * legs.size() * hats.size();
	assert(total_npcs > 0);

	assert(head_bits + body_bits + arm_bits + arm_bits + leg_bits + hat_bits <= std::ceilf(std::log2f((float) total_npcs)));

	// c++ random generation from stackoverflow: https://stackoverflow.com/questions/7560114/random-number-c-in-some-range
	std::random_device rd;
	std::mt19937 rng{rd()};
	std::uniform_int_distribution< size_t > npc_dist(0, total_npcs - 1);

	size_t npc_selection;
	do  {
		npc_selection = npc_dist(rng);
	} while (
		used_npcs.find(npc_selection) != used_npcs.end() ||
		(npc_selection & ~(~0x00 << head_bits)) >= heads.size() ||
		((npc_selection & (~(~0x00 << body_bits) << head_bits)) >> head_bits) >= bodies.size() ||
		((npc_selection & (~(~0x00 << arm_bits) << (head_bits + body_bits))) >> (head_bits + body_bits)) >= arms.size() ||
		((npc_selection & (~(~0x00 << arm_bits) << (head_bits + body_bits + arm_bits))) >> (head_bits + body_bits + arm_bits)) >= arms.size() ||
		((npc_selection & (~(~0x00 << leg_bits) << (head_bits + body_bits + arm_bits * 2))) >> (head_bits + body_bits + arm_bits * 2)) >= legs.size() ||
		((npc_selection & (~(~0x00 << hat_bits) << (head_bits + body_bits + arm_bits * 2 + leg_bits))) >> (head_bits + body_bits + arm_bits * 2 + leg_bits)) >= legs.size()
	);

	std::cout << npc_selection << std::endl;
	std::cout << (npc_selection & ~((~0x00) << head_bits));
	std::cout << ", " << ((npc_selection & ((~((~0x00) << body_bits)) << head_bits)) >> head_bits);
	std::cout << ", " << ((npc_selection & (~((~0x00) << arm_bits) << (head_bits + body_bits))) >> (head_bits + body_bits));
	std::cout << ", " << ((npc_selection & (~((~0x00) << arm_bits) << (head_bits + body_bits + arm_bits))) >> (head_bits + body_bits + arm_bits));
	std::cout << ", " << ((npc_selection & (~((~0x00) << leg_bits) << (head_bits + body_bits + arm_bits * 2))) >> (head_bits + body_bits + arm_bits * 2));
	std::cout << ", " << ((npc_selection & (~((~0x00) << hat_bits) << (head_bits + body_bits + arm_bits * 2 + leg_bits))) >> (head_bits + body_bits + arm_bits * 2 + leg_bits)) << std::endl;
	std::cout << heads.size() << ", " << bodies.size() << ", " << arms.size() << ", " << arms.size() << ", " << legs.size() << ", " << hats.size() << std::endl;
	
	std::cout << npc_selection << std::endl;

	NPC npc = NPC(head_bits, body_bits, arm_bits, leg_bits, hat_bits, npc_selection);
	npcs.emplace_back(npc);
	
	used_npcs.emplace(npc_selection);

	// create the transform and drawable with the selected information
	Scene::Transform head_tf;
	scene.drawables.emplace_back(&head_tf);
	Scene::Drawable &head_drawable = scene.drawables.back();

	Scene::Transform body_tf;
	scene.drawables.emplace_back(&body_tf);
	Scene::Drawable &body_drawable = scene.drawables.back();

	Scene::Transform arm_l_tf;
	scene.drawables.emplace_back(&arm_l_tf);
	Scene::Drawable &arm_l_drawable = scene.drawables.back();

	Scene::Transform arm_r_tf;
	scene.drawables.emplace_back(&arm_r_tf);
	Scene::Drawable &arm_r_drawable = scene.drawables.back();

	Scene::Transform legs_tf;
	scene.drawables.emplace_back(&legs_tf);
	Scene::Drawable &legs_drawable = scene.drawables.back();

	Scene::Transform hat_tf;
	scene.drawables.emplace_back(&hat_tf);
	Scene::Drawable &hat_drawable = scene.drawables.back();

	head_tf.parent = arm_l_tf.parent = arm_r_tf.parent = legs_tf.parent = &body_tf;
	hat_tf.parent = &head_tf;
	
	head_drawable.pipeline = lit_color_texture_program_pipeline;
	head_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	head_drawable.pipeline.type = heads.at(npc.get_head()).type;
	head_drawable.pipeline.start = heads.at(npc.get_head()).start;
	head_drawable.pipeline.count = heads.at(npc.get_head()).count;

	body_drawable.pipeline = lit_color_texture_program_pipeline;
	body_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	body_drawable.pipeline.type = bodies.at(npc.get_body()).type;
	body_drawable.pipeline.start = bodies.at(npc.get_body()).start;
	body_drawable.pipeline.count = bodies.at(npc.get_body()).count;
	
	arm_l_drawable.pipeline = lit_color_texture_program_pipeline;
	arm_l_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	arm_l_drawable.pipeline.type = arms.at(npc.get_arm_l()).type;
	arm_l_drawable.pipeline.start = arms.at(npc.get_arm_l()).start;
	arm_l_drawable.pipeline.count = arms.at(npc.get_arm_l()).count;

	arm_r_drawable.pipeline = lit_color_texture_program_pipeline;
	arm_r_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	arm_r_drawable.pipeline.type = arms.at(npc.get_arm_r()).type;
	arm_r_drawable.pipeline.start = arms.at(npc.get_arm_r()).start;
	arm_r_drawable.pipeline.count = arms.at(npc.get_arm_r()).count;

	legs_drawable.pipeline = lit_color_texture_program_pipeline;
	legs_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	legs_drawable.pipeline.type = legs.at(npc.get_legs()).type;
	legs_drawable.pipeline.start = legs.at(npc.get_legs()).start;
	legs_drawable.pipeline.count = legs.at(npc.get_legs()).count;

	hat_drawable.pipeline = lit_color_texture_program_pipeline;
	hat_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	hat_drawable.pipeline.type = hats.at(npc.get_hat()).type;
	hat_drawable.pipeline.start = hats.at(npc.get_hat()).start;
	hat_drawable.pipeline.count = hats.at(npc.get_hat()).count;

	// issue is that all these are local and go out of scope, so refernces dont exist

	body_tf.position += glm::vec3(2.f);

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	/*
	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	*/

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);


	glClearColor(.5f, .5f, .5f, 1.0f); // used to set background color...maybe can use it for a day/night cycle?
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
