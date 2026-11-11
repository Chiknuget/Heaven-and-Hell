# Heaven & Hell

**Version:** TechDemo v0.1

A pixel-art isometric RPG about a journey through heaven and hell, written in C++ for Linux with a graphics library built from scratch. No game engine, no SDL, no image libraries: everything from the window to the font is drawn by hand on top of X11.

This tech demo is the foundation of the game. It has a working world to explore, but the story, characters and art are still to come.

---

## Features

**Engine (`pix` library)**
- Window and input handled directly through Xlib
- Software framebuffer at a fixed 320×180 resolution, scaled up with crisp, pixel-perfect integer scaling and letterboxing
- Alpha blending for transparency, fades and overlays
- Isometric 2:1 tile rendering with raised blocks and shaded sides
- Depth sorting so characters can walk behind pillars, trees and walls
- Hand-made 5×7 bitmap font with upper and lowercase letters, digits and punctuation, at any whole-number scale

**Game**
- Smooth free movement in eight directions with jitter-free camera following
- Collision with wall sliding
- A world of stacked floors connected by stairs: Heaven, the Surface, the Caves, the Deep Caves, and the nine circles of hell
- Purgatory as its own mountain: Ante-Purgatory, seven terraces and the Earthly Paradise
- Death from deadly terrain (lava, the Styx, the river of blood)
- The Burden system: a quick way out of Purgatory that comes at a price
- A guide who appears only the first time you die
- Dialogue boxes with typewriter text and automatic word wrapping
- Fade transitions between floors and floor-name banners
- Main menu with New Game, Continue and Quit
- Levels and dialogue loaded from plain text files, so they can be edited without recompiling

---

## Requirements

- Debian 13 (or any Linux distribution with X11 or XWayland)
- GCC with C++20 support
- GNU Make
- Xlib development headers

On Debian:

```bash
sudo apt install build-essential libx11-dev
```

### If `sudo` isn't set up

If you gave the root account a password when installing Debian, the installer doesn't set up `sudo` for your user, and the command above fails with something like `sudo: command not found` or `your-name is not in the sudoers file`. You have two options.

**Install the packages as root.** Switch to the root account, install, then switch back:

```bash
su -
apt install build-essential libx11-dev
exit
```

`su -` asks for the **root** password, not your own.

**Or set up `sudo` for your user** so the usual `sudo` commands work from now on:

```bash
su -
apt install sudo
usermod -aG sudo your-username
exit
```

