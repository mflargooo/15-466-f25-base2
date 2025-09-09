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


GLuint npc_meshes_for_lit_color_texture_program = 0;
NPCCreator npc_creator({ "head", "body", "arm", "legs", "hat" }, { "head", "body", "arm_l", "arm_r", "legs", "hat" });
Load< MeshBuffer > npc_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("npcs.pnct"));
	npc_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > npcs_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("npcs.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = npc_meshes->lookup(mesh_name);
		npc_creator.mesh_templates[mesh_name].first = &mesh;
		npc_creator.mesh_templates[mesh_name].second = transform;
	});
});

GLuint ufo_meshes_for_lit_color_texture_program = 0;
Load < MeshBuffer > ufo_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("alien.pnct"));
	ufo_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

std::map < std::string, std::pair < const Mesh *, Scene::Transform * >> player_template;
Load < Scene > ufo_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("alien.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = ufo_meshes->lookup(mesh_name);
		player_template[mesh_name].first = &mesh;
		player_template[mesh_name].second = transform;
	
	});
});

PlayMode::PlayMode() : scene(*ufo_scene) {
	//organize meshes for convenience
	npc_creator.initialize();
	npc_creator.register_data(&(npc_meshes->meshes));

	std::vector< NPCCreator::NPCInfo > npc_infos = npc_creator.create_npc_infos(10);

    for (auto info : npc_infos) {
        npcs.emplace_back(NPCCreator::NPC(info));
        NPCCreator::NPC npc = npcs.back();

        npc.info = &info;
        for (auto names : info.mesh_names) {
            std::string part_name = names.first;
            std::string mesh_name = names.second;
            auto mesh = npc_creator.mesh_templates[mesh_name].first;
            auto transform = npc_creator.mesh_templates[mesh_name].second;

            scene.drawables.emplace_back(new Scene::Transform());
			Scene::Drawable &drawable = scene.drawables.back();
			npc.drawables[part_name] = &drawable;

            *drawable.transform = *transform;

			drawable.pipeline = lit_color_texture_program_pipeline;
            drawable.pipeline.vao = npc_meshes_for_lit_color_texture_program;
            drawable.pipeline.type = mesh->type;
            drawable.pipeline.start = mesh->start;
            drawable.pipeline.count = mesh->count;
        }
		npc.drawables["head"]->transform->parent = npc.drawables["arm_l"]->transform->parent = npc.drawables["arm_r"]->transform->parent = npc.drawables["legs"]->transform->parent = npc.drawables["body"]->transform;
		npc.drawables["hat"]->transform->parent = npc.drawables["head"]->transform;
		
		npc.drawables["arm_r"]->transform->scale.x *= -1;
		npc.drawables["arm_r"]->transform->position.x *= -1;
		
		npc.drawables["body"]->transform->position += glm::vec3((float) std::rand() / (float) RAND_MAX * 50.f, (float) std::rand() / (float) RAND_MAX * 50.f, 0.f);
	}

	// player anchor
	player.transform = new Scene::Transform();

	// setup player mesh and camera, and anchor both
	auto player_mesh = player_template["alien"].first;
	auto player_transform = player_template["alien"].second;

	scene.drawables.emplace_back(new Scene::Transform());
	player.drawable = &scene.drawables.back();
	*(player.drawable->transform) = *player_transform;
	player.drawable->transform->parent = player.transform;

	player.drawable->pipeline = lit_color_texture_program_pipeline;
	player.drawable->pipeline.vao = ufo_meshes_for_lit_color_texture_program;
	player.drawable->pipeline.type = player_mesh->type;
	player.drawable->pipeline.start = player_mesh->start;
	player.drawable->pipeline.count = player_mesh->count;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	cam_info.dist_from_player = cam_info.MAX_DIST_FROM_PLAYER;
	cam_info.pitch = (float)M_PI * .5f;

	camera->transform->parent = player.transform;
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

			// yaw and pitch to avoid rolling, clamp each
			cam_info.yaw -= motion.x * camera->fovy;
			if (cam_info.yaw > (float)M_PI * 2.f) cam_info.yaw -= (float)M_PI * 2.f;
			else if (cam_info.yaw < 0.f) cam_info.yaw += (float)M_PI * 2.f;

			cam_info.pitch += motion.y * camera->fovy;
			if (cam_info.pitch > (float)M_PI) cam_info.pitch = (float)M_PI;
			else if (cam_info.pitch < 0.f) cam_info.pitch = 0.f;

			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_WHEEL) {
		if (std::signbit(wheel.velocity) == std::signbit(evt.wheel.y)) {
			wheel.velocity += evt.wheel.y;
			wheel.scrolled += 1;
		}
		else {
			wheel.velocity = evt.wheel.y;
			wheel.scrolled = 1;
		}
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//update player and camera:
	{	
		constexpr glm::vec3 Cam_offset_from_center = glm::vec3(0.0f, 0.0f, 1.0f);
		constexpr float Tilt = -(float)M_PI / 12.f;

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move);

		// compute new camera position
		if (wheel.scrolled > 0) {
			float old_dist = cam_info.dist_from_player;
			cam_info.dist_from_player += wheel.velocity * cam_info.zoom_speed;
			if (old_dist < cam_info.dist_from_player) {
			// zoom out
				if (cam_info.dist_from_player < cam_info.MIN_DIST_FROM_PLAYER) {
					cam_info.dist_from_player = cam_info.MIN_DIST_FROM_PLAYER;
				}
				else if (cam_info.dist_from_player > cam_info.MAX_DIST_FROM_PLAYER) {
					cam_info.dist_from_player = cam_info.MAX_DIST_FROM_PLAYER;
				}
				wheel.scrolled = 0;
			}
			else if (old_dist > cam_info.dist_from_player) {
			// zoom in
				if (cam_info.dist_from_player < cam_info.MIN_DIST_FROM_PLAYER) {
					cam_info.dist_from_player = 0.f;
				}
				wheel.scrolled = 0;
			}
			wheel.scrolled--;
		}

		float cos_yaw = std::cosf(cam_info.yaw);
		float sin_yaw = std::sinf(cam_info.yaw);
		float cos_pitch = std::cosf(cam_info.pitch);
		float sin_pitch = std::sinf(cam_info.pitch);

		// get direction of camera from player, using spherical
		glm::vec3 dir_to_camera = glm::normalize(glm::vec3(
			sin_pitch * sin_yaw,
			-cos_yaw * sin_pitch,
			cos_pitch
		));

		static float player_rot = 0;
		if (move != glm::vec2(0.f)) {
			glm::vec3 move_dir = glm::normalize(glm::vec3(
				move.x * cos_yaw - move.y * sin_yaw, 
				move.x * sin_yaw + move.y * cos_yaw, 
				0.f
			));

			player.transform->position += PlayerSpeed * elapsed * move_dir;
			
			// rotate the player in the same way --- lerp between rotation if have time
			player_rot = cam_info.yaw;
			// i cant think of the math, so case work
			if (move.x > 0.f && move.y > 0.f) {
				player_rot += -(float)M_PI / 4.f;
			} else if (move.x > 0.f && move.y == 0.f) {
				player_rot += -(float)M_PI / 2.f;
			} else if (move.x > 0.f && move.y < 0.f) {
				player_rot += -(float)M_PI * .75f;
			} else if (move.x == 0.f && move.y < 0.f) {
				player_rot += (float) M_PI;
			} else if (move.x < 0.f && move.y < 0.f) {
				player_rot += (float) M_PI * .75f;
			} else if (move.x < 0.f && move.y == 0.f) {
				player_rot += (float)M_PI / 2.f;
			} else if (move.x < 0.f && move.y > 0.f) {
				player_rot += (float)M_PI / 4.f;
			}

			player.drawable->transform->rotation = glm::quat( glm::vec3(Tilt, 0.f, player_rot) );
		}
		else {
			player.drawable->transform->rotation = glm::quat (glm::vec3(0.f, 0.f, player_rot));
		}

		// place camera at new location with new rotation
		camera->transform->position = dir_to_camera * cam_info.dist_from_player + glm::vec3(0.0f, 0.0f, 1.0f);
		// euler to quat by https://gamedev.stackexchange.com/questions/13436/glm-euler-angles-to-quaternion
		camera->transform->rotation = glm::quat( glm::vec3(cam_info.pitch, 0.0f, cam_info.yaw));
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
