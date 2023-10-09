#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "utility.h"

enum sprites
{
	sprite_main_car0, sprite_main_car1,
	sprite_trap_car0, sprite_trap_car1,
	sprite_tank_car,
	sprite_regular_car,
	sprite_car_destroy0, sprite_car_destroy1, sprite_car_destroy2,
	sprite_explosion0, sprite_explosion1, sprite_explosion2,
	sprite_tree, sprite_puddle, sprite_box, sprite_trap,

	sprites_count
};
void load_textures(unique_ptr<texture_type>* textures, screen_type* screen)
{
	textures[sprite_main_car0] = new texture_type("sprites/main_car0.bmp", screen->renderer);
	textures[sprite_main_car1] = new texture_type("sprites/main_car1.bmp", screen->renderer);
	textures[sprite_trap_car0] = new texture_type("sprites/trap_car0.bmp", screen->renderer);
	textures[sprite_trap_car1] = new texture_type("sprites/trap_car1.bmp", screen->renderer);
	textures[sprite_tank_car] = new texture_type("sprites/tank_car.bmp", screen->renderer);
	textures[sprite_regular_car] = new texture_type("sprites/regular_car.bmp", screen->renderer);
	textures[sprite_car_destroy0] = new texture_type("sprites/car_destroy0.bmp", screen->renderer);
	textures[sprite_car_destroy1] = new texture_type("sprites/car_destroy1.bmp", screen->renderer);
	textures[sprite_car_destroy2] = new texture_type("sprites/car_destroy2.bmp", screen->renderer);
	textures[sprite_explosion0] = new texture_type("sprites/explosion0.bmp", screen->renderer);
	textures[sprite_explosion1] = new texture_type("sprites/explosion1.bmp", screen->renderer);
	textures[sprite_explosion2] = new texture_type("sprites/explosion2.bmp", screen->renderer);
	textures[sprite_tree] = new texture_type("sprites/tree.bmp", screen->renderer);
	textures[sprite_puddle] = new texture_type("sprites/puddle.bmp", screen->renderer);
	textures[sprite_box] = new texture_type("sprites/box.bmp", screen->renderer);
	textures[sprite_trap] = new texture_type("sprites/trap.bmp", screen->renderer);
}

struct entity;

enum entities
{
	entity_grass, entity_puddle, entity_box, entity_tree, entity_trap,
	entity_bullet, entity_bazooka, 
	entity_regular_car, entity_trap_car, entity_tank_car, entity_main_car,
	entity_explosion,

	entity_count,
	entity_cars_first = entity_regular_car,
	entity_cars_last = entity_main_car,
	entity_none = 0xff
};
enum direction
{
	direction_up, direction_down, direction_left, direction_right
};
enum class game_state
{
	running, paused, score_points, score_time, finished, quit
};
enum class running_state
{
	normal, enter, slow, destroy
};
struct game_data
{
	static constexpr const char save_file_prefix[] = "SpyHunterSaveFile";

	static constexpr int menu_bar_height = 32;
	static constexpr int inner_menu_bar_height = 24;
	static constexpr int menu_text_offset = 16;
	static constexpr int baseline_offset = 128;

	static constexpr int game_width = 64;
	static constexpr int game_height = 32;
	static constexpr int road_min = 24;
	static constexpr int road_max = 40;
	static constexpr int grass_min_left = 8;

	static constexpr int destroy_front = 64;
	static constexpr int destroy_back = 16;

	static constexpr float max_speed = 48.f;
	static constexpr float acceleration = 8.f;
	static constexpr float deaccelerate = 24.f;
	static constexpr float friction = 12.f;

	static constexpr long free_respawn_time = 60;
	static constexpr float slow_time = 1.f;
	static constexpr float enter_time = 1.f;
	static constexpr float destroy_time = 0.5f;

	static constexpr float bullet_lifetime = 0.1f;
	static constexpr float bullet_speed = 200.f;
	static constexpr float bullet_reload = 0.02f;
	static constexpr float bazooka_reload = 0.4f;
	static constexpr float explosion_time = 0.3f;
	static constexpr float trap_cooldown = 6.f;

	game_state state;
	bool arrows[4];
	bool shooting;
	unsigned long long random_seed;

	running_state car_state;
	float car_state_left;
	int lives;

	int generation_pos;

	float road_size_zeroth;
	float road_size_first;
	float road_pos_zeroth;
	float road_pos_first;

	int tree_cooldown;
	int puddle_cooldown;
	int box_cooldown;
	int car_cooldown;
	float bullet_cooldown;
	int bazooka_left;

	long score;
	long last_dist_score_checkpoint;
	long last_life_checkpoint;
	long elapsed_time;
	Uint32 last_frame_time;

	dynamic_array<unique_ptr<entity>> entities[entity_count];
};

struct render_data_type
{
	screen_type* screen;
	game_data* game;
	font_type* font;
	unique_ptr<texture_type> textures[sprites_count];
};

point coord_to_point(const render_data_type& render_data, coord val);
point game_to_screen(const render_data_type& render_data, coord size);

struct entity
{
	entity() = default;
	virtual ~entity() noexcept = default;

	virtual void update(float delta) {}
	virtual void render(const render_data_type& render_data) = 0;

	entity(FILE* file)
	{
		fread(&this->position, sizeof(this->position), 1, file);
		fread(&this->hitbox_rel_pos, sizeof(this->hitbox_rel_pos), 1, file);
		fread(&this->hitbox_size, sizeof(this->hitbox_size), 1, file);
	}
	virtual void save(FILE* file) const
	{
		fwrite(&this->position, sizeof(this->position), 1, file);
		fwrite(&this->hitbox_rel_pos, sizeof(this->hitbox_rel_pos), 1, file);
		fwrite(&this->hitbox_size, sizeof(this->hitbox_size), 1, file);
	}

	bool collides(const entity& other) const noexcept
	{
		coord hitbox_min = { this->position.x + this->hitbox_rel_pos.x, this->position.y + this->hitbox_rel_pos.y };
		coord hitbox_max = { hitbox_min.x + this->hitbox_size.x, hitbox_min.y + this->hitbox_size.y };

		coord hitbox_min2 = { other.position.x + other.hitbox_rel_pos.x, other.position.y + other.hitbox_rel_pos.y };
		coord hitbox_max2 = { hitbox_min2.x + other.hitbox_size.x, hitbox_min2.y + other.hitbox_size.y };

		return hitbox_max.x > hitbox_min2.x && hitbox_max2.x > hitbox_min.x &&
			hitbox_max.y > hitbox_min2.y && hitbox_max2.y > hitbox_min.y;
	}

