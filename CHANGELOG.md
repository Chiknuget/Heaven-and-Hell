# Changelog

All notable changes to Heaven & Hell are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the version starts with `0.`, the game is in early development and anything may change between releases.

## [0.2.0] - TechDemo v0.2 - 2026-11-13

The story begins. This release adds the prologue, the first mission, characters, a scripting language for scenes, and saving.

### Added

- **Prologue.** The traveler wakes in Heaven, sees what he shouldn't, and is judged by a voice from a blinding light. Choosing to serve begins the game; refusing leads to a short ending.
- **Name entry.** The player names the traveler before the game starts, and the name appears throughout the dialogue.
- **Mission 1, The Climb.** A scripted first death on the Surface, the guide's briefing in Ante-Purgatory, seven souls on the terraces to free through conversation, and a finale at the Earthly Paradise that leads into the next mission.
- **Missions.** A mission system with objectives, notices when missions start and finish, and a mission log on Tab.
- **Saving and loading.** Autosave on arrival, Save Game and Load Game in the menu, and protection against corrupted or mismatched save files.
- **Scenes.** A scripting language for cutscenes and story logic: dialogue, waits, screen fades, a growing light, free exploration, choices, labels and jumps, story flags and conditions, missions, captions, tile changes, banners, death and returning to the menu.
- **Characters.** NPCs placed from level files, who block movement, turn to face you when you talk, and can appear or disappear based on story flags, the first visit, or whether they greet you on arrival.
- **Conditional dialogue.** Dialogue files can be split into `[first]`, `[again]`, `[burdened]`, `[unburdened]` and `[default]` sections.
- **Charon**, the ferryman, waiting in the First Circle.
- **Sprites.** Text-based sprite files with palettes, see-through colors, walk animations, facing directions and automatic mirroring. The traveler, the guide, Charon and the souls of Purgatory have sprites.
- **Text placeholders.** `{name}` and `{flag}` in dialogue, scenes and objectives.
- **Level options:** `enter_scene` to run a scene on arrival, and `sprite=`, `if=`, `unless=`, `first_visit` and `greet` for characters.
- Choice boxes, centered captions and a blinking cursor for text entry.
- Typed text, Backspace and Tab support in the window library.
- A clear error message when a character has neither a scene nor a dialogue file.
- This changelog.

### Changed

- The game's code is split out of `main.cpp` into a `play` module (state, world, scenes, talk, missions, movement, text, save, render and ui), separate from the `game` module that loads data files.
- The guide is now a character with a scene, and gives the first mission.
- The stairs down from Heaven now appear only after the traveler agrees to serve.
- Dialogue speeches are split into pages when shown, so long player names never overflow the box.
- The main menu gains Save Game and Load Game, and shows short messages.
- The README covers the new controls, file formats, scene commands and project layout.

### Removed

- The `guide:` level key, replaced by `npc:` with the `first_visit` and `greet` options.
- `levels/guide.txt`, replaced by `scenes/guide.txt`.

## [0.1.0] - TechDemo v0.1 - 2026-11-11

The first public tech demo: the engine and core systems.

### Added

- The `pix` graphics library, built directly on Xlib: a 320×180 framebuffer with pixel-perfect scaling, alpha blending, isometric tile rendering, depth sorting and a hand-made bitmap font.
- Free eight-directional movement with collision, wall sliding and a jitter-free camera.
- A stacked world of floors connected by stairs: Heaven, the Surface, the Caves, the Deep Caves and the nine circles of hell.
- Deadly terrain, Purgatory's mountain of seven terraces, checkpoints and the Burden system.
- A guide who appears only on the first death.
- Dialogue boxes with typewriter text and word wrapping.
- Fade transitions and floor-name banners.
- A main menu with New Game, Continue and Quit.
- Levels and dialogue loaded from plain text files.
- A README with build instructions, controls and file formats.

[0.2.0]: https://github.com/Chiknuget/Heaven-and-Hell/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Chiknuget/Heaven-and-Hell/releases/tag/v0.1.0
