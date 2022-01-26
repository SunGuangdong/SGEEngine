

__Work-in-Progress The official "early alpha" release should be available anytime soon. I want to have good samples and documentation before releasing it.__

# SGEEngine

__SGEEngine__ is an open source __(MIT License)__, C++ centric game engine with an editor usable as a sandbox. Aimed at simple projects, SGEEngine is suitable for small games, game jams, learning, personal projects and can be used as a basis for your own game engine.


I've build many demos and toys with it.
The latest one can be tried here.

<iframe src="https://itch.io/embed/1232486" width="552" height="167" frameborder="0"><a href="https://ongamex.itch.io/broomin-around">Broomin' Around by ongamex, ChrisiaDraws</a></iframe>

<img src="./docs/img/editor_ss4.jpg" alt="alt text" width="100%">


<img src="./docs/img/editor_ss0.png" alt="alt text" width="50%" height="50%"><img src="./docs/img/editor_ss1.png" alt="alt text" width="50%" height="50%">
<img src="./docs/img/editor_ss2.png" alt="alt text" width="50%" height="50%"><img src="./docs/img/editor_ss3.png" alt="alt text" width="50%" height="50%">

__The main features of the engine are:__
 - Cross platform working on Windows and GNU/Lunix.
 - Scene editor for 3D and pseudo 2D scenes. Having all common.features like transform, tools, property editor, undo/redo, curve editing and more.
 - C++ hot reloading.
 - Direct3D 11, OpenGL 3.3 and WebGL 2 rendering backends.
 - Physics.
 - Path finding.
 - Importing external assets like GLTF, FBX, DAE and OBJ.
 - Simple particle effects.
 - Rich math library.
 - Timeline animations.
 - Material System.

[](https://user-images.githubusercontent.com/6237727/114287179-95c8a700-9a6d-11eb-9fdd-54009834ef2f.mp4)
>>>>>>> 2210545169dc35f6df4587bfc08a7d56cc5110d1

 __Roadmap:__
  - Flexible rendering pipeline.
  - OSX builds.
  - Better Particle Systems.
  - Better Inverse Kinematics.

## Philosophy
 
__SGEEngine__ strives to be simple to use, debug and modify. It is aimed at simpler games and application, however this does not stop you to extend it and build more complex ones.
Unlike other games engines, SGEEngine does not use the popular entity-component-system (or its derivatives). Instead it takes the classic game object approach - base game actor type that can expose its functionally with interfaces (called Traits). Traits are similar to components in Unity, however they are known in compile time. This means that for each different game object type you will construct a small Actor type that will have the desired traits. 
The engine comes with commonly used actors pre-built, like: static/dynamic obstacles, lights, level blocking actors, cameras, lights, navmeshes and others.

## Building

SGEEngine uses CMake as main, build system. Getting the engine running should be straightforward.

Minimum supported version of CMake is 3.14.
Importing 3D models requires the usage of Autodesk FBX SDK 2020.0.1 or later. Compiling SGEEngine without it is possible however you would not be able to import FBX, DAE and OBJ files. Autodesk FBX SDK is not needed for your final game it is used only as tool.

Here is a screenshot describing how to configure SGEEngine:
<img src="./docs/img/cmake_config.png" alt="alt text">