	coord position;
	coord hitbox_rel_pos;
	coord hitbox_size;
};
struct hitbox_check : public entity
{
	hitbox_check(coord position, coord size)
	{
		this->position = position;
		this->hitbox_size = size;
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };
	}
	hitbox_check(coord position, coord size, direction dir)
	{
		this->position = position;
		this->hitbox_size = size;
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };
		switch (dir)
		{
			case direction_up: this->hitbox_rel_pos.y = 0; break;
			case direction_down: this->hitbox_rel_pos.y = -this->hitbox_size.y; break;
			case direction_left: this->hitbox_rel_pos.x = -this->hitbox_size.x; break;
			case direction_right: this->hitbox_rel_pos.x = 0; break;
		}
	}

	void render(const render_data_type& render_data) override {}
};
struct grass : public entity
{
	grass(coord road_center_pos, float road_width, direction dir)
	{
		float x_radius = road_width / 2.f;
		float x_offset = road_center_pos.x;

		if (dir == direction_left) {
			x_radius = -x_radius;
			x_offset = -x_offset;
		}

		this->position.x = road_center_pos.x + x_radius;
		this->size.x = game_data::game_width / 2.f - road_width / 2.f - x_offset + 1.f;

		if (dir == direction_left)
			this->position.x -= this->size.x;

		this->position.y = road_center_pos.y;
		this->size.y = 1.f;

		this->hitbox_rel_pos = { 0, 0 };
		this->hitbox_size = this->size;
	}

	void render(const render_data_type& render_data) override
	{
		draw_rect(render_data.screen, coord_to_point(render_data, this->position), 
				  game_to_screen(render_data, this->size), color::green());
	}

	grass(FILE* file)
		:entity(file)
	{
		fread(&this->size, sizeof(this->size), 1, file);
	}
	void save(FILE* file) const override
	{
		entity::save(file);
		fwrite(&this->size, sizeof(this->size), 1, file);
	}

	coord size;
};
struct tree : public entity
{
	tree(coord position)
	{
		this->position = position;
	}

	void render(const render_data_type& render_data) override
	{
		draw_texture(render_data.screen, render_data.textures[sprite_tree].get(),
					 coord_to_point(render_data, this->position));
	}

	tree(FILE* file)
		:entity(file)
	{
	}
};
struct puddle : public entity
{
	puddle(coord position)
	{
		this->position = position;
		this->hitbox_size = { 2.5f, 1.5f };
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };
	}

	void render(const render_data_type& render_data) override
	{
		draw_texture(render_data.screen, render_data.textures[sprite_puddle].get(),
					 coord_to_point(render_data, this->position));
	}

	puddle(FILE* file)
		:entity(file)
	{
	}
};
struct trap : public entity
{
	trap(coord position)
	{
		this->position = position;
		this->hitbox_size = { 2.f, 1.5f };
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };
	}

	void render(const render_data_type& render_data) override
	{
		draw_texture(render_data.screen, render_data.textures[sprite_trap].get(),
					 coord_to_point(render_data, this->position));
	}

	trap(FILE* file)
		:entity(file)
	{
	}
};
struct box : public entity
{
	box(coord position)
	{
		this->position = position;
		this->hitbox_size = { 3, 2 };
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };
	}

	void render(const render_data_type& render_data) override
	{
		draw_texture(render_data.screen, render_data.textures[sprite_box].get(),
					 coord_to_point(render_data, this->position));
	}

	box(FILE* file)
		:entity(file)
	{
	}
};
struct bullet : public entity
{
	bullet(coord position, float speed, float lifetime, bool explodes = false)
	{
		this->position = position;
		this->hitbox_size = { 0.2f, 0.4f };
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };

		this->speed = speed;
		this->lifetime = lifetime;
		this->explodes = explodes;
	}

	void update(float delta) override
	{
		if (this->lifetime > 0) {
			this->position.y += this->speed * min(delta, this->lifetime);
			this->lifetime -= delta;
		}
	}
	void render(const render_data_type& render_data) override
	{
		if(this->lifetime > 0.f)
			draw_rect(render_data.screen, coord_to_point(render_data, { this->position.x + this->hitbox_rel_pos.x, 
														 this->position.y + this->hitbox_rel_pos.y }),
					  game_to_screen(render_data, this->hitbox_size), color::gray());
	}

	bullet(FILE* file)
		:entity(file)
	{
		fread(&this->speed, sizeof(this->speed), 1, file);
		fread(&this->lifetime, sizeof(this->lifetime), 1, file);
		fread(&this->explodes, sizeof(this->explodes), 1, file);
	}
	void save(FILE* file) const override
	{
		entity::save(file);
		fwrite(&this->speed, sizeof(this->speed), 1, file);
		fwrite(&this->lifetime, sizeof(this->lifetime), 1, file);
		fwrite(&this->explodes, sizeof(this->explodes), 1, file);
	}

	float speed;
	float lifetime;
	bool explodes;
};
struct explosion : public entity
{
	explosion(coord position, float lifetime)
	{
		this->position = position;
		this->hitbox_size = { 6.f, 4.f };
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };

		this->lifetime = lifetime;
		this->max_lifetime = lifetime;
	}

	void update(float delta) override
	{
		this->lifetime -= delta;
	}
	void render(const render_data_type& render_data) override
	{
		if (this->lifetime > 0.f) {
			sprites anim = sprites(sprite_explosion0 + (int)(3 * this->lifetime / this->max_lifetime));
			if (anim > sprite_explosion2)
				anim = sprite_explosion2;

			draw_texture(render_data.screen, render_data.textures[anim].get(),
						 coord_to_point(render_data, this->position));
		}
	}

	explosion(FILE* file)
		:entity(file)
	{
		fread(&this->lifetime, sizeof(this->lifetime), 1, file);
		fread(&this->max_lifetime, sizeof(this->max_lifetime), 1, file);
	}
	void save(FILE* file) const override
	{
		entity::save(file);
		fwrite(&this->lifetime, sizeof(this->lifetime), 1, file);
		fwrite(&this->max_lifetime, sizeof(this->max_lifetime), 1, file);
	}

	float lifetime;
	float max_lifetime;
};
struct car : public entity
{
	car(coord position, coord size, dynamic_array<sprites>&& anim, float anim_time, 
		float invinc_time = 0.f, int lifes = 1, float action_cooldown = 0.f)
	{
		this->position = position;
		this->hitbox_size = size;
		this->hitbox_rel_pos = { -this->hitbox_size.x / 2.f, -this->hitbox_size.y / 2.f };

		this->animation = move(anim);
		this->anim_restart_time = anim_time;
		this->life = lifes;
		this->invinc_time = invinc_time;
		this->action_cooldown = action_cooldown;
		this->action_time = action_cooldown;
	}

	void update(float delta) override
	{
		this->position.y += this->speed * delta;
		this->position.x += this->speed * delta * sinf(this->move_angle * (3.1415f / 180.f));

		if (this->action_cooldown != 0.f)
			this->action_time -= delta;

		this->anim_time += delta;
		this->invinc_time -= delta;
		this->explostion_invinc_time -= delta;
		if (this->destroyed && this->anim_time >= this->anim_restart_time)
			this->animation = dynamic_array<sprites>();

		while (this->anim_time >= this->anim_restart_time)
			this->anim_time -= this->anim_restart_time;
	}
	void render(const render_data_type& render_data) override
	{
		if (this->animation.size() == 0)
			return;

		int index = (int)(this->anim_time * this->animation.size() / this->anim_restart_time);

		draw_texture(render_data.screen, render_data.textures[this->animation[index]].get(),
					 coord_to_point(render_data, { this->position.x + this->render_position_offset.x, 
									this->position.y + this->render_position_offset.y }), this->move_angle);
	}

