#pragma once
#include "framebuffer.h"
#include <memory>
#include <string>

namespace pix
{

  enum class Key
  {
    W,
    A,
    S,
    D,
    E,
    Up,
    Down,
    Left,
    Right,
    Space,
    Enter,
    Escape,
    Count // not a real key: the number of keys above
  };

  class Window
  {
  public:
    Window(const std::string &title, int width, int height);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool isOpen() const { return m_open; }
    void pollEvents();
    void present(const Framebuffer &fb);

    bool isKeyDown(Key key) const;
    // True only on the frame the key went down, not while it's held
    bool wasKeyPressed(Key key) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_open = true;
  };

} // namespace pix
