# Heaven & Hell

**Version:** TechDemo v0.2

A pixel-art isometric RPG about a journey through heaven and hell, written in C++ for Linux with a graphics library built from scratch. No game engine, no SDL, no image libraries: everything from the window to the font is drawn by hand on top of X11.

A living man wanders into Heaven by accident and sees something no mortal should. Offered a choice between Hell and serving the Voice that judges him, he sets out on a journey down through the world. This tech demo contains the prologue and the first mission.

See [CHANGELOG.md](CHANGELOG.md) for what's new in each version.

---

## Features

**Engine (`pix` library)**
- Window, keyboard and text input handled directly through Xlib
- Software framebuffer at a fixed 320×180 resolution, scaled up with crisp, pixel-perfect integer scaling and letterboxing
- Alpha blending for transparency, fades, light and overlays
- Isometric 2:1 tile rendering with raised blocks and shaded sides
- Depth sorting so characters can walk behind pillars, trees and walls
- Hand-made 5×7 bitmap font with upper and lowercase letters, digits and punctuation, at any whole-number scale
- Text-based sprites with palettes, transparency, walk animations and automatic mirroring

**Game**
- Smooth free movement in eight directions with jitter-free camera following
- Collision with wall sliding, including solid characters
- A world of stacked floors connected by stairs: Heaven, the Surface, the Caves, the Deep Caves, and the nine circles of hell
- Purgatory as its own mountain: Ante-Purgatory, seven terraces and the Earthly Paradise
- Death from deadly terrain, and the Burden system for leaving Purgatory early
- A named, player-chosen traveler, with the name used throughout the dialogue
- A scripted prologue with a real choice and its own ending
- The first mission, The Climb: seven souls to free through conversation
- Characters placed from level files, who face you when you talk and remember whether you've met
- A scene scripting language for cutscenes, choices, story flags and missions
- Mission log, mission notices, floor banners and fade transitions
- Saving and loading, with autosave on arrival
- Levels, sprites, dialogue and scenes all loaded from plain text files, so they can be edited without recompiling

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

The game must be run from the project root, because it loads its `levels`, `sprites`, `dialogue` and `scenes` folders using relative paths, and writes saves to `saves`.

---

## Controls

| Key                    | Action                                         |
|------------------------|------------------------------------------------|
| W / A / S / D          | Move (arrow keys also work)                    |
| Two keys together      | Move diagonally along the iso grid             |
| E                      | Talk, and use stairs and gates                 |
| E / Enter / Space      | Advance dialogue; confirm choices and menus    |
| W / S                  | Move between options in choices and menus      |
| Tab                    | Open or close the mission log                  |
| Escape                 | Open the menu; press again to resume           |

On the name screen, type a name (letters, numbers, spaces, hyphens and apostrophes, up to 12 characters), use Backspace to correct it and Enter to begin.

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

### Death, Purgatory and Burden

Lava, the swamp of the Styx and the river of blood are deadly. When the traveler dies, he wakes in **Ante-Purgatory**, at the foot of the mountain. From there, there are two ways back:

- **Climb the mountain.** Seven short terraces, one for each deadly sin, lead up to the Earthly Paradise. Its gate returns you to your checkpoint for free, and reaching the summit lifts all of your Burden.
- **Take the toll gate.** It returns you to your checkpoint immediately, but adds 1 **Burden**.

While you carry any Burden, the stairs into Heaven will not let you ascend.

### Missions

Missions are given by the story. When one starts or ends, a notice appears in the top-left corner, and **Tab** opens the log with every mission and its current objective. This version contains **The Climb**, the first mission, which leads into the second.

### Saving

The game saves automatically each time you arrive somewhere new, once any scene or conversation there has finished. You can also choose **Save Game** from the menu at any time outside a scene. **Load Game** appears on the menu whenever a save exists.

There is one save slot, stored in `saves/save.txt`. Starting a New Game replaces it at the first autosave, right after the prologue.

---

## Editing the world

All content lives in plain text files. They are read each time a game starts or loads, so you can edit them, return to the menu, choose New Game or Load Game, and see your changes without rebuilding. If something is wrong in a file, the game stops with a message naming the file and line, for example `levels/caves.txt:3: unknown key 'sky_tpo'`.

```
levels/      World file and one file per level
sprites/     Character art
dialogue/    Simple conversations
scenes/      Cutscenes, story logic and conversations with choices
```

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

The `Floor` enum in `include/game/level.h` refers to main floors by their position, so keep its order in step with this file. Saves also remember floors by position: adding or removing levels makes old saves refuse to load, and reordering them is best followed by a new game.

### Level files

A level file has a header of `key: value` lines, then `map:`, then the map itself.

```
name: The Fifth Circle
subtitle: Wrath
sky_top: 40 60 40
sky_bottom: 10 18 10
spawn: 2 1
npc: 3 1 charon
map:
OOOOOOOO
O^mxxxxx
Oxmmxxxx
```

