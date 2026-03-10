#include "soft_render/render/renderer.hpp"
#include "soft_render/math/mat4.hpp"
#include "../demo/obj_loader.hpp"

#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstring>

using namespace sr;
using namespace sr::math;
using namespace sr::render;
using namespace sr::pipeline;

static const float PI = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Tiny 5x7 bitmap font for the HUD overlay
// Each char is 5 columns of 7 bits (LSB = top row).
// Only ASCII 32-126 are stored; index = ch - 32.
// ---------------------------------------------------------------------------
static const uint8_t FONT5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '\''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x08,0x2A,0x1C,0x2A,0x08}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x00,0x08,0x14,0x22,0x41}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x41,0x22,0x14,0x08,0x00}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x08,0x14,0x54,0x54,0x3C}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x08,0x08,0x2A,0x1C,0x08}, // '~'
};

// Draw a single character at pixel (px, py). Scale = pixel size of each font pixel.
static void drawChar(core::Framebuffer& fb, int px, int py, char ch, int scale,
                     uint8_t r, uint8_t g, uint8_t b)
{
    if (ch < 32 || ch > 126) return;
    const uint8_t* col = FONT5x7[ch - 32];
    for (int cx = 0; cx < 5; ++cx) {
        uint8_t bits = col[cx];
        for (int cy = 0; cy < 7; ++cy) {
            if (bits & (1 << cy)) {
                for (int sy = 0; sy < scale; ++sy)
                    for (int sx = 0; sx < scale; ++sx) {
                        int x = px + cx * scale + sx;
                        int y = py + cy * scale + sy;
                        if (x >= 0 && x < fb.width() && y >= 0 && y < fb.height())
                            fb.setPixel(x, y, {r/255.f, g/255.f, b/255.f});
                    }
            }
        }
    }
}

static void drawString(core::Framebuffer& fb, int px, int py, const char* str,
                       int scale, uint8_t r, uint8_t g, uint8_t b)
{
    for (; *str; ++str, px += (5 + 1) * scale)
        drawChar(fb, px, py, *str, scale, r, g, b);
}

// Draw text with a dark shadow for readability over any background
static void drawHUD(core::Framebuffer& fb, const char* str, int scale = 2)
{
    int px = 6, py = 6;
    drawString(fb, px+1, py+1, str, scale, 0,   0,   0  ); // shadow
    drawString(fb, px,   py,   str, scale, 255, 220, 60  ); // yellow text
}

// ---------------------------------------------------------------------------
// FPS-style orbit camera controlled by keyboard
// ---------------------------------------------------------------------------
struct Camera {
    Vec3 eye   {0, 1.5f, 5.0f};
    float yaw   = 0.0f;   // radians, around Y
    float pitch = -0.2f;  // radians, up/down

    float speed     = 3.0f;  // units/sec
    float turnSpeed = 1.5f;  // radians/sec

    Vec3 forward() const {
        return Vec3(std::sin(yaw) * std::cos(pitch),
                    std::sin(pitch),
                    -std::cos(yaw) * std::cos(pitch)).normalized();
    }
    Vec3 right() const {
        return Vec3(std::cos(yaw), 0, std::sin(yaw)).normalized();
    }

    Mat4 viewMatrix() const {
        Vec3 f = forward();
        Vec3 r = right();
        Vec3 u = r.cross(f);
        return Mat4::lookAt(eye, eye + f, {0,1,0});
    }

    void update(const uint8_t* keys, float dt) {
        Vec3 f = forward();
        Vec3 r = right();

        // WASD move
        if (keys[SDL_SCANCODE_W]) eye += f * (speed * dt);
        if (keys[SDL_SCANCODE_S]) eye -= f * (speed * dt);
        if (keys[SDL_SCANCODE_A]) eye -= r * (speed * dt);
        if (keys[SDL_SCANCODE_D]) eye += r * (speed * dt);
        if (keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_SPACE])
            eye.y += speed * dt;
        if (keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_LCTRL])
            eye.y -= speed * dt;

        // Arrow keys rotate
        if (keys[SDL_SCANCODE_LEFT])  yaw   -= turnSpeed * dt;
        if (keys[SDL_SCANCODE_RIGHT]) yaw   += turnSpeed * dt;
        if (keys[SDL_SCANCODE_UP])    pitch += turnSpeed * dt;
        if (keys[SDL_SCANCODE_DOWN])  pitch -= turnSpeed * dt;

        // Clamp pitch
        const float MAX_PITCH = PI * 0.48f;
        if (pitch >  MAX_PITCH) pitch =  MAX_PITCH;
        if (pitch < -MAX_PITCH) pitch = -MAX_PITCH;
    }
};

