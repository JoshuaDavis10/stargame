# stargame

This repository contains code for the Stargame engine, which is a CPU rendered game engine. Code for the engine is in the `engine_code` directory. The engine is supported on Windows and **TO AN EXTENT** supported on Linux. Platform layers/platform specific code is found in the `platform_code` directories. The `win32_platform.c` file and `linux_platform.c` file are the entry points for Windows and Linux, respectively. This repository also contains code for various "apps" in `app_code` that run using the Stargame engine. Some of them are just demos and tests, while some of them are actual games.

# Windows

To build a particular application, run the `compile.bat` file followed by the name of the app that you want to build.

For example, in Command Prompt:

```
compile particle_test
```

This will build both the Win32 platform layer entry point, and a .dll for the `app_code/particle_test.c` program, which the entry point will load when run.

To run a built application, run `run.bat`, followed by the name of the app that you want to build. 

For example, in Command Prompt:

```
run particle_test
```

Note that the *tilegame* and *tilegame_editor* applications, take a filepath as a second parameter, so in that case add the filepath after the application name.

e.g.:

```
run tilegame app_code\tilegame\levels\<level>.lvl
```

```
run tilegame_editor app_code\tilegame\levels\<level>.lvl
```

# Linux

**NOTE: Linux is currently not functional! None of the apps have been tested on Linux since some major changes were made, so they may or may not work (I suspect they will not)**

Scripts will be added to build and run the apps on Linux as well.

**TODO:** Test compilation and runs of all apps on Linux, and make appropriate changes to platform code as needed.

# Applications

## tilegame

**NOTE: tilegame is currently not working as intended!**

tilegame is a tile based puzzle game. It is really cool, but it's not working right now, due to me starting a graphics rehaul that I never really made progress on. You can run the application, but it'll just draw colored squares on a black background or something, and that's it.

## tilegame_editor

tilegame_editor is the editor for tilegame levels and it is 100% functional. 

Controls:
- `CTRL + W`  - increase tile grid width  (max width  is 7)
- `CTRL + H`  - increase tile grid height (max height is 7)
- `SHIFT + W` - decrease tile grid width  (min width  is 1)
- `SHIFT + H` - decrease tile grid height (min height is 1)
- `B`         - makes tile that mouse is currently over *blue* 
- `G`         - makes tile that mouse is currently over *green*
- `R`         - makes tile that mouse is currently over *red*
- `CTRL + B`  - places a *blue*  unit on the tile that the mouse is currently over
- `CTRL + G`  - places a *green* unit on the tile that the mouse is currently over
- `CTRL + R`  - places a *red*   unit on the tile that the mouse is currently over
- `CTRL + 1`  - increases *blue count*  by 1
- `CTRL + 2`  - increases *green count* by 1
- `CTRL + 3`  - increases *red count*   by 1
- `CTRL + 1`  - decreases *blue count*  by 1
- `CTRL + 2`  - decreases *green count* by 1
- `CTRL + 3`  - decreases *red count*   by 1
- `SPACEBAR`  - save changes

**NOTE:** blue count, green count, and red count determine how many of each color unit the player can place in the level

## particle_test

particle_test is just a program that I have been using to test my particle system. Run it to see some cool particles I guess.

## old_test

old_test was the first functional program to use Stargame's `draw_mesh` function (see `engine_code/cpu_render.c`). It's not impressive at all, but it was a big milestone to be able to do vertex-based rendering via triangle rasterization and world space -> camera space -> screen space translation (before that point, Stargame could just draw various shapes in screen coordinates). The program was still lying around in my Stargame folder, so I figured I would keep it around for good as a keepsake.

## template_game

template_game is supposed to just be an application template. It does some things you might expect from an application, like having a game_state struct that it stores in game memory and initializes on the first frame, clearing the background to a super boring gray color, and printing profiler data every 2 seconds.
