#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "../SDL2-2.0.10/include/SDL.h"
#include "../SDL2-2.0.10/include/SDL_main.h"
}

template<typename Type>
Type&& move(Type& val)
{
	return (Type&&)val;
}
template<typename Type>
Type min(Type a, Type b)
{
	return a < b ? a : b;
}
template<typename Type>
Type max(Type a, Type b)
{
	return a > b ? a : b;
}
template<typename Type>
Type clamp(Type v, Type min_v, Type max_v)
{
	return v > max_v ? max_v : v < min_v ? min_v : v;
}
template<typename Type>
Type abs(Type a)
{
	return a >= 0 ? a : -a;
}

template<typename Type>
struct unique_ptr
{
public:
	unique_ptr() noexcept
		:ptr_(nullptr)
	{
	}
	unique_ptr(Type* ptr) noexcept
		:ptr_(ptr)
	{
	}

	unique_ptr(unique_ptr&& other) noexcept
		:ptr_(other.ptr_)
	{
		other.ptr_ = nullptr;
	}
	unique_ptr& operator=(unique_ptr&& other) noexcept
	{
		this->reset();
		this->ptr_ = other.ptr_;
		other.ptr_ = nullptr;
		return *this;
	}

	unique_ptr(const unique_ptr&) = delete;
	unique_ptr& operator=(const unique_ptr&) = delete;

	~unique_ptr() noexcept
	{
		this->reset();
	}

	const Type* get() const noexcept
	{
		return this->ptr_;
	}
	const Type* operator->() const noexcept
	{
		return this->ptr_;
	}
	const Type& operator*() const noexcept
	{
		return *this->ptr_;
	}


	Type* get() noexcept
	{
		return this->ptr_;
	}
	Type* operator->() noexcept
	{
		return this->ptr_;
	}
	Type& operator*() noexcept
	{
		return *this->ptr_;
	}

	void reset()
	{
		if (this->ptr_) {
			delete this->ptr_;
			this->ptr_ = nullptr;
		}
	}

private:
	Type* ptr_;
};

template<typename Type>
class dynamic_array
{
public:
	dynamic_array() noexcept = default;
	explicit dynamic_array(size_t size)
	{
		this->resize(size);
	}
	
	dynamic_array(const dynamic_array&) = delete;
	dynamic_array& operator=(const dynamic_array&) = delete;

	dynamic_array(dynamic_array&& other) noexcept
	{
		this->data_ = other.data_;
		this->size_ = other.size_;
		this->capacity_ = other.capacity_;

		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}
	dynamic_array& operator=(dynamic_array&& other) noexcept
	{
		if (this->data_)
			delete[] this->data_;

		this->data_ = other.data_;
		this->size_ = other.size_;
		this->capacity_ = other.capacity_;

		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;

		return *this;
	}

	~dynamic_array() noexcept
	{
		if (this->data_)
			delete[] this->data_;
	}

	Type* begin() const noexcept
	{
		return this->data_;
	}
	Type* end() const noexcept
	{
		return this->data_ + this->size_;
	}

	size_t size() const noexcept
	{
		return this->size_;
	}

	void add(const Type& to_add)
	{
		this->reallocate(this->size_ + 1);
		this->data_[this->size_] = to_add;
		this->size_++;
	}
	void add(Type&& to_add)
	{
		this->reallocate(this->size_ + 1);
		this->data_[this->size_] = move(to_add);
		this->size_++;
	}
	void add(Type* first, Type* last)
	{
		this->reallocate(this->size_ + (last - first));
		Type* it = this->data_ + this->size_;
		this->size_ += last - first;

		while (first != last)
			*it++ = move(*first++);
	}

	template<typename Type2>
	Type* find(const Type2& val)
	{
		for (Type* it = this->data_, *e = this->data_ + this->size_; it != e; ++it)
			if (*it == val)
				return it;

		return nullptr;
	}

	void erase(Type* it)
	{
		for (Type* e = this->data_ + this->size_; it + 1 != e; ++it)
			*it = move(*(it + 1));
		this->data_[this->size_ - 1] = Type();
		this->size_--;
	}

	void resize(size_t new_size)
	{
		this->reallocate(new_size);
		this->size_ = new_size;
	}
	void reallocate(size_t new_size)
	{
		if (this->capacity_ < new_size)
		{
			this->capacity_ = max(this->capacity_ + this->capacity_ / 2, new_size);
			Type* new_data = new Type[this->capacity_];
			if (this->data_)
			{
				Type* src = this->data_;
				Type* src_end = this->data_ + this->size_;
				Type* dst = new_data;
				while (src != src_end)
					*dst++ = move(*src++);
				delete[] this->data_;
			}
			this->data_ = new_data;
		}
	}

	Type& operator[](size_t i)
	{
		return this->data_[i];
	}

private:
	Type* data_ = nullptr;
	size_t size_ = 0;
	size_t capacity_ = 0;
};

struct point
{
	int x = 0;
	int y = 0;
};
struct coord
{
	float x = 0;
	float y = 0;
};
struct color
{
	static constexpr color white()
	{
		return { 0xff, 0xff, 0xff, 0xff };
	}
	static constexpr color black()
	{
		return { 0, 0, 0, 0xff };
	}
	static constexpr color gray()
	{
		return { 0x3f, 0x3f, 0x3f, 0xff };
	}
	static constexpr color green()
	{
		return { 0, 0x7f, 0, 0xff };
	}
	static constexpr color red()
	{
		return { 0x7f, 0, 0, 0xff };
	}
	static constexpr color yellow()
	{
		return { 0x7f, 0x7f, 0, 0xff };
	}

	unsigned char r, g, b, a;
};

