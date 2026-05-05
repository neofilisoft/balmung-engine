/**
 * Balmung — Window.cpp
 * GLFW + GLAD (OpenGL 4.1 baseline, 4.6-capable drivers supported)
 * Build deps: glfw3, glad (generate from https://glad.dav1d.de or use vcpkg)
 */

// Detect if GLAD/GLFW available; compile as stub if not
#if __has_include(<glad/glad.h>)
#  include <glad/glad.h>
#  include <GLFW/glfw3.h>
#  define BM_HAS_GL 1
#else
#  define BM_HAS_GL 0
#endif

#include "renderer/window.hpp"
#include <iostream>
#include <stdexcept>

namespace Balmung {

#if BM_HAS_GL

void Window::_glfw_error_cb(int code, const char* msg) {
    std::cerr << "[GLFW] Error " << code << ": " << msg << "\n";
}

Window::Window(const WindowConfig& cfg)
    : _width(cfg.width), _height(cfg.height)
{
    glfwSetErrorCallback(_glfw_error_cb);
    if (!glfwInit())
        throw std::runtime_error("[Window] GLFW init failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, cfg.gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, cfg.gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE,  GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_SAMPLES,         cfg.msaa_samples);

    GLFWmonitor* mon = cfg.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    _window = glfwCreateWindow(cfg.width, cfg.height, cfg.title.c_str(), mon, nullptr);
    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("[Window] GLFW window creation failed");
    }
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(cfg.vsync ? 1 : 0);
    glfwSetWindowUserPointer(_window, this);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        throw std::runtime_error("[Window] GLAD failed to load OpenGL");
    }

    glfwSetKeyCallback       (_window, _glfw_key_cb);
    glfwSetMouseButtonCallback(_window, _glfw_mouse_btn_cb);
    glfwSetCursorPosCallback (_window, _glfw_cursor_cb);
    glfwSetScrollCallback    (_window, _glfw_scroll_cb);
    glfwSetFramebufferSizeCallback(_window, _glfw_resize_cb);

    std::cout << "[Window] OpenGL " << glGetString(GL_VERSION)
              << " | " << glGetString(GL_RENDERER) << "\n";
}

Window::~Window() {
    if (_window) glfwDestroyWindow(_window);
    glfwTerminate();
}

bool Window::should_close() const { return glfwWindowShouldClose(_window); }
void Window::poll_events()       { _mouse_dx = 0; _mouse_dy = 0; _scroll_y = 0; glfwPollEvents(); }
void Window::swap_buffers()      { glfwSwapBuffers(_window); }

bool Window::key_pressed (int k) const { return glfwGetKey        (_window, k) == GLFW_PRESS; }
bool Window::key_down    (int k) const { return glfwGetKey        (_window, k) != GLFW_RELEASE; }
bool Window::mouse_button_pressed(int b) const { return glfwGetMouseButton(_window, b) == GLFW_PRESS; }

void Window::set_cursor_locked(bool locked) {
    _cursor_locked = locked;
    glfwSetInputMode(_window, GLFW_CURSOR,
        locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

// ── Static callbacks ──────────────────────────────────────────────────────────

void Window::_glfw_key_cb(GLFWwindow* w, int key, int, int action, int) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self->on_key) self->on_key(key, action);
}

void Window::_glfw_mouse_btn_cb(GLFWwindow* w, int btn, int action, int) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self->on_mouse_button) self->on_mouse_button(btn, action);
}

void Window::_glfw_cursor_cb(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    static double last_x = x, last_y = y;
    self->_mouse_dx = (float)(x - last_x);
    self->_mouse_dy = (float)(y - last_y);
    last_x = x; last_y = y;
    self->_mouse_x = (float)x;
    self->_mouse_y = (float)y;
    if (self->on_mouse_move) self->on_mouse_move(x, y);
}

void Window::_glfw_scroll_cb(GLFWwindow* w, double, double yo) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    self->_scroll_y = (float)yo;
    if (self->on_scroll) self->on_scroll(0, yo);
}

void Window::_glfw_resize_cb(GLFWwindow* w, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    self->_width = width; self->_height = height;
    glViewport(0, 0, width, height);
    if (self->on_resize) self->on_resize(width, height);
}

#else // BM_HAS_GL = 0  ─── headless stub ─────────────────────────────────────

Window::Window(const WindowConfig& cfg) : _width(cfg.width), _height(cfg.height) {
    std::cout << "[Window] GLFW/GLAD not available — headless mode\n";
}
Window::~Window() {}
bool Window::should_close() const { return true; }
void Window::poll_events()        {}
void Window::swap_buffers()       {}
bool Window::key_pressed(int)     const { return false; }
bool Window::key_down(int)        const { return false; }
bool Window::mouse_button_pressed(int) const { return false; }
void Window::set_cursor_locked(bool)   {}

#endif

} // namespace Balmung