	void destroy()
	{
		this->destroyed = true;
		this->anim_time = 0.f;
		this->anim_restart_time = game_data::destroy_time;
		this->animation = dynamic_array<sprites>();
		this->animation.add(sprite_car_destroy0);
		this->animation.add(sprite_car_destroy1);
		this->animation.add(sprite_car_destroy2);
	}

	car(FILE* file)
		:entity(file)
	{
		fread(&this->life, sizeof(this->life), 1, file);
		fread(&this->invinc_time, sizeof(this->invinc_time), 1, file);
		fread(&this->explostion_invinc_time, sizeof(this->explostion_invinc_time), 1, file);
		fread(&this->anim_time, sizeof(this->anim_time), 1, file);
		fread(&this->anim_restart_time, sizeof(this->anim_restart_time), 1, file);
		fread(&this->action_time, sizeof(this->action_time), 1, file);
		fread(&this->action_cooldown, sizeof(this->action_cooldown), 1, file);
		fread(&this->speed, sizeof(this->speed), 1, file);
		fread(&this->move_angle, sizeof(this->move_angle), 1, file);
		fread(&this->render_position_offset, sizeof(this->render_position_offset), 1, file);
		fread(&this->destroyed, sizeof(this->destroyed), 1, file);
		
		size_t count;
		fread(&count, sizeof(count), 1, file);
		this->animation = dynamic_array<sprites>(count);
		fread(this->animation.begin(), sizeof(sprites), count, file);
	}
	void save(FILE* file) const override
	{
		entity::save(file);
		fwrite(&this->life, sizeof(this->life), 1, file);
		fwrite(&this->invinc_time, sizeof(this->invinc_time), 1, file);
		fwrite(&this->explostion_invinc_time, sizeof(this->explostion_invinc_time), 1, file);
		fwrite(&this->anim_time, sizeof(this->anim_time), 1, file);
		fwrite(&this->anim_restart_time, sizeof(this->anim_restart_time), 1, file);
		fwrite(&this->action_time, sizeof(this->action_time), 1, file);
		fwrite(&this->action_cooldown, sizeof(this->action_cooldown), 1, file);
		fwrite(&this->speed, sizeof(this->speed), 1, file);
		fwrite(&this->move_angle, sizeof(this->move_angle), 1, file);
		fwrite(&this->render_position_offset, sizeof(this->render_position_offset), 1, file);
		fwrite(&this->destroyed, sizeof(this->destroyed), 1, file);

		size_t count = this->animation.size();
		fwrite(&count, sizeof(count), 1, file);
		fwrite(this->animation.begin(), sizeof(sprites), count, file);
	}

	dynamic_array<sprites> animation;
	int life = 1;
	float invinc_time = 0.f;
	float explostion_invinc_time = 0.f;
	float anim_time = 0.f;
	float anim_restart_time = 0.f;
	float action_time = 0.f;
	float action_cooldown = 0.f;
	float speed = 0.f;
	float move_angle = 0.f;
	coord render_position_offset;
	bool destroyed = false;
};

point coord_to_point(const render_data_type& render_data, coord val)
{
	coord main_car_pos = render_data.game->entities[entity_main_car][0]->position;

	return { (int)(render_data.screen->width / 2 +
					   (val.x) * render_data.screen->width / game_data::game_width),
		render_data.screen->height - game_data::menu_bar_height - (int)(
			game_data::baseline_offset + (val.y - main_car_pos.y) *
			(render_data.screen->height - 2 * game_data::menu_bar_height) / game_data::game_height) };
}
point game_to_screen(const render_data_type& render_data, coord size)
{
	return { (int)(size.x * render_data.screen->width / game_data::game_width),
	(int)(size.y * render_data.screen->height / game_data::game_height) };
}

struct score_type
{
	score_type() = default;

	score_type(const score_type&) = delete;
	score_type& operator=(const score_type&) = delete;

	score_type(score_type&& other) noexcept
	{
		this->name = other.name;
		other.name = NULL;
		this->score = other.score;
		this->elapsed_time = other.elapsed_time;
	}
	score_type& operator=(score_type&& other) noexcept
	{
		if (this->name)
			free(this->name);
		this->name = other.name;
		other.name = NULL;
		this->score = other.score;
		this->elapsed_time = other.elapsed_time;
		return *this;
	}

	~score_type()
	{
		if (this->name)
			free(this->name);
	}

	char* name = NULL;
	long score;
	long elapsed_time;
};

void get_scores(score_type* scores, int* count, game_state sort)
{
	FILE* file;
	fopen_s(&file, "scores.txt", "rb");
	if (!file) {
		*count = 0;
		return;
	}
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	char* buffer = (char*)malloc(length + 1);
	fseek(file, 0, SEEK_SET);
	fread(buffer, 1, length, file);
	buffer[length] = NULL;
	fclose(file);

	int read = 0;
	size_t line_length;
	
	for (char* it = buffer, *end = buffer + length; it != end; it += line_length + 1)
	{
		line_length = strchr(it, '\n') - it;

		score_type score;
		int read_count;
		sscanf_s(it, "%li %li%n", &score.score, &score.elapsed_time, &read_count);

		if (read < *count || (sort == game_state::score_points && score.score > scores[*count - 1].score) ||
			(sort == game_state::score_time && score.elapsed_time > scores[*count - 1].elapsed_time)) {
			score.name = (char*)malloc(line_length - read_count);
			memcpy(score.name, it + read_count + 1, line_length - read_count - 1);
			score.name[line_length - read_count - 1] = NULL;
			if (score.name[line_length - read_count - 2] == '\r') score.name[line_length - read_count - 2] = NULL;

			for (int i = read; i >= 0; i--)
				if (i == 0 || (sort == game_state::score_points && score.score < scores[i - 1].score) ||
					(sort == game_state::score_time && score.elapsed_time < scores[i - 1].elapsed_time))
				{
					for (int j = *count - 1; j > i; j--)
						scores[j] = move(scores[j - 1]);
					scores[i] = move(score);
					if (read < *count)
						read++;
					break;
				}
		}
	}

	free(buffer);
	*count = read;
}

