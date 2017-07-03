#include <SDL2/SDL_image.h>
#include <stdint.h>
#include "math3d.h"
#include "texture.h"


namespace raytracer {

using math3d::V3D;

V3D Texture::GetColorAt(double u, double v, double distance) const {
  (void)distance; // TODO(gynvael): Add mipmaps.
  // TODO(gynvael): Add bilinear filtring.
  u = fmod(u, 1.0);
  v = fmod(v, 1.0);
  if (u < 0.0) u += 1.0;
  if (v < 0.0) v += 1.0;

  // Flip the vertical.
  v = 1.0 - v;

  size_t x = (int)(u * (width - 1));
  size_t y = (int)(v * (height - 1));

  return colors.at(x + y * width);
}

Texture *Texture::LoadFromFile(const char *fname) {
  fprintf(stderr, "info: loading texture \"%s\"\n", fname);
  struct SDLSurfaceDeleter {
    void operator()(SDL_Surface *s) const {
      SDL_FreeSurface(s);
    }
  };

  std::unique_ptr<SDL_Surface, SDLSurfaceDeleter> s(IMG_Load(fname));
  if (s == nullptr) {
    return nullptr;
  }

  // Some sanity checks.
  if (s->w <= 0 || s->h <= 0 || s->w > 30000 || s->h > 30000) {
    fprintf(stderr, "error: texture pretty insane \"%s\" (%i, %i)\n",
        fname, s->w, s->h);
    return nullptr;
  }

  // Convert to RGB if needed.
  if (s->format->format != SDL_PIXELFORMAT_RGBA32) {
    fprintf(stderr, "info: converting texture to RGBA32\n");
    s.reset(SDL_ConvertSurfaceFormat(s.get(), SDL_PIXELFORMAT_RGBA32, 0));
    if (s == nullptr) {
      fprintf(stderr, "error: texture conversion failed \"%s\"\n", fname);
      return nullptr;
    }
  }

  // Allocate and convert texture.
  std::unique_ptr<Texture> tex(new Texture);
  tex->width = (size_t)s->w;
  tex->height = (size_t)s->h;
  tex->colors.resize(tex->width * tex->height);

  size_t idx = 0;
  uint8_t *px = (uint8_t*)s->pixels;
  for (size_t j = 0; j < tex->height; j++) {
    for (size_t i = 0; i < tex->width; i++, idx++, px += 4) {
      tex->colors[idx] = {
        (double)px[0] / 255.0,
        (double)px[1] / 255.0,
        (double)px[2] / 255.0
      };
    }
  }
  
  return tex.release();
}

};  // namespace raytracer

