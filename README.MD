# Gek's Simple Game Engine
## Multi-platform multi-threaded game engine with a basic Phong renderer
### Notes on Limitations/outdatedness

The renderer can be replaced with a more complex one. Simple edits to the shader source code can 
reinterpret phong material float values as PBR metallic workflow material props. Phong is used
mostly because it is simple and... nostalgic. I like it.


The animation system I wrote is a hack to avoid writing or using importer libraries for standardized formats.
Nothing in the code prevents one from replacing the animation system with one that supports per-vertex bone animation.
In fact, I attempted to hack-in a version of it, but encountered problems along the way.

It is planned to transition to SDL2 at some point to make the engine more portable and less dependent on the PC platform.

It is also planned to phase out the pthreads implementation for one which uses SDL threads, or OpenMP threads, for ease of
transition between compiling environments.

Realistically, pthreads and glfw3 are available for all targets (PC, mac, linux) but I recognize them as weaknesses of the
engine when compared to others.

### Dependencies:
  - GLFW3
  - OpenAL
  - GLM
  - OpenGL 3.3
  - Bullet Physics
## Confirmed compiling on:
### MSYS2 with MinGW-w64 (Windows)
  - Currently Msys2 comes with bugged GL headers by default. You will need to replace them before you can compile the library.
### Debian GNU/Linux x86_64
  - Primary development platform.

## FEATURES
  - support for Wavefront OBJ models, including custom extensions for per-vertex color. (No MTL)
  - Phong lighting and shading with shadowmaps.
  - Collision detection
  - Entity component system
  - Multithreading
  - Full game-state serialization
  - Can serialize runtime changes to file by modifying GSGE file in-memory during runtime, then embedding in the save file (see demo rabbit)
  - CPU rendering (There is a demonstration raytracer for that)
  - Joystick support
  - Directional audio with OpenAL
  - dozens of hooks (virtual functions, function pointers, etcetera) to implement to insert your own functionality
  - Many other features i'm forgetting