void draw_overlay(const render_data_type& data)
{
	if (data.game->state == game_state::paused) {
		draw_rect(data.screen, { 0, 0 }, { data.screen->width, data.screen->height }, { 0, 0, 0, 96 });
		draw_text_center(data.screen, data.font, "Paused", { data.screen->width / 2, data.screen->height / 2 - 4 });
	}
	else if (data.game->state == game_state::score_points || 
			 data.game->state == game_state::score_time || data.game->state == game_state::finished)
	{
		point overlay_pos = { data.screen->width / 6, data.screen->height / 6 };
		point overlay_size = { 2 * data.screen->width / 3, 2 * data.screen->height / 3 };

		int border_width = (game_data::menu_bar_height - game_data::inner_menu_bar_height) / 2;

		point inner_overlay_pos = { overlay_pos.x + border_width, overlay_pos.y + border_width };
		point inner_overlay_size = { overlay_size.x - 2 * border_width, overlay_size.y - 2 * border_width };

		draw_rect(data.screen, overlay_pos, overlay_size, color::yellow());
		draw_rect(data.screen, inner_overlay_pos, inner_overlay_size, color::red());

		static constexpr int text_offset = (game_data::menu_bar_height - 8) / 2;

		point main_message_pos = { inner_overlay_pos.x + inner_overlay_size.x / 2, inner_overlay_pos.y + text_offset };
		point info_pos = { inner_overlay_pos.x + game_data::menu_text_offset, inner_overlay_pos.y + inner_overlay_size.y / 2 };
		char text[128];

		if (data.game->state == game_state::finished) {
			draw_text_center(data.screen, data.font, "You died", main_message_pos);

			info_pos.x += game_data::menu_text_offset;
			info_pos.y -= 8;

			sprintf_s(text, "You survived for %.3f seconds.", data.game->elapsed_time / 1000.f);
			draw_text(data.screen, data.font, text, info_pos);
			info_pos.y += 16;

			sprintf_s(text, "Your score: %li.", data.game->score);
			draw_text(data.screen, data.font, text, info_pos);
			info_pos.y += 16;
		}
		else {
			draw_text_center(data.screen, data.font, "Best scores", main_message_pos);
			main_message_pos.y += inner_overlay_size.y - 2 * text_offset - 8;
			draw_text_center(data.screen, data.font, "Best scores", main_message_pos);

			score_type best_scores[12] = {};
			int count = 12;
			get_scores(best_scores, &count, data.game->state);

			info_pos.y -= 16 * count / 2;
			for (int i = 0; i < count; i++) {
				sprintf_s(text, "%2i. Score: %6li; Time: %9.3f - %s", 
						  i + 1, best_scores[i].score, best_scores[i].elapsed_time / 1000.f, best_scores[i].name);
				draw_text(data.screen, data.font, text, info_pos);
				info_pos.y += 16;
			}
		}
	}
}
void draw(const render_data_type& data)
{
	static constexpr int inner_bar_offset = (game_data::menu_bar_height - game_data::inner_menu_bar_height) / 2;
	static constexpr int text_offset = (game_data::menu_bar_height - 8) / 2;

	for (int i = 0; i < entity_count; i++)
		for (unique_ptr<entity>& e : data.game->entities[i])
			e->render(data);

	draw_rect(data.screen, { 0, 0 },
			  { data.screen->width, game_data::menu_bar_height }, color::yellow());
	draw_rect(data.screen, { 0, inner_bar_offset },
			  { data.screen->width,  game_data::inner_menu_bar_height }, color::red());

	draw_rect(data.screen, { 0, data.screen->height - game_data::menu_bar_height },
			  { data.screen->width, game_data::menu_bar_height }, color::yellow());
	draw_rect(data.screen, { 0, data.screen->height - game_data::inner_menu_bar_height - inner_bar_offset },
			  { data.screen->width,  game_data::inner_menu_bar_height }, color::red());

	char text[256] = {};
	char lives[128] = {};

	if (data.game->elapsed_time / 1000 < game_data::free_respawn_time) {
		sprintf_s(text, sizeof(text), "%li", game_data::free_respawn_time - data.game->elapsed_time / 1000);
		draw_text_center(data.screen, data.font, text, { data.screen->width / 2, text_offset });
	}
	else
		sprintf_s(lives, sizeof(lives), "Lives: %li   ", data.game->lives + 1);
	
	sprintf_s(text, sizeof(text), "%sScore: %li   Elapsed time: %.3f", lives, data.game->score, data.game->elapsed_time / 1000.f);

	draw_text(data.screen, data.font, "Wojciech Slomowicz, 193151",
			  { game_data::menu_text_offset, text_offset });
	draw_text_right(data.screen, data.font, text,
					{ data.screen->width - game_data::menu_text_offset, text_offset });
	draw_text_right(data.screen, data.font, "Implemented features: a, b, c, d, e, f, g, h, i, j, k, l, m, n, o",
					{ data.screen->width - game_data::menu_text_offset, data.screen->height - text_offset - 8 });

	draw_overlay(data);
}

void get_text(char** text, const char* title, const char* message);
void save_score(game_data* data)
{
	char* text = NULL;
	get_text(&text, "Name", "Enter your name:");
	if (text)
	{
		FILE* file;
		fopen_s(&file, "scores.txt", "ab");
		if (file) {
			fprintf(file, "%li %li %s\n", data->score, data->elapsed_time, text);
			fclose(file);
		}
		free(text);
	}
}

direction grass_collision_check(game_data* data, coord position, float dist)
{
	hitbox_check grass_left_check(position, { dist, 30.f }, direction_left);
	hitbox_check grass_right_check(position, { dist, 30.f }, direction_right);

	for (int i = 0; i < data->entities[entity_grass].size();)
		if (grass_left_check.collides(*data->entities[entity_grass][i]))
			return direction_right;
		else if (grass_right_check.collides(*data->entities[entity_grass][i]))
			return direction_left;
		else
			i++;

	return direction_up;
}
direction car_collision_check(game_data* data, car* c, entities collision_entity = entity_none)
{
	hitbox_check car_ahead_check(c->position, { 7, 10.f }, direction_up);

	for (int j = entity_cars_first; j <= entity_cars_last; j++)
		if (collision_entity == entity_none || j == collision_entity)
			for (int i = 0; i < data->entities[j].size();) {
				car* other = (car*)data->entities[j][i].get();

				if (c != other && !other->destroyed && car_ahead_check.collides(*other))
					if (c->position.x > other->position.x)
						return direction_left;
					else
						return direction_right;
				else
					i++;
			}

	return direction_up;
}

