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
#include <algorithm>

size_t score = 0;
size_t level = 0;
size_t npcs_to_spawn = 4;

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
	// create npcs
	{
		//organize meshes for convenience
		static bool initialized = false;
		if (!initialized) {
			npc_creator.initialize();
			npc_creator.register_data(&(npc_meshes->meshes));
		}
		initialized = true;

		level += 1;
		npcs_to_spawn *= 2;
		npcs_to_spawn = std::min(npcs_to_spawn, (size_t)512);
		float spawn_radius = (npcs_to_spawn + 50.f);
		std::cout << std::endl << "--------------------------------------" << std::endl << std::endl;
		std::cout << "| LEVEL: " << level << "! FIND " << (size_t) TARGET_COUNT << " TARGETS OUT OF " << npcs_to_spawn << " |" << std::endl;
		std::cout << "--------------------------------------" << std::endl << std::endl;

		const Mesh *body_mesh = nullptr;
		npc_infos = npc_creator.create_npc_infos(npcs_to_spawn);

		std::uniform_real_distribution< float > spawner(-spawn_radius / 2.f, spawn_radius / 2);
		std::uniform_real_distribution< float > rand_angle(0.f, (float)M_PI * 2.f);

		for (auto info : npc_infos) {
			npcs.emplace_back(NPCCreator::NPC());
			NPCCreator::NPC *npc = &npcs.back();

			for (auto names : info.mesh_names) {
				std::string part_name = names.first;
				std::string mesh_name = names.second;
				auto mesh = npc_creator.mesh_templates[mesh_name].first;
				auto transform = npc_creator.mesh_templates[mesh_name].second;

				if (part_name == "body") body_mesh = mesh;

				scene.drawables.emplace_back(new Scene::Transform());
				Scene::Drawable &drawable = scene.drawables.back();
				npc->drawables[part_name] = &drawable;

				*drawable.transform = *transform;

				drawable.pipeline = lit_color_texture_program_pipeline;
				drawable.pipeline.vao = npc_meshes_for_lit_color_texture_program;
				drawable.pipeline.type = mesh->type;
				drawable.pipeline.start = mesh->start;
				drawable.pipeline.count = mesh->count;
			}

			// since body is reused, maybe hash mesh.start with body radius
			float body_radius = -1;
			if (body_mesh) {
				body_radius = std::min(std::min(
					std::abs(body_mesh->max.x - body_mesh->min.x),
					std::abs(body_mesh->max.y - body_mesh->min.y)),
					std::abs(body_mesh->max.z - body_mesh->min.z));

				npc_radii.emplace_back(body_radius);
			}
			else {
				throw new std::runtime_error("No body mesh could be found!");
			}

			npc->drawables["head"]->transform->parent = npc->drawables["arm_l"]->transform->parent = npc->drawables["arm_r"]->transform->parent = npc->drawables["legs"]->transform->parent = npc->drawables["body"]->transform;
			npc->drawables["hat"]->transform->parent = npc->drawables["head"]->transform;
			
			npc->drawables["arm_r"]->transform->scale.x *= -1;
			npc->drawables["arm_r"]->transform->position.x *= -1;
			
			glm::vec3 rand_pos = glm::vec3(0.f);
			size_t trials = 0;
			bool near_npc = false;
			do {
				rand_pos = glm::vec3(spawner(rng), spawner(rng), 0.f);
				for (size_t i = 0; i < npcs.size() - 1; i++) {
					if (glm::length(rand_pos - npcs[i].drawables["body"]->transform->position) <= 2.f * (npc_radii[i] - (float) trials / 1000.f)) {
						near_npc = true;
						break;
					}
				}
				trials++;
			} while (trials < 1000 && (near_npc || glm::length(rand_pos) < player.collision_radius * 2.5f));
			
			npc->drawables["body"]->transform->position += rand_pos;

			npc->drawables["body"]->transform->rotation = glm::quat( glm::vec3(0.f, 0.f, rand_angle(rng)));
		}
	}
	// create player
	{
		// player anchor
		player.transform = new Scene::Transform();

		// setup player mesh and camera, and anchor both
		auto alien_mesh = player_template["alien"].first;
		auto alien_transform = player_template["alien"].second;

		auto saucer_mesh = player_template["saucer"].first;
		auto saucer_transform = player_template["saucer"].second;

		scene.drawables.emplace_back(new Scene::Transform());
		player.drawable = &scene.drawables.back();
		scene.drawables.emplace_back(new Scene::Transform());
		player.saucer_drawable = &scene.drawables.back();

		*(player.drawable->transform) = *alien_transform;
		*(player.saucer_drawable->transform) = *saucer_transform;
		player.drawable->transform->parent = player.saucer_drawable->transform;
		player.saucer_drawable->transform->parent = player.transform;
		
		// hide player model since zoomed in at beginning
		player.drawable->transform->scale = glm::vec3(.01f, .01f, .01f);
		player.drawable->transform->position = glm::vec3(0.f, 0.f, 1001.f);

		player.drawable->pipeline = lit_color_texture_program_pipeline;
		player.drawable->pipeline.vao = ufo_meshes_for_lit_color_texture_program;
		player.drawable->pipeline.type = alien_mesh->type;
		player.drawable->pipeline.start = alien_mesh->start;
		player.drawable->pipeline.count = alien_mesh->count;

		player.saucer_drawable->pipeline = lit_color_texture_program_pipeline;
		player.saucer_drawable->pipeline.vao = ufo_meshes_for_lit_color_texture_program;
		player.saucer_drawable->pipeline.type = saucer_mesh->type;
		player.saucer_drawable->pipeline.start = saucer_mesh->start;
		player.saucer_drawable->pipeline.count = saucer_mesh->count;

		//get pointer to camera for convenience:
		if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
		camera = &scene.cameras.front();

		cam_info.dist_from_player = 0.f;

		cam_info.pitch = (float)M_PI * .5f;

		camera->transform->parent = player.transform;
	}
	// setup game loop, ui instantiation
	{
		std::vector < size_t > indices(npcs.size());
		for (size_t i = 0; i < indices.size(); i++) {
			indices[i] = i;
		}

		std::vector < size_t > tmp_targets;
		tmp_targets.reserve(TARGET_COUNT);

		std::sample(indices.begin(), indices.end(), std::back_inserter(tmp_targets), TARGET_COUNT, rng);

		size_t i = 0;
		for (auto idx : tmp_targets) {
			// add target idx to container to check for collision later
			targets.npc_idxs.emplace_back(idx);
			targets.alive.emplace_back(true);

			// setup the location of the icon with respect to the camera
			scene.drawables.emplace_back(new Scene::Transform());
			targets.ui_transforms.emplace_back(scene.drawables.back().transform);
			Scene::Transform *ui_slot = targets.ui_transforms.back(); 

			ui_slot->position = glm::vec3(0.f, 0.f, 0.f);
			ui_slot->scale = glm::vec3(1.f);

			targets.drawables.emplace_back(std::map< std::string, Scene::Drawable * >());
			auto &ui_drawables = targets.drawables.back();
			for (auto pair : npcs[idx].drawables) {
				scene.drawables.emplace_back(new Scene::Transform());
				Scene::Drawable &drawable = scene.drawables.back();
				drawable.pipeline = pair.second->pipeline;
				*(drawable.transform) = *(pair.second->transform);
				ui_drawables[pair.first] = &scene.drawables.back();
			}

			ui_drawables["head"]->transform->parent = ui_drawables["arm_l"]->transform->parent = ui_drawables["arm_r"]->transform->parent = ui_drawables["legs"]->transform->parent = ui_drawables["body"]->transform;
			ui_drawables["hat"]->transform->parent = ui_drawables["head"]->transform;

			ui_drawables["body"]->transform->parent = ui_slot;
			ui_drawables["body"]->transform->position = glm::vec3(0.f);
			ui_drawables["body"]->transform->rotation = glm::quat( glm::vec3(-(float) M_PI / 6.0f, 0.f, 0.f) );

			ui_slot->parent = player.transform;

			ui_drawables["body"]->transform->scale *= .05;
			i += 1;
		}
	}
}

