# Asset pipeline

Images, maps, fonts and other data are packed into your game at build time by the `32blit` tool, which you installed from `tools/requirements.txt` (see [Install the tools](../getting-started/install.md#the-32blit-tools)). The packed data ends up in flash alongside your code, so loading it costs no file access at run time.

Use the requirements file rather than a bare `pip install 32blit`: it pins `bitstring<5`, because bitstring 5 removed `ConstBitStream`, which the tools still import, and it requires the minimum `32blit` version the SDK's CMake checks for. The tool's own documentation is at [32blit/32blit-tools](https://github.com/32blit/32blit-tools).

## Using `assets.yml`

List your assets in an `assets.yml` file next to `CMakeLists.txt`, and add it to the build right after `blit_executable`:

```cmake
blit_assets_yaml(${PROJECT_NAME} assets.yml)
```

A simple `assets.yml` looks like this:

```yaml
assets.cpp:
  assets/sprites.png:
    name: asset_sprites
  assets/level.tmx:
    name: asset_level
    output_struct: true
```

The top-level key is the file to generate. CMake adds `assets.cpp` to your sources and writes an `assets.hpp` next to it, declaring each asset and its length:

```cpp
#include "assets.hpp"

// extern const uint8_t asset_sprites[];
// extern const uint32_t asset_sprites_length;
```

CMake reruns the tool whenever `assets.yml` or one of the listed files changes.

## Asset types

The type is picked from the file extension; set `type:` to override it.

| Files | Type | Use it with |
| ----- | ---- | ----------- |
| `.png`, `.gif` | `image/image` | `Surface::load(asset)` for sprites and pictures |
| `.tmx` (Tiled) | `map/tiled` | `TileMap::load_tmx(asset, sheet)`, with `output_struct: true` |
| `.png`, `.gif` drawn as a font | `font/image` (set it explicitly) | `const Font font(asset);` |
| `.ttf`, or other formats FreeType reads with `type: font/font` | `font/font` | `const Font font(asset);` |
| `.csv` | `raw/csv` | an array of bytes |
| `.bin`, `.raw`, or anything with `type: raw/binary` | `raw/binary` | the file as-is, for example `.wav` or `.mp3` sounds with `File::add_buffer_file()` |

### Images

```cpp
screen.sprites = Surface::load(asset_sprites);
```

Images are packed with a palette of the colours they use, at the fewest bits per pixel that hold that many colours, so a 4-colour image takes 2 bits per pixel in flash. `Surface::load()` unpacks them into an 8-bit paletted surface in RAM. Pixels that are fully transparent in the PNG stay transparent.

### Maps

For a level you edit while playing, `TileMap::load_tmx()` makes its own copy of the tiles. If you build a map by hand from a raw asset, copy the data into RAM first:

```cpp
constexpr uint16_t level_width = 64;
constexpr uint16_t level_height = 64;

auto local_level_data = new uint8_t[level_width * level_height];
memcpy(local_level_data, asset_level, level_width * level_height);

level = new TileMap(local_level_data, nullptr, Size(level_width, level_height), screen.sprites);
```

See [Tile maps](../tutorials/tilemaps.md).

## Additional options

Every option, with its default:

```yaml
assets.cpp:                         # Output filename, can also be a .hpp
    assets/sprites.png:
        name: sprites_data          # Name of variable generated for this asset, if omitted is generated from the
                                    # file name

        type: image/image           # Type of this asset, usually auto-detected from the extension

        palette: assets/palette.png # Optional palette for the image. Supports .act, .pal, .gpl and any image file

        transparent: 255,0,255      # Optional colour to map to transparency

        packed: yes                 # Pack the image into the minimum number of bits, for example an image with
                                    # two colours would use one bit per pixel. Set to false to always use eight
                                    # bits per pixel

        strict: true                # Allow only the colours defined in the palette, if false (the default)
                                    # colours are automatically added to the palette

    # If you do not need to specify any options other than the name, you can use
    assets/sprites.png: sprites_data

    # Parses a Tiled map
    assets/level.tmx:
        name: level_data
        type: map/tiled
    
    # Embeds a file without any processing
    assets/raw.bin:
        name: raw_bin
        type: raw/binary
    
    # Parses a CSV file into an array of bytes
    assets/raw.csv:
        name: csv_data
        type: raw/csv
     
    # Generates a font from an image
    # It can be used as `const Font font(asset_font)`
    assets/font.png:
        name: asset_font

        type: font/image            # This is required here as it would be auto-detected as an image asset

        height: 8                   # The height of the font, should match the image file if specified

        horizontal_spacing: 1       # Additional space between characters for variable-width mode (defaults to 1)

        vertical_spacing: 1         # Space between lines (defaults to 1)

        space_width: 4              # Width of the space character (defaults to 1)

    # Generates a font from a font file, can use any format supported by freetype
    assets/Comic_Sans_MS.ttf:
        name: asset_comic_font

        type: font/font

        height: 16                  # The height of the font to request, the resulting font's height may be
                                    # slightly different

        vertical_spacing: 1         # Space between lines (defaults to 1)

# It is also possible to define multiple outputs
more-assets.hpp:
    assets/extra_sprites.png: extra_sprites

```

## Visual Studio
To use `assets.yml` with a Visual Studio project, you need to run the packer as a pre-build step. Make sure that the output path is in the project's include path.
```
python -m ttblit pack --force --config $(ProjectDir)assets.yml --output $(ProjectDir)
```
To add the build step right click the project and click properties. From there go to build events and paste the command into the command line text box.

If the step is failing to complete then make sure that python is installed and that the 32blit tools are installed from `tools/requirements.txt` (see above). 

After the build completes you will want to add the two files it generates to the project. (assets.cpp and assets.hpp).
Right click the project go to add then existing item and choose the two files from the project directory.

Failing to do this may cause errors when trying to debug similar to the ones below.

`LNK2001	unresolved external symbol "unsigned char const * const sprites_data" (?sprites_data@@3QBEB)`

`LNK1120	1 unresolved externals`