direction regular_car_turn_behaviour(game_data* data, car* c, float* speed)
{
	direction main_car_ahead_dir = car_collision_check(data, c, entity_main_car);
	direction car_ahead_dir = car_collision_check(data, c);

	if (main_car_ahead_dir != direction_up)
		return main_car_ahead_dir;
	else {
		direction grass_collision = grass_collision_check(data, c->position, 5.f);

		if (grass_collision != direction_up && car_ahead_dir != direction_up)
			*speed *= 0.85f;
		else if (car_ahead_dir != direction_up)
			*speed *= 0.9f;

		if (grass_collision == direction_left)
			return direction_right;
		else if (grass_collision == direction_right)
			return direction_left;
		else if (car_ahead_dir != direction_up)
			return car_ahead_dir;
	}

	return direction_up;
}
direction trap_car_turn_behaviour(game_data* data, car* c, float* speed)
{
	car* main_car = (car*)data->entities[entity_main_car][0].get();

	direction main_car_ahead_dir = car_collision_check(data, c, entity_main_car);
	direction car_ahead_dir = car_collision_check(data, c);

	if (main_car_ahead_dir != direction_up)
		return main_car_ahead_dir;
	else {
		direction grass_collision = grass_collision_check(data, c->position, 5.f);

		if (grass_collision != direction_up && car_ahead_dir != direction_up)
			*speed *= 0.85f;
		else if (car_ahead_dir != direction_up)
			*speed *= 0.9f;

		if (grass_collision == direction_left)
			return direction_right;
		else if (grass_collision == direction_right)
			return direction_left;
		else if (car_ahead_dir != direction_up)
			return car_ahead_dir;
		else if (fabsf(main_car->position.x - c->position.x) < 4.f)
		{
			direction grass_wide_collision = grass_collision_check(data, c->position, 10.f);

			if (grass_wide_collision == direction_left)
				return direction_right;
			else if (grass_wide_collision == direction_right)
				return direction_left;
			else if (main_car->move_angle > 0.f)
				return direction_left;
			else if (main_car->move_angle < 0.f)
				return direction_right;
			else
				if (main_car->position.x > c->position.x)
					return direction_right;
				else
					return direction_left;
		}
	}

	return direction_up;
}
direction tank_car_turn_behaviour(game_data* data, car* c)
{
	car* main_car = (car*)data->entities[entity_main_car][0].get();
	direction grass_collision = grass_collision_check(data, c->position, 5.f);

	coord attack_pos = c->position;
	attack_pos.y += 5.f;
	hitbox_check main_car_attack_check(c->position, { 30.f, 5.f }, direction_down);

	if (grass_collision == direction_left)
		return direction_right;
	else if (grass_collision == direction_right)
		return direction_left;

	if (grass_collision == direction_up)
	{
		if (main_car_attack_check.collides(*main_car)) {
			if (main_car->position.x > c->position.x)
				return direction_left;
			else
				return direction_right;
		}
		else {
			direction car_ahead_dir = car_collision_check(data, c);

			if (car_ahead_dir != direction_up)
				return car_ahead_dir;
			else if (fabsf(main_car->position.x - c->position.x) < 4.f)
			{
				direction grass_wide_collision = grass_collision_check(data, c->position, 10.f);

				if (grass_wide_collision == direction_left)
					return direction_right;
				else if (grass_wide_collision == direction_right)
					return direction_left;
				else if (main_car->move_angle > 0.f)
					return direction_left;
				else if (main_car->move_angle < 0.f)
					return direction_right;
				else
					if (main_car->position.x > c->position.x)
						return direction_right;
					else
						return direction_left;
			}
		}
	}

	return direction_up;
}

void clean_entities(game_data* data)
{
	float main_car_pos = data->entities[entity_main_car][0]->position.y;

	for (int i = 0; i < entity_count; i++)
		while (data->entities[i].size() != 0 &&
			   data->entities[i][0]->position.y < main_car_pos - game_data::destroy_back)
			data->entities[i].erase(data->entities[i].begin());

	for (int j = entity_cars_first; j <= entity_cars_last; j++)
		if (j != entity_main_car)
			for (int i = 0; i < data->entities[j].size();) {
				car* c = (car*)data->entities[j][i].get();
				if (c->position.y < main_car_pos - game_data::destroy_back ||
				   c->position.y > main_car_pos + game_data::destroy_front)
					data->entities[j].erase(data->entities[j].begin() + i);
				else if (c->animation.size() == 0) {
					if (j == entity_tank_car)
						data->score += 300;
					else if (j == entity_trap_car)
						data->score += 150;
					else if (j == entity_regular_car)
						data->score -= 150;
					data->entities[j].erase(data->entities[j].begin() + i);
				}
				else
					i++;
			}

	for (int i = 0; i < data->entities[entity_bullet].size();) {
		bullet* b = (bullet*)data->entities[entity_bullet][i].get();
		if (b->lifetime <= 0.f) {
			if (b->explodes)
				data->entities[entity_explosion].add(new explosion(b->position, game_data::explosion_time));
			data->entities[entity_bullet].erase(data->entities[entity_bullet].begin() + i);
		}
		else
			i++;
	}

	for (int i = 0; i < data->entities[entity_explosion].size();) {
		explosion* e = (explosion*)data->entities[entity_explosion][i].get();
		if (e->lifetime <= 0.f)
			data->entities[entity_explosion].erase(data->entities[entity_explosion].begin() + i);
		else
			i++;
	}
}
void generate_road(game_data* data)
{
	float opt = random_float(data->random_seed);
	data->road_size_first += (opt > 0.8f ? 1 : opt < 0.2f ? -1 : 0) * 0.02f;
	data->road_size_first = clamp(data->road_size_first, -0.06f, 0.06f);
	data->road_size_zeroth += data->road_size_first;

	data->road_size_zeroth = clamp(data->road_size_zeroth, (float)game_data::road_min, (float)game_data::road_max);

	opt = random_float(data->random_seed);
	data->road_pos_first += (opt > 0.8f ? 1 : opt < 0.2f ? -1 : 0) * 0.02f;
	data->road_pos_first = clamp(data->road_pos_first, -0.06f, 0.06f);
	data->road_pos_zeroth += data->road_pos_first;

	float max_offset = max(game_data::game_width / 2 - game_data::grass_min_left - data->road_size_zeroth / 2, 0.f);
	data->road_pos_zeroth = clamp(data->road_pos_zeroth, -max_offset, max_offset);

	coord pos = { data->road_pos_zeroth, (float)data->generation_pos };
	data->entities[entity_grass].add(new grass(pos, data->road_size_zeroth, direction_left));
	data->entities[entity_grass].add(new grass(pos, data->road_size_zeroth, direction_right));
}
void generate_cars(game_data* data)
{
	if (data->car_cooldown-- <= 0 && random_float(data->random_seed) < 0.08f) {
		data->car_cooldown = 50;

		size_t regular_count = data->entities[entity_regular_car].size();
		size_t trap_count = data->entities[entity_trap_car].size();
		size_t tank_count = data->entities[entity_tank_car].size();
		size_t enemy_count = trap_count + tank_count;

		static constexpr int max_regular_count = 3;
		static constexpr int max_enemy_count = 2;

		bool generate_regular = false;
		bool generate_enemy = false;

		if (regular_count == enemy_count && regular_count < max_regular_count && enemy_count < max_enemy_count)
			if (random_float(data->random_seed) <= 0.5f)
				generate_regular = true;
			else
				generate_enemy = true;
		else if (regular_count < max_regular_count && (regular_count < enemy_count || enemy_count >= max_enemy_count))
			generate_regular = true;
		else if (enemy_count < max_enemy_count && (enemy_count < regular_count || regular_count >= max_regular_count))
			generate_enemy = true;

		if (generate_regular) {
			float x_rand = random_float(data->random_seed);
			float min_width = data->road_size_zeroth / 2.f - 6.f;
			float pos_x = 2.f * (x_rand - 0.5f) * min_width;

			dynamic_array<sprites> anim;
			anim.add(sprite_regular_car);
			data->entities[entity_regular_car].add(new car(
				{ pos_x + data->road_pos_zeroth,(float)data->generation_pos },
				{ 3, 2 }, move(anim), 1.f, 3.f));
		}
		else if (generate_enemy) {
			float x_rand = random_float(data->random_seed);
			float min_width = data->road_size_zeroth / 2.f - 6.f;
			float pos_x = 2.f * (x_rand - 0.5f) * min_width;

			if (trap_count == 0 && (trap_count < tank_count || random_float(data->random_seed) <= 0.5f))
			{
				dynamic_array<sprites> anim;
				anim.add(sprite_trap_car0);
				anim.add(sprite_trap_car1);
				data->entities[entity_trap_car].add(new car(
					{ pos_x + data->road_pos_zeroth,(float)data->generation_pos },
					{ 2.5f, 2 }, move(anim), 0.3f, 3.f, 10, game_data::trap_cooldown));
			}
			else
			{
				dynamic_array<sprites> anim;
				anim.add(sprite_tank_car);
				data->entities[entity_tank_car].add(new car(
					{ pos_x + data->road_pos_zeroth,(float)data->generation_pos },
					{ 3, 3 }, move(anim), 1.f, 3.f, 20));
			}
		}
		else
			data->car_cooldown = 0;
	}
}
void generate(game_data* data, bool should_generate_cars)
{
	int main_car_y_offset = (int)data->entities[entity_main_car][0]->position.y;

	for (; data->generation_pos - main_car_y_offset < game_data::destroy_front; data->generation_pos++)
	{
		generate_road(data);

		if (data->tree_cooldown-- <= 0 && random_float(data->random_seed) < 0.05f) {
			data->tree_cooldown = 40;
			float x_rand = random_float(data->random_seed);

			float available_width = game_data::game_width - data->road_size_zeroth - 8.f;
			float width = -game_data::game_width / 2.f + x_rand * available_width + 2.f;
			if (width > data->road_pos_zeroth - data->road_size_zeroth / 2.f - 2.f)
				width += data->road_size_zeroth + 4.f;

			data->entities[entity_tree].add(new tree({ width, (float)data->generation_pos }));
		}

		if (data->puddle_cooldown-- <= 0 && random_float(data->random_seed) < 0.01f) {
			data->puddle_cooldown = 60;
			float x_rand = random_float(data->random_seed);
			float min_width = data->road_size_zeroth / 2.f - 3.f;
			float pos_x = 2.f * (x_rand - 0.5f) * min_width;

			data->entities[entity_puddle].add(new puddle({ pos_x + data->road_pos_zeroth,
														 (float)data->generation_pos }));
		}

		if (data->box_cooldown-- <= 0 && random_float(data->random_seed) < 0.005f) {
			data->box_cooldown = 300;
			float x_rand = random_float(data->random_seed);
			float min_width = data->road_size_zeroth / 2.f - 3.f;
			float pos_x = 2.f * (x_rand - 0.5f) * min_width;

			data->entities[entity_box].add(new box({ pos_x + data->road_pos_zeroth,
														 (float)data->generation_pos }));
		}

		if (should_generate_cars)
			generate_cars(data);
	}
}

