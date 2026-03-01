All the relevant information is here:

Tech stack:
Arch Linux (desktop + laptop via SSH) - Git + GitHub (SSH keys configured, repo working) - CMake + Ninja build system - GLFW + OpenGL 3.3 core - GLAD (generated locally and vendored in extern/glad) - GLM for math - VS Code (Code - OSS) with: CMake Tools clangd CodeLLDB compile_commands.json generated via CMake - Debugging working with LLDB

Main Goal:
Generate a spherical (~60km circumference, icospohere or cubesphere),(similar to minecraft, valheim, no man's sky) walkable (WASD) procedural 'planet' with realistic world like terrain thru Noise/Heightmap or other methods. Implement LOD's and Chunks if necessary. Optimised to run on old hardware, No voxels

Later Goals:
add trees, mountains, bodies of water, vegetation, other 3d low poly assets that can be added thru the generation algorithm. Day and Night cycle, weather, phisics. Consider future scalability: like chopping the trees and digging the ground (SDF engine) and ability to save the changes. Plan is to evolve it in to a game engine.

RoadMap:
0 - Cube-sphere + per-face quadtree LOD 
6 cube faces → project to sphere (“cube-sphere”).
Each face is a quadtree of patches; split/merge based on camera distance.
This is a very common approach for planetary terrain LOD and has lots of prior art + open repos to study.
Deliverable for this phase: a wireframe cube-sphere made of 6 patches (no noise), stable camera orbit.

1 - Make your rendering “engine-shaped” (minimal but scalable)
Before terrain explodes your complexity, lay a thin foundation:
A. Rendering core
Shader class (compile/link, hot reload optional)
Mesh class (VAO/VBO/IBO, dynamic updates)
Simple renderer (draw calls + state)
Basic material struct (shader + uniforms)
B. Debug UI (saves weeks)
Add Dear ImGui now for toggles and live numbers (FPS, patch counts, LOD levels, wireframe, etc.).
Deliverable: press a key to open a UI panel and toggle wireframe + show FPS.

2 - Planet patches: “fixed grid mesh + reuse indices”
Implement the correct world scale, world units 1 unit = 1 meter
Adjust the subdivision metrics if needed
Terrain LOD gets much easier if every patch is the same topology:
Patch resolution 65×65 verts.
Prebuild index buffers per LOD (or per stitch pattern if you go fancy).
Keep the vertex format lean: position + normal (later add biome weights / material ids).
Then build the pipeline:
Patch local grid (u,v)
Convert to cube face position
Project to sphere
Apply height displacement (later)
Deliverable: a planet made of a few patches, correct projection, no cracks, stable.

3 - Add LOD with crack handling (do the simple crack fix first)
LOD selection
Quadtree split threshold based on distance (or screen-space error).
Cap max depth (old hardware friendly).
Crack handling options (pick one):
Skirts (recommended first): extrude patch edges slightly downward to hide cracks. It’s the classic “works today” solution.
Stitching: more complex (multiple edge patterns).
Deliverable: fly from space to ground: patch count changes smoothly, no visible cracks.

4 - Height function: realistic-ish terrain without reinventing noise
Terrain that feels like a world.
Noise library
FastNoise2: modern C++/SIMD, node-graph style composition (great performance).
Terrain recipe (practical)
Base continents: low-frequency fbm
Mountains: ridged noise masked by “continentalness”
Detail: higher-frequency noise blended in by slope/height
Domain warp (sparingly) for less “procedural” look
Important on a sphere:
Sample noise in 3D (using normalized sphere position) to avoid seams and pole artifacts.
Deliverable: same planet seed always generates same terrain; change seed changes the world.

5 - Normals, lighting, and “looks good enough”
Compute normals from the displaced surface (cross products on the grid or analytic-ish derivative sampling).
Simple directional light + ambient.
Add fog (distance-based) to hide LOD transitions and improve atmosphere.
Deliverable: terrain reads well in motion; mountains/valleys look correct under light.

6 - Make it walkable (WASD on a sphere, proper ground alignment)
Implement “character controller lite” before full physics:
Maintain player state as:
up = normalize(player_position) (planet radial up)
camera yaw around up, pitch around camera right
Move on the tangent plane:
forward/right vectors are perpendicular to up
Ground height query:
Given latched position direction dir, height = radius + height(dir)
Set player position = dir * (height + eye_offset)
Optional: basic gravity = -up
Deliverable: you can walk around, the camera stays upright relative to the planet, no “falling off”.

7 - Performance pass (old hardware friendly)
This is where you win or lose.
Core tactics
Frustum culling per patch AABB/OBB (cheap, huge gains)
Keep patch vertex buffers static; only regenerate when needed
Background generation thread for patch meshes (double-buffer uploads)
Limit max visible patches (budget-based LOD)
Precision tactics (planet scale pain)
Large view distances + near-ground detail can cause z-fighting and jitter. Consider:
Reversed-Z for better depth precision at long range.
“Floating origin” later if you go truly huge, but at ~60 km circumference you might get away without it depending on your units and camera ranges.
Mesh optimization (later, but very useful)
If you start pushing lots of geometry, meshoptimizer is a great tool for cache/overdraw optimization.
Deliverable: stable frame time while moving; patch count and draw calls stay under a defined budget.

8 - “Chunk system” (what chunks mean in a non-voxel planet)
Your quadtree nodes are your chunks.
For each chunk store:
face id, quadtree coords, LOD level
mesh handles (GPU)
seed/biome params
min/max height bounds (for culling)
Then implement:
streaming (generate/unload around camera)
disk cache later (save chunks as generated)
Deliverable: memory doesn’t grow forever; planet streams smoothly.