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

std::vector< std::pair< std::string, Mesh > >heads;
size_t head_bits = 0;
std::vector< std::pair< std::string, Mesh > >bodies;
size_t body_bits = 0;
std::vector< std::pair< std::string, Mesh > >arms;
size_t arm_bits = 0;
std::vector< std::pair< std::string, Mesh > >legs;
size_t leg_bits = 0;
std::vector< std::pair< std::string, Mesh > >hats;
size_t hat_bits = 0;

std::unordered_map< std::string, Scene::Transform * > transforms;

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > npc_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("npcs.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > npcs_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("npcs.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		transforms[mesh_name] = transform;
		/*
		Mesh const &mesh = npc_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		std::cout << transform->position.x << ", " << transform->position.y << ", " << transform->position.z << std::endl;


		
		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		glBindVertexArray(drawable.pipeline.vao);
		*/
	});
});

PlayMode::PlayMode() : scene(*npcs_scene) {
	//organize meshes for convenience
	for (auto data : npc_meshes->meshes) {
		std::cout << data.first << std::endl;
		if (data.first.find("head") != std::string::npos) {
			heads.emplace_back(data);
		} else if (data.first.find("body") != std::string::npos) {
			bodies.emplace_back(data);
		} else if (data.first.find("arm") != std::string::npos) {
			arms.emplace_back(data);
		} else if (data.first.find("leg") != std::string::npos) {
			legs.emplace_back(data);
		} else if (data.first.find("hat") != std::string::npos) {
			hats.emplace_back(data);
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

	NPC *npc = new NPC(head_bits, body_bits, arm_bits, leg_bits, hat_bits);

	do  {
		npc->selection = npc_dist(rng);
	} while (
		used_npcs.find(npc->selection) != used_npcs.end() ||
		npc->get_head() >= heads.size() ||
		npc->get_body() >= bodies.size() ||
		npc->get_arm_l() >= arms.size() ||
		npc->get_arm_r() >= arms.size() ||
		npc->get_legs() >= legs.size() ||
		npc->get_hat() >= hats.size()
	);
	npcs.emplace_back(npc);
	used_npcs.emplace(npc->selection);

	// create the transform and drawable with the selected information

	npc->head_transform = new Scene::Transform();
	npc->body_transform= new Scene::Transform();
	npc->arm_l_transform= new Scene::Transform();
	npc->arm_r_transform= new Scene::Transform();
	npc->legs_transform= new Scene::Transform();
	npc->hat_transform= new Scene::Transform();

	*(npc->head_transform) = *transforms[heads.at(npc->get_head()).first];
	*(npc->body_transform) = *transforms[bodies.at(npc->get_body()).first];
	*(npc->arm_l_transform) = *transforms[arms.at(npc->get_arm_l()).first];
	*(npc->arm_r_transform) = *transforms[arms.at(npc->get_arm_r()).first];
	*(npc->legs_transform) = *transforms[legs.at(npc->get_legs()).first];
	*(npc->hat_transform) = *transforms[hats.at(npc->get_hat()).first];

	npc->head_transform->parent = npc->arm_l_transform->parent = npc->arm_r_transform->parent = npc->legs_transform->parent = npc->body_transform;
	npc->hat_transform->parent = npc->head_transform;

	scene.drawables.emplace_back(npc->head_transform);
	Scene::Drawable &head_drawable = scene.drawables.back();

	scene.drawables.emplace_back(npc->body_transform);
	Scene::Drawable &body_drawable = scene.drawables.back();

	scene.drawables.emplace_back(npc->arm_l_transform);
	Scene::Drawable &arm_l_drawable = scene.drawables.back();
	
	scene.drawables.emplace_back(npc->arm_r_transform);
	Scene::Drawable &arm_r_drawable = scene.drawables.back();

	scene.drawables.emplace_back(npc->legs_transform);
	Scene::Drawable &legs_drawable = scene.drawables.back();

	scene.drawables.emplace_back(npc->hat_transform);
	Scene::Drawable &hat_drawable = scene.drawables.back();

	head_drawable.pipeline = lit_color_texture_program_pipeline;
	head_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	head_drawable.pipeline.type = heads.at(npc->get_head()).second.type;
	head_drawable.pipeline.start = heads.at(npc->get_head()).second.start;
	head_drawable.pipeline.count = heads.at(npc->get_head()).second.count;
	glBindVertexArray(head_drawable.pipeline.vao);

	body_drawable.pipeline = lit_color_texture_program_pipeline;
	body_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	body_drawable.pipeline.type = bodies.at(npc->get_body()).second.type;
	body_drawable.pipeline.start = bodies.at(npc->get_body()).second.start;
	body_drawable.pipeline.count = bodies.at(npc->get_body()).second.count;
	glBindVertexArray(body_drawable.pipeline.vao);
	
	arm_l_drawable.pipeline = lit_color_texture_program_pipeline;
	arm_l_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	arm_l_drawable.pipeline.type = arms.at(npc->get_arm_l()).second.type;
	arm_l_drawable.pipeline.start = arms.at(npc->get_arm_l()).second.start;
	arm_l_drawable.pipeline.count = arms.at(npc->get_arm_l()).second.count;
	glBindVertexArray(arm_l_drawable.pipeline.vao);

	arm_r_drawable.pipeline = lit_color_texture_program_pipeline;
	arm_r_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	arm_r_drawable.pipeline.type = arms.at(npc->get_arm_r()).second.type;
	arm_r_drawable.pipeline.start = arms.at(npc->get_arm_r()).second.start;
	arm_r_drawable.pipeline.count = arms.at(npc->get_arm_r()).second.count;
	npc->arm_r_transform->scale.x *= -1;
	npc->arm_r_transform->position.x *= -1;
	glBindVertexArray(arm_r_drawable.pipeline.vao);

	legs_drawable.pipeline = lit_color_texture_program_pipeline;
	legs_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	legs_drawable.pipeline.type = legs.at(npc->get_legs()).second.type;
	legs_drawable.pipeline.start = legs.at(npc->get_legs()).second.start;
	legs_drawable.pipeline.count = legs.at(npc->get_legs()).second.count;
	glBindVertexArray(legs_drawable.pipeline.vao);

	hat_drawable.pipeline = lit_color_texture_program_pipeline;
	hat_drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	hat_drawable.pipeline.type = hats.at(npc->get_hat()).second.type;
	hat_drawable.pipeline.start = hats.at(npc->get_hat()).second.start;
	hat_drawable.pipeline.count = hats.at(npc->get_hat()).second.count;
	glBindVertexArray(hat_drawable.pipeline.vao);


	npc->body_transform->position += glm::vec3(2.f);

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