| Key           | Meaning                                                                 |
|---------------|-------------------------------------------------------------------------|
| `name`        | Shown in large text on the banner when you arrive (required)            |
| `subtitle`    | Smaller second line on the banner                                       |
| `sky_top`     | Background color at the top of the screen, as `R G B`                   |
| `sky_bottom`  | Background color at the bottom of the screen                            |
| `spawn`       | Tile where you appear when not arriving by stairs, as `column row`      |
| `enter_scene` | Scene to run on arrival; add `once` to run it on the first visit only   |
| `npc`         | A character, as `column row name` plus options (below); repeatable      |
| `map`         | Everything after this line is the map (required)                        |

Options for `npc`:

| Option        | Meaning                                                    |
|---------------|------------------------------------------------------------|
| `sprite=name` | Use `sprites/name.txt` instead of the character's own name |
| `if=flag`     | Only present while the flag is set                         |
| `unless=flag` | Only present while the flag is not set                     |
| `first_visit` | Only present the first time the level is entered           |
| `greet`       | Starts talking as soon as you arrive                       |

A character runs `scenes/name.txt` when spoken to if that file exists, and otherwise speaks from `dialogue/name.txt`.

Map notes:
- Each character is one tile. Columns and rows are counted from 0.
- A space is empty void, so leading spaces matter. Trailing spaces do not.
- Use spaces only, never tabs.
- Back walls are usually placed only along the top row and left column, since walls along the front edges would hide the player.
- Lines starting with `//` in the header are comments. There are no comments inside the map.

### Tiles

| Char | Tile              | Walkable | Notes                           |
|------|-------------------|----------|---------------------------------|
| ` `  | Void              | no       | Nothing is drawn                |
| `.`  | Cloud             | yes      | Heaven                          |
| `g`  | Gold path         | yes      | Heaven                          |
| `P`  | Marble pillar     | no       | Tall                            |
| `,`  | Grass             | yes      | Surface                         |
| `d`  | Dirt path         | yes      | Surface                         |
| `T`  | Tree              | no       | Tall                            |
| `w`  | Water             | no       | Low                             |
| `:`  | Cave floor        | yes      | Caves                           |
| `#`  | Cave wall         | no       | Tall                            |
| `;`  | Dark stone        | yes      | Deep Caves                      |
| `%`  | Basalt wall       | no       | Tall                            |
| `*`  | Crystal           | no       | Tall                            |
| `r`  | Hellrock          | yes      | Hell                            |
| `O`  | Obsidian          | no       | Tall                            |
| `~`  | Lava              | yes      | **Deadly**                      |
| `a`  | Ash               | yes      | Limbo                           |
| `k`  | Storm stone       | yes      | Lust                            |
| `m`  | Mud               | yes      | Gluttony, Wrath, Sloth          |
| `C`  | Coin heap         | no       | Greed                           |
| `x`  | Styx swamp        | yes      | **Deadly**                      |
| `B`  | Burning tomb      | no       | Heresy                          |
| `b`  | River of blood    | yes      | **Deadly**                      |
| `f`  | Ditch stone       | yes      | Fraud, Wrath                    |
| `i`  | Ice               | yes      | Treachery                       |
| `p`  | Pale stone        | yes      | Purgatory                       |
| `F`  | Purifying fire    | yes      | Safe to walk through            |
| `^`  | Stairs up         | yes      | Press E to ascend               |
| `v`  | Stairs down       | yes      | Press E to descend              |
| `G`  | Gate              | yes      | Free return to checkpoint       |
| `$`  | Toll gate         | yes      | Return to checkpoint, +1 Burden |

An unknown character is drawn as a bright magenta block so typos are easy to spot. Deadly tiles only kill you when the center of your body is over them, so brushing past an edge is safe.

New tile types are added in `tileType()` in `src/game/level.cpp`.

### Sprites

A sprite file has a size, an origin (the pixel that stands on the ground, between the feet), a palette, and one or more animations made of frames.

```
size: 8 14
origin: 4 13
palette:
b 60 120 200
y 255 220 100 200
anim: down
..bbbb..
...
```

- Each palette line is a character and an `R G B` color, with an optional fourth number for alpha (0 to 255) for see-through pixels. `.` is always transparent.
- Animations are named `down`, `up`, `left` and `right`. If `left` is missing, `right` is mirrored automatically, and the other way around. A sprite with only `down` always faces forward.
- Each frame is exactly `size` rows tall. The first frame is the standing pose; the rest play in a loop while walking.

The traveler is `sprites/traveler.txt`.

### Dialogue files

For characters with simple conversations. A `name:` line sets the speaker, and every other line is one speech; long speeches are split into pages automatically. Sections in square brackets choose what the character says, and the first section whose condition is true is used:

