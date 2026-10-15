# Publishing a game

A finished game is a single `.blit` file. Players copy it onto their console's drive, eject, and it installs. This guide covers building releases, versioning, and making sure the file works on players' consoles.

## Releases with the template's CI

Games made from the [game template](https://github.com/chili-chip/game-template) come with a GitHub Actions workflow that builds every target on every push:

| Job | Output |
| --- | ------ |
| VGC (.blit) | `game.blit`, uploaded as the `game-blit` workflow artifact, and checked to be an RP2350 `.blit` |
| VGC | a standalone `game.uf2`, uploaded as `game-uf2` |
| Linux, macOS, Visual Studio | desktop builds |
| Emscripten | a web build, deployed to GitHub Pages on pushes to `main` |
| STM32 | a build for the original 32blit handheld |

To release:

1. Bump `version` in `metadata.yml`.
2. Push, and check the workflow is green.
3. On GitHub, **Releases → Draft a new release**, tag it with the version (for example `v1.1.0`), and publish.

Publishing the release runs the workflow again, which attaches `game.blit` plus a zip and tarball for each platform to the release. Point players at the `.blit`.

For the web build, enable GitHub Pages for the repository with **Settings → Pages → Source: GitHub Actions**. Set `url` in `metadata.yml` to the page so players can find it.

## Versions and updates

* Keep the `title` and `author` the same across releases. Installing a `.blit` with the same title and author replaces the installed copy and keeps its saves; a different title or author installs as a separate game. (Saves are kept by title alone.)
* Use `version` for what players see on the info page. The console doesn't compare versions: any build that differs from the installed one replaces it, and copying the exact same build again just removes the copy from the drive.
* If your save layout changes, keep a `version` field in your save struct and convert old saves (see [Saving progress](../tutorials/saving.md#loading)).

## Compatibility with players' consoles

Each `.blit` records the API version of the SDK it was built with, and a console only installs games whose API version is the same as or older than its loader's. A newer game shows "Needs a system update", whether or not it uses the new calls.

| API | Added | Calls |
| --- | ----- | ----- |
| 0.3 | battery, USB and settings | `get_battery_level()`, `get_battery_voltage()`, `is_usb_connected()`, `get_system_settings()`, `set_system_settings()` |
| 0.4 | system info | `get_system_info()` |
| 0.5 | save on quit | `on_quit` |

Older games keep working on newer loaders. A game built with this SDK (API 0.5) needs a loader from API 0.5 onwards. If players see "Needs a system update", they should update the loader (see [Flashing and the loader](../console/flashing.md#updating-the-os)); alternatively, build with the older SDK version their loader came with.

## Checklist

* [ ] `title`, `author`, `description` and `version` filled in; title short enough to read under the cover.
* [ ] A 128×96 cover that reads well when scaled down, and an 8×8 icon.
* [ ] Tested on a console, not just the desktop build: speed, memory and controls.
* [ ] Saves at checkpoints and in `on_quit`, and loads cleanly with no save present.
* [ ] Doesn't use HOME. MENU pauses, if the game has a pause menu.
* [ ] Volume is reasonable at the console's default volume.
* [ ] A release build: the template's presets build Debug, so build releases with `-DCMAKE_BUILD_TYPE=Release` or from CI, which uses Release.