int main(int argc, char** argv)
{
    const int W = 800, H = 600;

    // ---- SDL2 setup -------------------------------------------------------
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window*   window = SDL_CreateWindow("soft_render viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* sdlr   = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture*  tex    = SDL_CreateTexture(sdlr,
        SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, W, H);

    if (!window || !sdlr || !tex) {
        std::fprintf(stderr, "SDL setup failed: %s\n", SDL_GetError());
        return 1;
    }

    // ---- Renderer ---------------------------------------------------------
    Renderer renderer(W, H);
    renderer.setProjection(Mat4::perspective(PI / 3.0f, (float)W/H, 0.1f, 200.0f));

    renderer.addLight({ {3,  4,  3},  {1.0f, 0.95f, 0.8f}, 6.0f, 10.0f });
    renderer.addLight({ {-3, 2, -2}, {0.3f, 0.4f,  1.0f}, 2.0f,  8.0f });

    // ---- Mesh -------------------------------------------------------------
    demo::Mesh mesh;
    if (argc > 1) {
        if (!demo::loadOBJ(argv[1], mesh)) {
            std::fprintf(stderr, "Failed to load OBJ: %s\n", argv[1]);
            return 1;
        }
        std::fprintf(stderr, "Loaded %s: %zu verts, %zu indices\n",
            argv[1], mesh.vertices.size(), mesh.indices.size());
    } else {
        mesh = demo::makeSphere(64, 40);
    }

    Material mat;
    mat.albedo    = {0.8f, 0.4f, 0.15f};
    mat.ambient   = 0.04f;
    mat.diffuse   = 0.85f;
    mat.specular  = 0.6f;
    mat.shininess = 64.0f;

    DrawCall dc;
    dc.vertices    = mesh.vertices.data();
    dc.vertexCount = (int)mesh.vertices.size();
    dc.indices     = mesh.indices.data();
    dc.indexCount  = (int)mesh.indices.size();
    dc.material    = mat;

    // ---- Camera & timing --------------------------------------------------
    Camera cam;
    Mat4 model = Mat4::identity();

    auto lastTime = std::chrono::high_resolution_clock::now();
    double renderMs = 0.0;

    char hudBuf[64];
    bool running = true;

    // ---- Main loop --------------------------------------------------------
    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        // cap dt so a stall doesn't yeet the camera
        if (dt > 0.1f) dt = 0.1f;

        // -- Events
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        // -- Camera
        const uint8_t* keys = SDL_GetKeyboardState(nullptr);
        cam.update(keys, dt);

        renderer.setView(cam.viewMatrix());
        renderer.setCameraPos(cam.eye);
        renderer.setModel(model);

        // -- Render (timed)
        auto t0 = std::chrono::high_resolution_clock::now();
        renderer.beginFrame();
        renderer.draw(dc);
        auto t1 = std::chrono::high_resolution_clock::now();
        renderMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        // -- HUD overlay (drawn directly into framebuffer after render)
        std::snprintf(hudBuf, sizeof(hudBuf), "%.1f ms", renderMs);
        drawHUD(renderer.framebuffer(), hudBuf);

        // -- Blit framebuffer → SDL texture
        // SDL RGBA8888 is byte order R,G,B,A on big-endian — we use ABGR on
        // little-endian. Use SDL_PIXELFORMAT_ABGR8888 if colors look wrong.
        const core::Pixel* pixels = renderer.framebuffer().pixels();
        int pitch = W * 4;
        void* texPixels;
        SDL_LockTexture(tex, nullptr, &texPixels, &pitch);
        // Framebuffer stores {r,g,b,a} bytes; SDL RGBA8888 big-endian packs
        // the same byte layout — just memcpy.
        std::memcpy(texPixels, pixels, (size_t)W * H * 4);
        SDL_UnlockTexture(tex);

        SDL_RenderClear(sdlr);
        SDL_RenderCopy(sdlr, tex, nullptr, nullptr);
        SDL_RenderPresent(sdlr);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(sdlr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
