//#########################
//Pekka Kana 2
//Copyright (c) 2003 Janne Kivilahti
//#########################
#include "engine/PRender.hpp"

#include "engine/PLog.hpp"

#include "PSdl.hpp"

#include <SDL.h>

#include <stdexcept>

void PSdl::load_ui_texture(void* surface) {

    ui_surface = (SDL_Surface*)surface;
    ui_texture = SDL_CreateTextureFromSurface(renderer, (SDL_Surface*)ui_surface);
    if (ui_texture == NULL) {

        PLog::Write(PLog::ERR, "PSdl", "Couldn't load texture!");

    }

}
void PSdl::render_ui(PRender::FRECT src, PRender::FRECT dst, float alpha) {

    RenderOptions opt;

    opt.src = src;
    opt.dst = dst;
    opt.alpha = alpha;

    render_list.push_back(opt);

}

void PSdl::clear_screen() {

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

}

void PSdl::set_screen(PRender::FRECT screen_dst) {

    int w, h;
    PRender::get_window_size(&w, &h);

    screen_dest.x = screen_dst.x * w;
    screen_dest.y = screen_dst.y * h;
    screen_dest.w = screen_dst.w * w;
    screen_dest.h = screen_dst.h * h;

}

int PSdl::set_shader(int mode) {

    const bool nearest = mode == PRender::SHADER_NEAREST;
    if (!nearest && mode != PRender::SHADER_LINEAR)
        return 1;

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, nearest ? "0" : "2"))
        return 1;

    // Previously the next frame's newly created texture picked up this hint.
    if (framebuffer_texture)
        return SDL_SetTextureScaleMode(framebuffer_texture,
            nearest ? SDL_ScaleModeNearest : SDL_ScaleModeBest) != 0;

    return 0;
}

int PSdl::set_vsync(bool set) {

    if(renderer!=nullptr && SDL_RenderSetVSync(renderer, set)!=0){
        PLog::Write(PLog::ERR, "PSDL", "Couldn't set vsync %s", SDL_GetError());
    }

    /*Uint32 sync = set? SDL_RENDERER_PRESENTVSYNC : 0;

    if (renderer) {

        ui_texture = NULL;
        SDL_DestroyRenderer(renderer);

    }

    Uint32 sync = set? SDL_RENDERER_PRESENTVSYNC : 0;

    renderer = SDL_CreateRenderer(curr_window, -1, SDL_RENDERER_ACCELERATED | sync);
    if (!renderer) {

        PLog::Write(PLog::FATAL, "PSdl", "Couldn't create renderer!");
        return -1;

    }

    SDL_RenderClear(renderer);*/

    return 0;

}

void PSdl::reset_textures() {

    SDL_DestroyTexture(framebuffer_texture);
    framebuffer_texture = nullptr;
    SDL_DestroyTexture(ui_texture);
    ui_texture = nullptr;

}

bool PSdl::ensure_framebuffer(int width, int height) {

    if (!framebuffer_surface || framebuffer_surface->w != width || framebuffer_surface->h != height) {
        SDL_DestroyTexture(framebuffer_texture);
        framebuffer_texture = nullptr;
        SDL_FreeSurface(framebuffer_surface);
        framebuffer_surface = SDL_CreateRGBSurfaceWithFormat(
            0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
        if (!framebuffer_surface)
            return false;
    }

    if (!framebuffer_texture)
        framebuffer_texture = SDL_CreateTexture(renderer, framebuffer_surface->format->format,
            SDL_TEXTUREACCESS_STREAMING, width, height);

    return framebuffer_texture != nullptr;

}

void PSdl::update(void* _buffer8) {

    auto* buffer8 = static_cast<SDL_Surface*>(_buffer8);

    // Convert the indexed image with its current palette, including palette animation.
    if (!ensure_framebuffer(buffer8->w, buffer8->h) ||
        SDL_BlitSurface(buffer8, nullptr, framebuffer_surface, nullptr) != 0 ||
        SDL_UpdateTexture(framebuffer_texture, nullptr, framebuffer_surface->pixels,
            framebuffer_surface->pitch) != 0) {
        PLog::Write(PLog::ERR, "PSdl", "Couldn't upload framebuffer: %s", SDL_GetError());
        render_list.clear();
        return;
    }

    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, framebuffer_texture, nullptr, &screen_dest);
    
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    float prop_x = (float)w;
    float prop_y = (float)h;
    
    if (ui_surface) {

        if (!ui_texture)
            ui_texture = SDL_CreateTextureFromSurface(renderer, (SDL_Surface*)ui_surface);

        for (auto opt : render_list) {
            
            u8 mod = opt.alpha * 256;
            if (mod == 0)
                continue;
            
            SDL_SetTextureAlphaMod(ui_texture, mod);
            
            SDL_Rect dst;
            dst.x = opt.dst.x * prop_x;
            dst.y = opt.dst.y * prop_y;
            dst.w = opt.dst.w * prop_x;
            dst.h = opt.dst.h * prop_y;

            SDL_Rect src;
            src.x = opt.src.x * 1024;
            src.y = opt.src.y * 1024;
            src.w = opt.src.w * 1024;
            src.h = opt.src.h * 1024;

            SDL_RenderCopy(renderer, ui_texture, &src, &dst);
            
        }

        render_list.clear();

    }

    SDL_RenderPresent(renderer);

}

PSdl::PSdl(int width, int height, void* window) {

    curr_window = (SDL_Window*)window;

    renderer = SDL_CreateRenderer(curr_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {

        PLog::Write(PLog::FATAL, "PSdl", "Couldn't create renderer!");
		throw std::runtime_error("Cannot create SDL renderer!");
	}

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    SDL_RenderClear(renderer);

}

PSdl::~PSdl() {

    reset_textures();
    SDL_FreeSurface(framebuffer_surface);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(ui_surface);
    PLog::Write(PLog::DEBUG, "PSdl", "Terminated");

}