void update_generic_car(game_data* data, car* c, float delta, bool turn_left, bool turn_right)
{
	car* main_car = (car*)data->entities[entity_main_car][0].get();
	static constexpr float turn_speed = 60.f;

	if (turn_left)
		c->move_angle = clamp(c->move_angle + turn_speed * delta, -20.f, 20.f);
	else if (turn_right)
		c->move_angle = clamp(c->move_angle - turn_speed * delta, -20.f, 20.f);
	else
		if (c->move_angle < 0.f)
			c->move_angle = min(c->move_angle + turn_speed * delta, 0.f);
		else
			c->move_angle = max(c->move_angle - turn_speed * delta, 0.f);

	for (int i = 0; i < data->entities[entity_grass].size();)
		if (c->collides(*data->entities[entity_grass][i])) {
			c->destroy();
			break;
		}
		else
			i++;

	for (int i = 0; i < data->entities[entity_bullet].size();) {
		bullet* b = (bullet*)data->entities[entity_bullet][i].get();
		if (b->lifetime > 0.f && c->collides(*b)) {
			c->life--;
			if (c->life <= 0)
				c->destroy();
			b->lifetime = 0.f;
			break;
		}
		else
			i++;
	}
	if (c->explostion_invinc_time <= 0.f)
		for (int i = 0; i < data->entities[entity_explosion].size();) {
			explosion* e = (explosion*)data->entities[entity_explosion][i].get();
			if (e->lifetime > 0.f && c->collides(*e)) {
				c->life -= 7;
				c->explostion_invinc_time = game_data::bazooka_reload;
				if (c->life <= 0)
					c->destroy();
				break;
			}
			else
				i++;
		}
}
void update_regular_cars(game_data* data, float delta)
{
	car* main_car = (car*)data->entities[entity_main_car][0].get();

	for (unique_ptr<entity>& e : data->entities[entity_regular_car]) {
		car* c = (car*)e.get();

		if (c->destroyed) {
			c->move_angle = 0.f;
			continue;
		}

		if (c->position.y > main_car->position.y + game_data::destroy_front / 2)
			c->speed = 0.f;
		else
			c->speed = (main_car->speed * 1.2f + game_data::max_speed * 0.4f) / 2.f;

		bool turn_left = false;
		bool turn_right = false;

		direction turn_dir = regular_car_turn_behaviour(data, c, &c->speed);
		if (turn_dir == direction_left)
			turn_left = true;
		else if (turn_dir == direction_right)
			turn_right = true;

		update_generic_car(data, c, delta, turn_left, turn_right);

		if (c->invinc_time <= 0.f)
			for (int i = entity_cars_first; i <= entity_cars_last; i++)
				for (unique_ptr<entity>& e : data->entities[i]) {
					car* other = (car*)e.get();
					if (c != other && !other->destroyed && c->collides(*other) &&
						!(other == main_car && data->car_state == running_state::enter)) {
						c->destroy();
						other->speed = min(c->speed * 0.8f, other->speed);
					}
				}
	}
}
void update_trap_cars(game_data* data, float delta)
{
	car* main_car = (car*)data->entities[entity_main_car][0].get();

	for (unique_ptr<entity>& e : data->entities[entity_trap_car]) {
		car* c = (car*)e.get();

		if (c->destroyed) {
			c->move_angle = 0.f;
			continue;
		}

		if (c->position.y > main_car->position.y + game_data::destroy_front / 2)
			c->speed = 0.f;
		else
			c->speed = (main_car->speed * 1.2f + game_data::max_speed * 0.4f) / 2.f;

		if (c->action_time <= 0.f) {
			c->action_time = c->action_cooldown;
			data->entities[entity_trap].add(new trap(c->position));
		}

		bool turn_left = false;
		bool turn_right = false;

		direction turn_dir = trap_car_turn_behaviour(data, c, &c->speed);
		if (turn_dir == direction_left)
			turn_left = true;
		else if (turn_dir == direction_right)
			turn_right = true;

		update_generic_car(data, c, delta, turn_left, turn_right);
	}
}
void update_tank_cars(game_data* data, float delta)
{
	car* main_car = (car*)data->entities[entity_main_car][0].get();

	for (unique_ptr<entity>& e : data->entities[entity_tank_car]) {
		car* c = (car*)e.get();

		if (c->destroyed) {
			c->move_angle = 0.f;
			continue;
		}

		if (c->position.y > main_car->position.y + game_data::destroy_front / 2)
			c->speed = 0.f;
		else {
			if (c->position.y > main_car->position.y + 10.f)
				c->speed = (main_car->speed * 1.2f + game_data::max_speed * 0.4f) / 2.f;
			else if (c->position.y > main_car->position.y + 2.f)
				c->speed = (main_car->speed * 0.4f + game_data::max_speed * 1.0f) / 2.f;
			else if (c->position.y > main_car->position.y - 10.f)
				c->speed = (main_car->speed * 1.0f + game_data::max_speed * 0.8f) / 2.f;
			else
				c->speed = (main_car->speed * 1.4f + game_data::max_speed * 1.0f) / 2.f;
		}

		bool turn_left = false;
		bool turn_right = false;

		direction turn_dir = tank_car_turn_behaviour(data, c);
		if (turn_dir == direction_left)
			turn_left = true;
		else if (turn_dir == direction_right)
			turn_right = true;

		update_generic_car(data, c, delta, turn_left, turn_right);
	}
}

