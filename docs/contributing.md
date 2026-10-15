# Working on these docs

This site is built with [MkDocs](https://www.mkdocs.org/) and the [Material](https://squidfunk.github.io/mkdocs-material/) theme from the Markdown files in the SDK's [`docs/`](https://github.com/chili-chip/32blit-sdk/tree/master/docs) folder. Every push to `master` rebuilds it and publishes it to GitHub Pages, next to the web builds of the examples.

## Preview locally

From the SDK's folder:

```bash
python3 -m pip install -r docs/requirements.txt
mkdocs serve
```

Open <http://127.0.0.1:8000>. The page reloads as you save files.

Before opening a pull request, build it the way CI does:

```bash
mkdocs build --strict
```

`--strict` turns warnings, such as a link to a page or heading that doesn't exist, into errors. The output goes to `build-docs/`, because `site/` holds the web example shell.

## Layout

| Folder | Contents |
| ------ | -------- |
| `docs/getting-started/` | installing, the first game, building for desktop and console |
| `docs/tutorials/` | the numbered lessons |
| `docs/guides/` | assets, metadata, performance, debugging, publishing |
| `docs/console/` | the VGC Zero and ChiliChip OS |
| `docs/api/` | the API reference, one page per topic |
| `docs/setup/` | per-OS setup notes inherited from 32blit |
| `docs/hardware/` | the original 32blit and other Pico boards |
| `docs/examples/` | the example games (real, buildable code) |
| `docs/assets/` | the logo, stylesheet and screenshots |

The navigation is listed in `mkdocs.yml`; a new page needs an entry there.

## Code on the pages

Longer code comes from the example games rather than being pasted into pages, so it's always code that compiles. Pages include a file, or a marked section of one, with snippets:

```cpp
;--8<-- "chili-catch/chili-catch.cpp:render"
```

Paths are relative to `docs/examples/`. A section is marked in the source with comments, which don't appear on the page:

```cpp
// --8<-- [start:render]
void render(uint32_t time) {
  ...
}
// --8<-- [end:render]
```

## Building the examples

The examples are one CMake project. For the desktop:

```bash
cmake -S docs/examples -B build.examples
cmake --build build.examples
```

For the VGC Zero, add the console options from [Run on the console](getting-started/build-console.md#build-a-blit). Every example has its own `metadata.yml` and a 128×96 `cover.png`; shared sprites and the icon are in `docs/examples/shared/`.

Examples are built with `-Wall -Wextra -Wdouble-promotion`, and should build without warnings for both platforms. CI builds them for the desktop on every pull request.

## Screenshots

Screenshots are 256×256 PNGs of the desktop build's window (128×128 at 2×), in `docs/assets/screenshots/`, named after the example. Use `{ .screenshot }` after an image to give it the console-screen frame, and wrap several in `<div class="gallery" markdown>` for a row.

## Writing style

* Write for someone making their first game: explain what a thing is for before how it works.
* Say how things behave **on the VGC Zero** where it differs from the desktop or the original 32blit.
* Check behaviour against the code, and say plainly when something is surprising or broken rather than working around it silently.
