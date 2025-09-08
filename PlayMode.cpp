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

NPCCreator npc_creator({ "head", "body", "arm", "legs", "hat" }, { "head", "body", "arm_l", "arm_r", "legs", "hat" });
std::map< std::string, Scene::Transform * > mesh_transforms;

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > npc_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("npcs.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > npcs_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("npcs.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		mesh_transforms[mesh_name] = transform;
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
	npc_creator.initialize();
	npc_creator.register_data(&(npc_meshes->meshes), &(mesh_transforms));

	std::vector<NPCCreator::NPC> *npcs = npc_creator.create_npcs(1);
	for (auto npc : *npcs) {
		for (auto p : npc.parts) {
			scene.drawables.emplace_back(p.second.second);
			Scene::Drawable &drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = meshes_for_lit_color_texture_program;
			drawable.pipeline.type = p.second.first->type;
			drawable.pipeline.start = p.second.first->start;
			drawable.pipeline.count = p.second.first->count;
			
			std::cout << p.first << ", " << npc.creator->selection_to_mesh_name[p.first][npc.get_from_selection(p.first)] << ": " << drawable.pipeline.type << ", " << drawable.pipeline.start << ", " << drawable.pipeline.count << std::endl;
		}

		npc.parts["head"].second->parent = npc.parts["arm_l"].second->parent = npc.parts["arm_r"].second->parent = npc.parts["legs"].second->parent = npc.parts["body"].second;
		npc.parts["hat"].second->parent = npc.parts["head"].second;

		// flip the right arm to the other side of the body
		npc.parts["arm_r"].second->scale.x *= -1;
		npc.parts["arm_r"].second->position.x *= -1;
		//npc.parts["body"].second->position += glm::vec3((float) std::rand() / (float) RAND_MAX * 5.f, (float) std::rand() / (float) RAND_MAX * 5.f, (float) std::rand() / (float) RAND_MAX * 5.f);
	}
	// create the transform and drawable with the selected information
	/*
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
	*/

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


	glClearColor(0.f, 0.f, 0.f, 1.0f); // used to set background color...maybe can use it for a day/night cycle?
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