void update_main_car_enter(game_data* data)
{
	data->car_state = running_state::enter;
	data->car_state_left = game_data::enter_time;

	float pos_y = 0.f;
	if (data->entities[entity_main_car].size() != 0)
		pos_y = data->entities[entity_main_car][0]->position.y;
	data->entities[entity_main_car] = dynamic_array<unique_ptr<entity>>();

	float pos_x = data->road_pos_zeroth;

	dynamic_array<sprites> main_car_anim;
	main_car_anim.add(sprite_main_car0);
	main_car_anim.add(sprite_main_car1);
	data->entities[entity_main_car].add(new car({ pos_x, pos_y }, { 3.f, 2.5f }, move(main_car_anim), 0.3f));
}
void update_main_car(game_data* data, float delta)
{
	bool infinite_lives = data->elapsed_time / 1000 < game_data::free_respawn_time;

	bool accelerate = data->arrows[direction_up];
	bool deaccelerate = data->arrows[direction_down];
	bool move_left = data->arrows[direction_left];
	bool move_right = data->arrows[direction_right];

	if (data->car_state_left <= 0.f)
		if (data->car_state == running_state::destroy)
			if (infinite_lives)
				update_main_car_enter(data);
			else if (data->lives > 0) {
				data->lives--;
				update_main_car_enter(data);
			}
			else {
				data->lives--;
				save_score(data);
				data->state = game_state::finished;
			}
		else
			data->car_state = running_state::normal;

	car* main_car = (car*)(data->entities[entity_main_car][0].get());

	if (data->car_state != running_state::enter)
	{
		if (data->car_state == running_state::slow || data->car_state == running_state::destroy) {
			accelerate = false;
			deaccelerate = true;
		}
		if (data->car_state == running_state::destroy) {
			move_left = false;
			move_right = false;
		}

		if (accelerate) main_car->speed += (game_data::acceleration + game_data::friction) * delta;
		if (deaccelerate) main_car->speed -= game_data::deaccelerate * delta;
		main_car->speed -= game_data::friction * delta;
		main_car->speed = max(min(main_car->speed, game_data::max_speed), 0.f);

		main_car->move_angle = 0.f;
		if (move_left) main_car->move_angle -= 25.f;
		if (move_right) main_car->move_angle += 25.f;
	}

	if (main_car->position.y - data->last_dist_score_checkpoint > 50) {
		data->score += 50;
		data->last_dist_score_checkpoint = (long)main_car->position.y;
	}

	if (data->score - data->last_life_checkpoint > (infinite_lives ? 2000 : 5000)) {
 		data->lives++;
		data->last_life_checkpoint = data->score;
	}

	data->bullet_cooldown -= delta;

	if (data->car_state != running_state::enter && data->car_state != running_state::destroy)
		if (data->shooting && data->bullet_cooldown <= 0.f) {
			if (data->bazooka_left-- <= 0) {
				data->bullet_cooldown = game_data::bullet_reload;
				data->entities[entity_bullet].add(new bullet(main_car->position,
															 game_data::bullet_speed + main_car->speed,
															 game_data::bullet_lifetime));
			}
			else {
				data->bullet_cooldown = game_data::bazooka_reload;
				data->entities[entity_bullet].add(new bullet(main_car->position,
															 game_data::bullet_speed + main_car->speed,
															 game_data::bullet_lifetime, true));
			}
		}

	data->car_state_left -= delta;
}
void update_main_car_collisions(game_data* data)
{
	car* main_car = (car*)(data->entities[entity_main_car][0].get());

	for (int i = 0; i < data->entities[entity_puddle].size();)
		if (main_car->collides(*data->entities[entity_puddle][i])) {
			data->entities[entity_puddle].erase(data->entities[entity_puddle].begin() + i);
			data->car_state = running_state::slow;
			data->car_state_left = game_data::slow_time;
			break;
		}
		else
			i++;

	for (int i = 0; i < data->entities[entity_trap].size();)
		if (main_car->collides(*data->entities[entity_trap][i])) {
			data->entities[entity_trap].erase(data->entities[entity_trap].begin() + i);
			data->car_state = running_state::destroy;
			data->car_state_left = game_data::destroy_time;
			main_car->destroy();
			return;
		}
		else
			i++;

	for (int i = 0; i < data->entities[entity_box].size();)
		if (main_car->collides(*data->entities[entity_box][i])) {
			data->entities[entity_box].erase(data->entities[entity_box].begin() + i);
			data->bazooka_left = 6;
			break;
		}
		else
			i++;

	for (int i = 0; i < data->entities[entity_grass].size();)
		if (main_car->collides(*data->entities[entity_grass][i])) {
			data->car_state = running_state::destroy;
			data->car_state_left = game_data::destroy_time;
			main_car->destroy();
			return;
		}
		else
			i++;

	for (int j = entity_trap_car; j <= entity_tank_car; j++)
		for (int i = 0; i < data->entities[j].size();) {
			car* enemy = (car*)data->entities[j][i].get();
			if (!enemy->destroyed && main_car->collides(*enemy)) {
				data->car_state = running_state::destroy;
				data->car_state_left = game_data::destroy_time;
				main_car->destroy();
				return;
			}
			else
				i++;
		}
}
void update(game_data* data)
{
	if (data->state != game_state::running)
		return;

	Uint32 frame_time = SDL_GetTicks();
	Uint32 frame_diff = frame_time - data->last_frame_time;
	data->last_frame_time = frame_time;

	data->elapsed_time += frame_diff;
	float delta = frame_diff / 1000.f;

	car* main_car = (car*)(data->entities[entity_main_car][0].get());
	update_main_car(data, delta);

	if (data->car_state == running_state::enter)
		main_car->render_position_offset.y = min(1.f - (data->car_state_left / game_data::enter_time) * 
												 game_data::destroy_back, 0.f);
	else
		main_car->render_position_offset.y = 0.f;

	if (data->car_state != running_state::destroy && data->car_state != running_state::enter)
		update_main_car_collisions(data);

	update_regular_cars(data, delta);
	update_tank_cars(data, delta);
	update_trap_cars(data, delta);

	for (int i = 0; i < entity_count; i++)
		for (unique_ptr<entity>& e : data->entities[i])
			e->update(delta);

	generate(data, true);
	clean_entities(data);
}

