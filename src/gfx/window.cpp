#include "gfx/window.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <iterator>

namespace pix
{

	namespace
	{

		int keyIndex(KeySym sym)
		{
			switch (sym)
			{
			case XK_w:
				return static_cast<int>(Key::W);
			case XK_a:
				return static_cast<int>(Key::A);
			case XK_s:
				return static_cast<int>(Key::S);
			case XK_d:
				return static_cast<int>(Key::D);
			case XK_e:
				return static_cast<int>(Key::E);
			case XK_Up:
				return static_cast<int>(Key::Up);
			case XK_Down:
				return static_cast<int>(Key::Down);
			case XK_Left:
				return static_cast<int>(Key::Left);
			case XK_Right:
				return static_cast<int>(Key::Right);
			case XK_space:
				return static_cast<int>(Key::Space);
			case XK_Return:
				return static_cast<int>(Key::Enter);
			case XK_Escape:
				return static_cast<int>(Key::Escape);
			case XK_BackSpace:
				return static_cast<int>(Key::Backspace);
			case XK_Tab:
				return static_cast<int>(Key::Tab);
			default:
				return -1;
			}
		}

	} // namespace

	struct Window::Impl
	{
		Display *display = nullptr;
		::Window handle = 0; // ::Window is the X11 type, not ours
		GC gc = nullptr;
		Atom wmDeleteWindow = 0;
		XImage *image = nullptr;
		std::vector<std::uint32_t> backbuffer; // window-sized, already scaled
		int width = 0;
		int height = 0;
		bool keys[static_cast<int>(Key::Count)] = {};
		bool pressed[static_cast<int>(Key::Count)] = {};
		std::string typed; // text typed since the last pollEvents

		void createImage(int w, int h)
		{
			destroyImage();
			width = w;
			height = h;
			backbuffer.assign(static_cast<size_t>(w) * h, 0);

			int screen = DefaultScreen(display);
			image = XCreateImage(display, DefaultVisual(display, screen),
													 DefaultDepth(display, screen), ZPixmap, 0,
													 reinterpret_cast<char *>(backbuffer.data()),
													 w, h, 32, 0);
			if (!image)
				throw std::runtime_error("XCreateImage failed");
		}

		void destroyImage()
		{
			if (image)
			{
				image->data = nullptr; // the vector owns the memory, not X11
				XDestroyImage(image);
				image = nullptr;
			}
		}
	};

	Window::Window(const std::string &title, int width, int height)
			: m_impl(std::make_unique<Impl>())
	{
		Impl &w = *m_impl;

		w.display = XOpenDisplay(nullptr);
		if (!w.display)
			throw std::runtime_error("Could not connect to the X server");

		int screen = DefaultScreen(w.display);
		if (DefaultDepth(w.display, screen) < 24)
			throw std::runtime_error("A 24-bit or 32-bit color display is required");

		w.handle = XCreateSimpleWindow(w.display, RootWindow(w.display, screen),
																	 0, 0, width, height, 0,
																	 BlackPixel(w.display, screen),
																	 BlackPixel(w.display, screen));

		XStoreName(w.display, w.handle, title.c_str());

		XSelectInput(w.display, w.handle,
								 ExposureMask | KeyPressMask | KeyReleaseMask |
										 StructureNotifyMask | FocusChangeMask);

		// Holding a key normally makes X11 send fake release/press pairs.
		// This turns that off so a held key stays "down".
		XkbSetDetectableAutoRepeat(w.display, True, nullptr);

		// Ask the window manager to tell us when the X button is clicked
		w.wmDeleteWindow = XInternAtom(w.display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(w.display, w.handle, &w.wmDeleteWindow, 1);

		w.gc = XCreateGC(w.display, w.handle, 0, nullptr);
		XMapWindow(w.display, w.handle);

		w.createImage(width, height);
	}

	Window::~Window()
	{
		Impl &w = *m_impl;
		w.destroyImage();
		if (w.gc)
			XFreeGC(w.display, w.gc);
		if (w.handle)
			XDestroyWindow(w.display, w.handle);
		if (w.display)
			XCloseDisplay(w.display);
	}

	void Window::pollEvents()
	{
		Impl &w = *m_impl;
		std::fill(std::begin(w.pressed), std::end(w.pressed), false);
		w.typed.clear();

		while (XPending(w.display))
		{
			XEvent e;
			XNextEvent(w.display, &e);

			switch (e.type)
			{
			case ClientMessage:
				if (static_cast<Atom>(e.xclient.data.l[0]) == w.wmDeleteWindow)
					m_open = false;
				break;

			case ConfigureNotify:
				if (e.xconfigure.width != w.width || e.xconfigure.height != w.height)
					w.createImage(e.xconfigure.width, e.xconfigure.height);
				break;

			case KeyPress:
			case KeyRelease:
			{
				int k = keyIndex(XLookupKeysym(&e.xkey, 0));
				if (k >= 0)
				{
					bool down = (e.type == KeyPress);
					// Held keys repeat KeyPress events, so only count it as
					// "pressed" if the key wasn't already down
					if (down && !w.keys[k])
						w.pressed[k] = true;
					w.keys[k] = down;
				}

				// The actual characters typed, for text boxes like name entry.
				// Unlike the keys above, held keys repeat here, like in any text box.
				if (e.type == KeyPress)
				{
					char buf[16];
					KeySym sym;
					int n = XLookupString(&e.xkey, buf, sizeof(buf), &sym, nullptr);
					for (int i = 0; i < n; ++i)
					{
						if (buf[i] >= 32 && buf[i] < 127)
							w.typed += buf[i];
					}
				}
				break;
			}

			case FocusOut:
				// Forget held keys so nothing gets stuck when you click away
				std::fill(std::begin(w.keys), std::end(w.keys), false);
				break;
			}
		}
	}

	void Window::present(const Framebuffer &fb)
	{
		Impl &w = *m_impl;
		if (!w.image)
			return;

		// Biggest whole-number scale that fits, so pixels stay perfectly square
		int scale = std::max(1, std::min(w.width / fb.width(), w.height / fb.height()));
		int outW = fb.width() * scale;
		int outH = fb.height() * scale;
		int offX = (w.width - outW) / 2; // center it with black bars
		int offY = (w.height - outH) / 2;

		std::fill(w.backbuffer.begin(), w.backbuffer.end(), 0);

		const Color *src = fb.data();
		for (int y = 0; y < outH; ++y)
		{
			int dy = offY + y;
			if (dy < 0 || dy >= w.height)
				continue;

			const Color *srcRow = src + (y / scale) * fb.width();
			std::uint32_t *dstRow = w.backbuffer.data() + dy * w.width;

			for (int x = 0; x < outW; ++x)
			{
				int dx = offX + x;
				if (dx < 0 || dx >= w.width)
					continue;
				dstRow[dx] = srcRow[x / scale];
			}
		}

		XPutImage(w.display, w.handle, w.gc, w.image, 0, 0, 0, 0, w.width, w.height);
		XFlush(w.display);
	}

	bool Window::isKeyDown(Key key) const
	{
		return m_impl->keys[static_cast<int>(key)];
	}

	bool Window::wasKeyPressed(Key key) const
	{
		return m_impl->pressed[static_cast<int>(key)];
	}

	const std::string &Window::typedText() const
	{
		return m_impl->typed;
	}

} // namespace pix