PlayMode::~PlayMode() {
	for (auto drawable : scene.drawables) {
		delete drawable.transform;
	}

	delete player.transform;
	npc_creator.used_npcs.clear();
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
		} else if (evt.key.key == SDLK_R) {
			score = 0;
			level = 0;
			npcs_to_spawn = 4;
			return false;
		} else if (evt.key.key == SDLK_SPACE) {
			return score != TARGET_COUNT * level;
		} else if (evt.key.key == SDLK_Q) {
			std::cout << std::endl << "***********************************************" << std::endl;
			std::cout << "* YOUR CAREER IS OVER! ELIMINATED " << score << " TARGETS! *" << std::endl;
			std::cout << "***********************************************" << std::endl;
			return false;
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
			if (cam_info.yaw > (float)M_PI) cam_info.yaw -= (float)M_PI * 2.f;
			else if (cam_info.yaw < -(float)M_PI) cam_info.yaw += (float)M_PI * 2.f;

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
	if (player.dead) {
		if (!score_displayed) {
			score_displayed = true;
			player.drawable->transform->scale = glm::vec3(.01f, .01f, .01f);
			player.drawable->transform->position = glm::vec3(0.f, 0.f, 1005.f);
			player.saucer_drawable->transform->scale = glm::vec3(.01f, .01f, .01f);
			player.saucer_drawable->transform->position = glm::vec3(0.f, 0.f, 1005.f);

			for (auto ui_transform : targets.ui_transforms) {
				ui_transform->position = glm::vec3(0.f, 0.f, 1005.f);
			}
			std::cout << std::endl << "*********************************************" << std::endl;
			std::cout << "* YOUR RUN IS OVER! ELIMINATED " << score << " TARGETS! *" << std::endl;
			std::cout << "*********************************************" << std::endl;
			std::cout << std::endl << "Press: R - To Restart" << std::endl;
			std::cout << "       Q - To Quit" << std::endl;
		}
		return;
	}

	//update player and camera:
	{	
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
		if (wheel.scrolled != 0) {
			float old_dist = cam_info.dist_from_player;
			cam_info.dist_from_player += wheel.velocity * cam_info.zoom_speed;
			if (old_dist < cam_info.dist_from_player) {
			// zoom out
				if (old_dist < cam_info.MIN_DIST_FROM_PLAYER) {
					cam_info.dist_from_player = cam_info.MIN_DIST_FROM_PLAYER;
					// show player model 
					player.drawable->transform->scale = glm::vec3(1.f);
					player.drawable->transform->position = glm::vec3(0.f);
				}
				else if (cam_info.dist_from_player > cam_info.MAX_DIST_FROM_PLAYER) {
					cam_info.dist_from_player = cam_info.MAX_DIST_FROM_PLAYER;
				}
				wheel.scrolled = 0;
				wheel.velocity = 0.f;
			}
			else if (old_dist > cam_info.dist_from_player) {
			// zoom in
				if (cam_info.dist_from_player < cam_info.MIN_DIST_FROM_PLAYER) {
					cam_info.dist_from_player = 0.f;
					// hide player model in funky way
					player.drawable->transform->scale = glm::vec3(.01f, .01f, .01f);
					player.drawable->transform->position = glm::vec3(0.f, 0.f, 1001.f);
				}
				wheel.scrolled = 0;
				wheel.velocity = 0.f;
			}
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

		static glm::vec3 player_rot = glm::vec3(0.0f);
		if (move != glm::vec2(0.f)) {
			glm::vec3 move_dir = glm::normalize(glm::vec3(
				move.x * cos_yaw - move.y * sin_yaw, 
				move.x * sin_yaw + move.y * cos_yaw, 
				0.f
			));

			player.transform->position += PlayerSpeed * elapsed * move_dir;
			
			float target_rot_z = cam_info.yaw;
			// i cant think of the math, so case work instead
			if (move.x > 0.f && move.y > 0.f) {
				target_rot_z += -(float)M_PI / 4.f;
			} else if (move.x > 0.f && move.y == 0.f) {
				target_rot_z += -(float)M_PI / 2.f;
			} else if (move.x > 0.f && move.y < 0.f) {
				target_rot_z += -(float)M_PI * .75f;
			} else if (move.x == 0.f && move.y < 0.f) {
				target_rot_z += (float) M_PI;
			} else if (move.x < 0.f && move.y < 0.f) {
				target_rot_z += (float) M_PI * .75f;
			} else if (move.x < 0.f && move.y == 0.f) {
				target_rot_z += (float)M_PI / 2.f;
			} else if (move.x < 0.f && move.y > 0.f) {
				target_rot_z += (float)M_PI / 4.f;
			}
			
			float diff_z = target_rot_z - player_rot.z;
			//normalize
			if (diff_z > (float)M_PI) diff_z -= (float)M_PI * 2.f;
			else if (diff_z < -(float)M_PI) diff_z += (float)M_PI * 2.f;

			player_rot.x = player_rot.x * .9f + Tilt * .1f;
			player_rot.z = player_rot.z * .9f + (player_rot.z + diff_z) * .1f;
			

			player.saucer_drawable->transform->rotation = glm::quat(player_rot);
		}
		else {
			player_rot.x = player_rot.x * .9f;	
			player.saucer_drawable->transform->rotation = glm::quat(player_rot);
		}

		if (player_rot.z > (float) M_PI) player_rot.z -= (float) M_PI * 2.f;
		else if (player_rot.z < -(float) M_PI) player_rot.z += (float) M_PI * 2.f;

		// place camera at new location with new rotation
		camera->transform->position = dir_to_camera * cam_info.dist_from_player + cam_info.offset;
		// euler to quat by https://gamedev.stackexchange.com/questions/13436/glm-euler-angles-to-quaternion
		camera->transform->rotation = glm::quat( glm::vec3(cam_info.pitch, 0.0f, cam_info.yaw));

		for (size_t i = 0; i < TARGET_COUNT; i++) {
			if (!targets.alive[i]) continue;
			glm::vec3 base_pos = glm::vec3(UI_SPACING * ((float)TARGET_COUNT / 2.f - (float) TARGET_COUNT + (float) TARGET_COUNT / ((float)TARGET_COUNT - 1.f) * (float) i) / TARGET_COUNT, 2.75f, 1.f);
			targets.ui_transforms[i]->position = glm::vec3(base_pos.x * cos_yaw - base_pos.y * sin_yaw, base_pos.x * sin_yaw + base_pos.y * cos_yaw, base_pos.z);
			targets.ui_transforms[i]->rotation = glm::quat( glm::vec3(0.f, 0.f, cam_info.yaw));
		}
	}
	// check collision
	{
		for (size_t i = 0; i < npcs.size(); i++) {
			if (glm::length(player.transform->position - npcs[i].drawables["body"]->transform->position) <= player.collision_radius + npc_radii[i]) {
				// check to see if npc we collided with was a target
				int32_t valid_target_idx = -1;
				for (size_t j = 0; j < TARGET_COUNT; j++) {
					if (i == targets.npc_idxs[j]) {
						valid_target_idx = (int32_t) j;
						break;
					}
				}

				// if it was, move off screen
				if (valid_target_idx >= 0) {
				//simply move the transform off screen to avoid cleaning up memory at the moment 
					std::cout << "YOU ELIMINATED +1 TARGET!" << std::endl;
					npcs[i].drawables["body"]->transform->position = glm::vec3(0.0f, 0.0f, 1005.f);
					npcs[i].drawables["body"]->transform->scale *= .2f;
					targets.alive[valid_target_idx] = false;
					targets.ui_transforms[valid_target_idx]->position = glm::vec3(0.0f, 0.0f, 1005.f);
					score += 1;
					break;
				}
				else { // die
					player.dead = true;
					break;
				}
			}
		}
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
		lines.draw_text("Mouse motion rotates camera; Scroll wheel zooms camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; Scroll wheel zooms camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		char game_state_text[40];
		std::snprintf(&game_state_text[0], 16, "LEVEL: %zd\tELIMINATIONS: %zd", level, score);
		lines.draw_text(game_state_text,
			glm::vec3(-aspect + .1f * H, .9 - .1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		std::snprintf(&game_state_text[0], 40, "LEVEL: %zd     ELIMINATIONS: %zd", level, score);
		lines.draw_text(game_state_text,
			glm::vec3(-aspect + .1f * H + ofs, .9 - .1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		if (score == TARGET_COUNT * level && !player.dead) {
			lines.draw_text("SPACE - Next Quota",
				glm::vec3(-.35, 0.f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("SPACE - Next Quota",
				glm::vec3(ofs -.35, ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		else if (player.dead) {
			lines.draw_text("R - Restart",
				glm::vec3(-.2, 0.f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("R - Restart",
				glm::vec3(ofs -.2, ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));

			lines.draw_text("Q - Quit",
				glm::vec3(-.15, -.15, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("Q - Quit",
				glm::vec3(ofs -.15, ofs - .15, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
}