void new_game(game_data* data)
{
	data->state = game_state::running;
	for (int i = 0; i < entity_count; i++)
		data->entities[i] = dynamic_array<unique_ptr<entity>>();

	data->random_seed = ((unsigned long long)(SDL_GetTicks()) << 32) + SDL_GetTicks();
	data->score = 0;
	data->last_dist_score_checkpoint = 0;
	data->elapsed_time = 0;
	data->last_frame_time = SDL_GetTicks();

	data->lives = 0;
	data->last_life_checkpoint = 0;

	data->tree_cooldown = 50;
	data->puddle_cooldown = 50;
	data->box_cooldown = 400;
	data->car_cooldown = 0;
	data->bullet_cooldown = 0.f;
	data->bazooka_left = 0;

	data->road_size_zeroth = 0.f;
	data->road_size_first = 0.f;
	data->road_pos_zeroth = 0.f;
	data->road_pos_first = 0.f;

	data->car_state = running_state::destroy;
	data->car_state_left = 0.f;
	update_main_car(data, 0.f);

	data->generation_pos = -game_data::destroy_back;
	generate(data, false);
}

void get_file_path(char** path);

void save_game(game_data* data)
{
	time_t rawtime;
	tm* timeinfo;

	time(&rawtime);

	tm local_time;
	localtime_s(&local_time, &rawtime);
	timeinfo = &local_time;

	char path[128];
	sprintf_s(path, sizeof(path), "saves/%04d-%02d-%02dT%02d-%02d-%02d", 
			  timeinfo->tm_year + 1900, timeinfo->tm_mday, timeinfo->tm_mon + 1,
			  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	FILE* file;
	fopen_s(&file, path, "wb");

	if (!file)
		return;

	fwrite(game_data::save_file_prefix, sizeof(game_data::save_file_prefix), 1, file);
	fwrite(data, sizeof(*data) - sizeof(game_data::entities), 1, file);
	for (int i = 0; i < entity_count; i++) {
		size_t count = data->entities[i].size();
		fwrite(&count, sizeof(size_t), 1, file);
		for (int j = 0; j < count; j++)
			data->entities[i][j]->save(file);
	}

	fclose(file);
}
void load_game(game_data* data)
{
	char* path = NULL;
	get_file_path(&path);

	if (path == NULL)
		return;

	char text_check[sizeof(game_data::save_file_prefix)] = {};

	FILE* file;
	fopen_s(&file, path, "rb");

	if (file)
	{
		fread(text_check, sizeof(game_data::save_file_prefix), 1, file);

		if (strcmp(text_check, game_data::save_file_prefix) == 0)
		{
			fread(data, sizeof(*data) - sizeof(game_data::entities), 1, file);
			for (int i = 0; i < entity_count; i++) {
				size_t count;
				fread(&count, sizeof(size_t), 1, file);
				data->entities[i] = dynamic_array<unique_ptr<entity>>(count);
				for (int j = 0; j < count; j++) {
					entity* loaded_entity;
					switch (i)
					{
						case entity_grass: loaded_entity = new grass(file);  break;
						case entity_puddle: loaded_entity = new puddle(file); break;
						case entity_box: loaded_entity = new box(file); break;
						case entity_tree: loaded_entity = new tree(file); break;
						case entity_trap: loaded_entity = new trap(file); break;
						case entity_bullet:
						case entity_bazooka: loaded_entity = new bullet(file); break;
						case entity_explosion: loaded_entity = new explosion(file); break;

						default: loaded_entity = new car(file); break;
					}
					data->entities[i][j] = unique_ptr<entity>(loaded_entity);
				}
			}
		}
		fclose(file);
	}

	if (path)
		free(path);
}

void update_game_state(game_data* data, Uint32 input_type)
{
	switch (input_type)
	{
		case SDLK_p:
			if (data->state == game_state::running)
				data->state = game_state::paused;
			else if (data->state == game_state::paused) {
				data->last_frame_time = SDL_GetTicks();
				data->state = game_state::running;
			}
			break;
		case SDLK_y:
			if (data->state == game_state::score_points) {
				data->last_frame_time = SDL_GetTicks();
				data->state = data->lives == -1 ? game_state::finished : game_state::running;
			}
			else if (data->state != game_state::quit)
				data->state = game_state::score_points;
			break;
		case SDLK_t:
			if (data->state == game_state::score_time) {
				data->last_frame_time = SDL_GetTicks();
				data->state = data->lives == -1 ? game_state::finished : game_state::running;
			}
			else if (data->state != game_state::quit)
				data->state = game_state::score_time;
			break;
	}
}
int SDL_main(int argc, char* argv[])
{
	unique_ptr<stdout_redirect> redirect(new stdout_redirect("output.txt"));
	unique_ptr<screen_type> screen(new screen_type("SpyHunter", 640, 480));
	unique_ptr<font_type> font(new font_type("cs8x8.bmp", screen->renderer));

	unique_ptr<game_data> data(new game_data());
	new_game(data.get());

	render_data_type render_data = { screen.get(), data.get(), font.get() };
	load_textures(render_data.textures, screen.get());

	while (data->state != game_state::quit)
	{
		update(data.get());
		draw(render_data);
		screen->update();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE: data->state = game_state::quit; break;
						case SDLK_n: new_game(data.get()); break;
						case SDLK_p: case SDLK_t: case SDLK_y: update_game_state(data.get(), event.key.keysym.sym); break;
						case SDLK_s: if (data->state == game_state::running) save_game(data.get()); 
							data->last_frame_time = SDL_GetTicks(); break;
						case SDLK_l: load_game(data.get()); data->last_frame_time = SDL_GetTicks(); break;
						case SDLK_UP: data->arrows[direction_up] = true; break;
						case SDLK_DOWN: data->arrows[direction_down] = true; break;
						case SDLK_LEFT: data->arrows[direction_left] = true; break;
						case SDLK_RIGHT: data->arrows[direction_right] = true; break;
						case SDLK_SPACE: data->shooting = true; break;
					}
					break;
				case SDL_KEYUP:
					switch (event.key.keysym.sym)
					{
						case SDLK_UP: data->arrows[direction_up] = false; break;
						case SDLK_DOWN: data->arrows[direction_down] = false; break;
						case SDLK_LEFT: data->arrows[direction_left] = false; break;
						case SDLK_RIGHT: data->arrows[direction_right] = false; break;
						case SDLK_SPACE: data->shooting = false; break;
					}
					break;
				case SDL_QUIT:
					data->state = game_state::quit;
					break;
			};
		};
	};

	return EXIT_SUCCESS;
};