| Condition      | True when                                       |
|----------------|-------------------------------------------------|
| `[first]`      | You've never spoken to this character           |
| `[again]`      | You've spoken to them before                    |
| `[burdened]`   | You carry any Burden                            |
| `[unburdened]` | You carry no Burden                             |
| `[default]`    | Always (also used for lines before any section) |

```
name: Charon
[first]
Another soul, wandering where the living do not belong.
[again]
Still walking? Most stop eventually.
```

### Scenes

Scenes are scripts that run one command per line: cutscenes, conversations with choices, and the story's logic. They're used for the prologue, for characters with a scene file, and for levels with `enter_scene`.

| Command                                | What it does                                                  |
|----------------------------------------|---------------------------------------------------------------|
| `say Speaker: text`                    | A line of dialogue                                            |
| `wait frames`                          | Pause (60 frames = 1 second)                                  |
| `fade r g b alpha frames`              | Fade a color over the screen; 255 covers it, 0 clears it      |
| `glow radius frames`                   | Grow or shrink a blinding light (0 = gone)                    |
| `explore x y`                          | Let the player walk until they reach that tile                |
| `choice Text = label \| Text = label`  | Show a choice; each option jumps to a label                   |
| `label name` / `goto name`             | Mark a place in the script / jump to it                       |
| `set flag number` / `add flag number`  | Set a story flag, or add to it (flags start at 0)             |
| `if flag goto label`                   | Jump if the flag isn't 0                                      |
| `if flag >= number goto label`         | Jump if the comparison is true (`==` `!=` `<` `<=` `>` `>=`)  |
| `mission id Title`                     | Start a mission                                               |
| `objective id text`                    | Change a mission's objective                                  |
| `complete id`                          | Complete a mission                                            |
| `caption text`                         | Centered text on its own; waits for E                         |
| `settile x y tile`                     | Change a tile on the current level (kept in saves)            |
| `banner`                               | Show the current level's banner                               |
| `die`                                  | The traveler dies and wakes in Purgatory                      |
| `menu`                                 | End the game and return to the main menu                      |
| `end`                                  | End the scene; normal play continues                          |

Lines starting with `//` are comments.

### Text in dialogue and scenes

In any dialogue, scene text, speaker name or objective, `{name}` becomes the player's name and `{flag}` becomes that flag's value, so an objective like `Freed: {freed}/7` stays up to date by itself.

Only use characters the font contains: letters, digits, spaces and `. , ! ? ' " : - + / ( ) &`. Anything else is drawn as a hollow box.

---

## Project layout

```
.
├── Makefile
├── README.md
├── CHANGELOG.md
├── include/
│   ├── gfx/            The pix engine library
│   │   ├── font.h          Bitmap font and text drawing
│   │   ├── framebuffer.h   Pixel buffer, colors and blending
│   │   ├── iso.h           Isometric tiles and blocks
│   │   ├── sprite.h        Text sprites and animation frames
│   │   └── window.h        X11 window, keys and typed text
│   ├── game/           Loading the game's data files
│   │   ├── dialogue.h      Dialogue files, word wrapping and pages
│   │   ├── level.h         Tiles, levels, characters and the world
│   │   └── scene.h         Scene scripts
│   └── play/           Running the game
│       ├── state.h         The game's state and tuning constants
│       ├── world.h         Arrival, stairs, transitions and the frame update
│       ├── scenes.h        Running scene scripts
│       ├── talk.h          Dialogue boxes and characters
│       ├── missions.h      Missions, banners and notices
│       ├── movement.h      Movement and collision
│       ├── text.h          Story flags and filling in text
│       ├── save.h          Saving and loading
│       ├── render.h        Drawing the world
│       └── ui.h            Drawing boxes, text and effects
├── src/                Implementations, in the same folders
│   └── main.cpp        Main menu, name entry and the main loop
├── levels/             World and level files
├── sprites/            Sprite files
├── dialogue/           Dialogue files
├── scenes/             Scene files
└── saves/              Save files (created when the game first saves)
```

The code is split into three layers. `pix` is the engine and knows nothing about the game. `game` reads the data files the game is built from. `play` runs the game itself, and `main.cpp` ties it together with the menus.

---

## Known limitations

- One save slot.
- Saves remember floors by their position in `world.txt`.
- Saving isn't possible in the middle of a scene.
- The first mission's story continues into a second mission that isn't in this version yet.
- There is no combat yet.
- Tiles are drawn as flat-colored blocks.
- Linux and X11 only.
- The game loop assumes 60 frames per second.
- The game must be run from the project root.

---

## Roadmap

- Mission 2: the Tormentor's village
- Combat
- Mission 3 and the darker second half of the story
- Textured tiles and more character art
- Multiple save slots
- Sound and music
