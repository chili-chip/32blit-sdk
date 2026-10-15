# Metadata and the home screen

`metadata.yml` describes your game: its name, who made it, its version, and the pictures the home screen shows. CMake reads it with `blit_metadata()` and stores it in the built game.

```yaml title="metadata.yml"
title: Chili Catch
description: Catch the chilis, dodge the bombs.
author: ChiliChip
version: v1.0.0
category: game
url: https://github.com/you/chili-catch
icon:
  file: assets/icon.png
splash:
  file: assets/cover.png
```

```cmake title="CMakeLists.txt"
blit_metadata(${PROJECT_NAME} metadata.yml)
```

## Fields

| Field | Required | Limit | Shown |
| ----- | -------- | ----- | ----- |
| `title` | yes | 24 characters | under the cover on the home screen, and on the info page. Also names the [save folder](../console/storage.md#saves). |
| `author` | yes | 16 characters | under the title, and on the info page. With the title, identifies the game when a new build replaces an installed one. |
| `version` | yes | 16 characters | on the info page, for example `v1.2.0` |
| `description` | no | 128 characters | on the info page, wrapped |
| `category` | no | 16 characters | `game` for games. `launcher` makes the file a home screen, which replaces the installed one. |
| `url` | no | 128 characters | where to find the game |
| `icon` | yes, for console builds | 8×8 pixels | a small icon |
| `splash` | yes, for console builds | 128×96 pixels | the game's cover in the carousel |
| `filetypes` | no | extensions of up to 4 characters | file types the game can open, for launchers that support it |

Quote values that contain a colon, such as `description: "Level 2: the caves"`, or YAML will reject the file.

Console builds stop with "An 8x8 pixel icon is required" or "A 128x96 pixel splash is required" if either image is missing or the wrong size. Desktop builds don't need them.

## Cover art

The home screen shows the selected game's `splash` image large in the middle of the screen, with its neighbours smaller at the edges. Make it count:

* Draw it at exactly **128×96**. Other sizes are rejected.
* The carousel scales the cover down to fit its tile, about three quarters size in the middle and smaller at the sides, so keep shapes bold and avoid fine text.
* A screenshot of the game makes a decent cover. Grab one from the desktop build and crop it to 128×96.

Games without a valid splash get a coloured card with their title.

## Reading metadata in the game

`get_metadata()` returns the game's own metadata, which is handy for a title or credits screen:

```cpp
auto meta = get_metadata();
screen.text(std::string(meta.title) + " " + meta.version, minimal_font, Point(2, 2));
```

`get_version_string()` and `get_build_date()` describe the SDK the game was built with. See [Engine](../api/engine.md#metadata-and-version).

## Changing the title later

Saves are stored by title, so renaming a released game makes it look for saves in a new folder: players keep their old saves on the drive, but the game no longer finds them. If you must rename, keep the old `title` and change only what you show in your own title screen.