inline float random_float(unsigned long long& state) noexcept
{
	unsigned long long oldstate = state;
	state = oldstate * 6364136223846793005ULL + 1442695040888963407ULL;
	unsigned long xorshifted = static_cast<unsigned long>(((oldstate >> 18u) ^ oldstate) >> 27u);
	long rot = static_cast<long>(oldstate >> 59u);
	unsigned long result = (xorshifted >> rot) | (xorshifted << (32 - rot));
	return (float)result / 0xffffffff;
}

class stdout_redirect
{
public:
	stdout_redirect(const char* target)
	{
		freopen_s(&this->output_, target, "a+", stdout);
		printf("\nBegan logging to %s.\n", target);
	}
	~stdout_redirect() noexcept
	{
		fclose(this->output_);
	}

private:
	FILE* output_;
};

struct screen_type
{
	screen_type(const char* title, int width, int height)
		:width(width), height(height)
	{
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			printf("SDL_Init error: %s.\n", SDL_GetError());
			throw EXIT_FAILURE;
		}

		if (SDL_CreateWindowAndRenderer(this->width, this->height, 0, &this->window, &this->renderer) != 0) {
			SDL_Quit();
			printf("SDL_CreateWindowAndRenderer error: %s.\n", SDL_GetError());
			throw EXIT_FAILURE;
		};

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_RenderSetLogicalSize(this->renderer, this->width, this->height);
		SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);

		SDL_SetWindowTitle(this->window, title);
	}
	~screen_type() noexcept
	{
		SDL_DestroyRenderer(this->renderer);
		SDL_DestroyWindow(this->window);

		SDL_Quit();
	}

	screen_type(const screen_type&) = delete;
	screen_type(screen_type&&) = delete;
	screen_type& operator=(const screen_type&) = delete;
	screen_type& operator=(screen_type&&) = delete;

	void update()
	{
		SDL_RenderPresent(this->renderer);
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
		SDL_RenderClear(this->renderer);
	}

	SDL_Window* window;
	SDL_Renderer* renderer;

	int width, height;
};

struct font_type
{
	font_type(const char* bmp_file_path, SDL_Renderer* renderer)
	{
		SDL_Surface* surface = SDL_LoadBMP(bmp_file_path);
		if (surface == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", bmp_file_path, SDL_GetError());
			throw EXIT_FAILURE;
		};
		SDL_SetColorKey(surface, true, 0x000000);

		this->charset = SDL_CreateTextureFromSurface(renderer, surface);
	}
	~font_type() noexcept
	{
		SDL_DestroyTexture(this->charset);
	}

	font_type(const font_type&) = delete;
	font_type(font_type&&) = delete;
	font_type& operator=(const font_type&) = delete;
	font_type& operator=(font_type&&) = delete;

	SDL_Texture* charset;
};
struct texture_type
{
	texture_type(const char* bmp_file_path, SDL_Renderer* renderer)
	{
		SDL_Surface* bmp_surface = SDL_LoadBMP(bmp_file_path);
		if (bmp_surface == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", bmp_file_path, SDL_GetError());
			throw EXIT_FAILURE;
		};
		SDL_Surface* surface = SDL_ConvertSurfaceFormat(bmp_surface, SDL_PIXELFORMAT_RGBA8888, 0);

		for (int y = 0; y < surface->h; y++)
			for (int x = 0; x < surface->w; x++) {
				Uint32* p = (Uint32*)((Uint8*)surface->pixels + y * surface->pitch + 
									  x * surface->format->BytesPerPixel);
				if (*p == 0xffffffff)
					*p = 0x00000000;
			}

		this->texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_SetTextureBlendMode(this->texture, SDL_BLENDMODE_BLEND);
		
		this->width = surface->w;
		this->height = surface->h;

		SDL_FreeSurface(bmp_surface);
		SDL_FreeSurface(surface);
	}

	~texture_type() noexcept
	{
		SDL_DestroyTexture(this->texture);
	}

	texture_type(const texture_type&) = delete;
	texture_type(texture_type&&) = delete;
	texture_type& operator=(const texture_type&) = delete;
	texture_type& operator=(texture_type&&) = delete;

	SDL_Texture* texture;
	int width, height;
};


void draw_rect(screen_type* screen, point pos, point size, color c)
{
	SDL_Rect rect = { pos.x, pos.y, size.x, size.y };
	SDL_SetRenderDrawColor(screen->renderer, c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(screen->renderer, &rect);
}
void draw_texture(screen_type* screen, const texture_type* texture, point center)
{
	SDL_Rect rect = { center.x - texture->width / 2, center.y - texture->height / 2,
		texture->width, texture->height };

	SDL_RenderCopy(screen->renderer, texture->texture, NULL, &rect);
}
void draw_texture(screen_type* screen, const texture_type* texture, point center, float angle)
{
	SDL_Point point = { texture->width / 2, texture->height / 2 };
	SDL_Rect rect = { center.x - texture->width / 2, center.y - texture->height / 2,
		texture->width, texture->height };

	SDL_RenderCopyEx(screen->renderer, texture->texture, NULL, &rect, angle, &point, SDL_FLIP_NONE);
}

void draw_text(screen_type* screen, const font_type* font, const char* text, point pos)
{
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = pos.x;
		d.y = pos.y;
		SDL_RenderCopy(screen->renderer, font->charset, &s, &d);
		pos.x += 8;
		text++;
	};
};
void draw_text_center(screen_type* screen, const font_type* font, const char* text, point pos)
{
	pos.x -= (int)strlen(text) * 4;
	draw_text(screen, font, text, pos);
}
void draw_text_right(screen_type* screen, const font_type* font, const char* text, point pos)
{
	pos.x -= (int)strlen(text) * 8;
	draw_text(screen, font, text, pos);
}