Replace `your-username` with your login name (run `whoami` before `su -` if you're not sure). Then **log out and log back in**, or reboot, because group changes only take effect on a new login. You can check it worked with:

```bash
sudo -v
```

If it asks for your password and returns without an error, `sudo` is ready.

Use `su -` with the dash, not plain `su`. The dash gives you root's full environment, including `/usr/sbin`, which is where `usermod` lives. Without it you may get `usermod: command not found`.

The game runs on Wayland desktops (such as Debian's default GNOME session) through XWayland, which is installed by default.

---

## Building and running

From the project root (the folder containing the `Makefile`):

```bash
make run
```

Other commands:

| Command      | What it does                         |
|--------------|--------------------------------------|
| `make`       | Builds the game into `build/rpg`     |
| `make run`   | Builds if needed, then runs the game |
| `make clean` | Deletes the `build` folder           |

The game must be run from the project root, because it loads `levels/world.txt` using a relative path.

---

## Controls

| Key                    | Action                                   |
|------------------------|------------------------------------------|
| W / A / S / D          | Move (arrow keys also work)              |
| Two keys together      | Move diagonally along the iso grid       |
| E                      | Use stairs and gates                     |
| E / Enter / Space      | Advance dialogue; select in menus        |
| Escape                 | Open the menu; press again to resume     |

---

## How the world works

### Floors and stairs

The world is a stack of floors, top to bottom:

1. Heaven (where a new game begins)
2. The Surface
3. The Caves
4. The Deep Caves
5. The nine circles of hell: Limbo, Lust, Gluttony, Greed, Wrath, Heresy, Violence, Fraud and Treachery

Stand on stairs and press E to move between floors. A `^` tile leads to the floor above and a `v` tile leads to the floor below. If a floor has several staircases, they pair up in reading order: the first `^` on a floor connects to the first `v` on the floor above, and so on.

Arriving on a floor sets your **checkpoint**.

### Death and Purgatory

Lava, the swamp of the Styx and the river of blood are deadly. Walk into one and you wake in **Ante-Purgatory**, at the foot of the mountain. From there you have two ways back:

- **Climb the mountain.** Seven short terraces, one for each deadly sin, lead up to the Earthly Paradise. Its gate returns you to your checkpoint for free, and reaching the summit lifts all of your Burden.
- **Take the toll gate.** It returns you to your checkpoint immediately, but adds 1 **Burden**.

While you carry any Burden, the stairs into Heaven will not let you ascend. The only way to be rid of it is to die and make the climb.

The first time you die, a guide is waiting in Ante-Purgatory to explain all this. They only appear once.

---

## Editing the world

All content lives in the `levels` folder. The files are read each time a new game starts, so you can edit a map, return to the menu, choose New Game and see your changes without rebuilding.

### `levels/world.txt`

Lists the level files that make up the world. Plain file names are the main floors, from top to bottom. Lines starting with `purgatory:` build Purgatory's own stack, also from top to bottom; the last one is where the dead arrive.

```
// Main floors, top to bottom
heaven.txt
surface.txt

// Purgatory, top to bottom
purgatory: earthly_paradise.txt
purgatory: ante_purgatory.txt
```

The `Floor` enum in `include/game/level.h` refers to main floors by their position, so keep its order in step with this file.

### Level files

A level file has a header of `key: value` lines, then `map:`, then the map itself.

```
name: The Fifth Circle
subtitle: Wrath
sky_top: 40 60 40
sky_bottom: 10 18 10
spawn: 2 1
map:
OOOOOOOO
O^mxxxxx
Oxmmxxxx
```

| Key          | Required | Meaning                                                        |
|--------------|----------|----------------------------------------------------------------|
| `name`       | yes      | Shown in large text on the banner when you arrive              |
| `subtitle`   | no       | Smaller second line on the banner                              |
| `sky_top`    | no       | Background color at the top of the screen, as `R G B`          |
| `sky_bottom` | no       | Background color at the bottom of the screen                   |
| `spawn`      | no       | Tile where you appear when not arriving by stairs, as `column row` |
| `guide`      | no       | Tile where the guide stands (used in Ante-Purgatory)           |
| `map`        | yes      | Everything after this line is the map                          |

Map notes:
- Each character is one tile. Columns and rows are counted from 0.
- A space is empty void, so leading spaces matter. Trailing spaces do not.
- Use spaces only, never tabs.
- Back walls are usually placed only along the top row and left column, since walls along the front edges would hide the player.
- Lines starting with `//` in the header are comments. There are no comments inside the map.

If something is wrong in a file, the game stops with a message naming the file and line, for example `levels/caves.txt:3: unknown key 'sky_tpo'`.

### Tiles

| Char | Tile              | Walkable | Notes                         |
|------|-------------------|----------|-------------------------------|
| ` `  | Void              | no       | Nothing is drawn              |
| `.`  | Cloud             | yes      | Heaven                        |
| `g`  | Gold path         | yes      | Heaven                        |
| `P`  | Marble pillar     | no       | Tall                          |
| `,`  | Grass             | yes      | Surface                       |
| `d`  | Dirt path         | yes      | Surface                       |
| `T`  | Tree              | no       | Tall                          |
| `w`  | Water             | no       | Low                           |
| `:`  | Cave floor        | yes      | Caves                         |
| `#`  | Cave wall         | no       | Tall                          |
| `;`  | Dark stone        | yes      | Deep Caves                    |
| `%`  | Basalt wall       | no       | Tall                          |
| `*`  | Crystal           | no       | Tall                          |
| `r`  | Hellrock          | yes      | Hell                          |
| `O`  | Obsidian          | no       | Tall                          |
| `~`  | Lava              | yes      | **Deadly**                    |
| `a`  | Ash               | yes      | Limbo                         |
| `k`  | Storm stone       | yes      | Lust                          |
| `m`  | Mud               | yes      | Gluttony, Wrath, Sloth        |
| `C`  | Coin heap         | no       | Greed                         |
| `x`  | Styx swamp        | yes      | **Deadly**                    |
| `B`  | Burning tomb      | no       | Heresy                        |
| `b`  | River of blood    | yes      | **Deadly**                    |
| `f`  | Ditch stone       | yes      | Fraud, Wrath                  |
| `i`  | Ice               | yes      | Treachery                     |
| `p`  | Pale stone        | yes      | Purgatory                     |
| `F`  | Purifying fire    | yes      | Safe to walk through          |
| `^`  | Stairs up         | yes      | Press E to ascend             |
| `v`  | Stairs down       | yes      | Press E to descend            |
| `G`  | Gate              | yes      | Free return to checkpoint     |
| `$`  | Toll gate         | yes      | Return to checkpoint, +1 Burden |

An unknown character is drawn as a bright magenta block so typos are easy to spot. Deadly tiles only kill you when the center of your body is over them, so brushing past an edge is safe.

New tile types are added in `tileType()` in `src/game/level.cpp`.

### Dialogue files

`levels/guide.txt` holds the guide's speech. A `name:` line sets the speaker shown on the dialogue box, and every other line is one speech. Long speeches are split into pages automatically.

```
name: The Guide
Peace, traveler. Do not be afraid.
```

Only use characters the font contains: letters, digits, spaces and `. , ! ? ' " : - + / ( ) &`. Anything else is drawn as a hollow box.

---

## Project layout

```
.
├── Makefile
├── README.md
├── include/
│   ├── gfx/                 The pix graphics library
│   │   ├── font.h           Bitmap font and text drawing
│   │   ├── framebuffer.h    Pixel buffer, colors and blending
│   │   ├── iso.h            Isometric tiles and blocks
│   │   └── window.h         X11 window and keyboard input
│   └── game/
│       ├── dialogue.h       Dialogue files and word wrapping
│       └── level.h          Tiles, levels and world loading
├── src/
│   ├── main.cpp             Game loop, menu, player, transitions, Purgatory
│   ├── gfx/                 Implementations of the pix library
│   └── game/                Implementations of the game module
└── levels/                  World, level and dialogue files
```

The code is split into two namespaces. `pix` is the engine and knows nothing about the game. `game` holds the data the game is built from. `main.cpp` brings them together.

---

## Known limitations

- There is no saving yet. Continue only resumes a game started in the current session.
- The player and guide are drawn as simple colored shapes until sprites are added.
- Linux and X11 only.
- The game loop assumes 60 frames per second.
- Maps are small, and the Ninth Circle is currently the bottom of the world.
- The `levels` folder must be next to where the game is run from.

---

## Roadmap

- Sprites: an image loader, transparency, and walk animations with facing directions
- NPCs you can talk to
- Worldbuilding and the main story
- Saving and loading
- Items, combat and progression
- Sound and music
