# Files

`engine/file.hpp`

Games can read and write files on the console's USB drive (or, on the desktop, in the folder the game runs from). Most games only need [saves](saves.md); files are for things like user levels, logs, or loading data the player copies onto the drive.

## Storage

```cpp
bool is_storage_available();
```

`true` if files can be used. On the VGC Zero the drive is always there; on hardware with an SD card it's `false` without one.

## Directories

```cpp
struct FileInfo {
  std::string name;
  int flags;       // FileFlags::directory for folders
  uint32_t size;
};

std::vector<FileInfo> list_files(const std::string &path,
                                 std::function<bool(const FileInfo &)> filter = nullptr);
bool file_exists(const std::string &path);
bool directory_exists(const std::string &path);
bool create_directory(const std::string &path);
bool rename_file(const std::string &old_name, const std::string &new_name);
bool remove_file(const std::string &path);
```

`list_files(path, filter)`
:   The files and folders in `path`. With a `filter`, only entries for which it returns `true` are included:

    ```cpp
    auto levels = list_files("levels", [](const FileInfo &f) {
      return !(f.flags & FileFlags::directory) && f.name.ends_with(".lvl");
    });
    ```

The other functions return `true` on success. `create_directory` doesn't create missing parent folders.

## `File`

```cpp
enum OpenMode { read = 1, write = 2, cached = 4 };

class File {
public:
  File() = default;
  File(const std::string &filename, int mode = OpenMode::read);
  File(const uint8_t *buf, uint32_t buf_len);   // read from memory

  bool open(const std::string &file, int mode = OpenMode::read);
  bool open(const uint8_t *buf, uint32_t buf_len);
  int32_t read(uint32_t offset, uint32_t length, char *buffer);
  int32_t write(uint32_t offset, uint32_t length, const char *buffer);
  void close();
  uint32_t get_length();
  bool is_open() const;
  const uint8_t *get_ptr() const;

  static void add_buffer_file(std::string path, const uint8_t *ptr, uint32_t len);
};
```

A `File` closes itself when destroyed. It can be moved but not copied.

`File(filename, mode)`, `open(filename, mode)`
:   Opens a file. `OpenMode::read` opens an existing file for reading. `OpenMode::write` creates the file, or empties it if it exists. `OpenMode::read | OpenMode::write` opens an existing file for both without emptying it. Check `is_open()` afterwards.

`read(offset, length, buffer)`, `write(offset, length, buffer)`
:   Read or write `length` bytes at `offset` from the start of the file. Return the number of bytes done, or a negative number on error. Every call says where in the file to work, so there's no seek.

`get_length()`
:   The file's size in bytes.

`File(buf, len)`
:   Treats a block of memory as a read-only file. `get_ptr()` returns the memory.

`OpenMode::cached`
:   On hardware that supports it, copies the file to fast temporary flash for quicker reads. It has no effect on the VGC Zero.

### In-memory files

```cpp
File::add_buffer_file("music.wav", asset_music, asset_music_length);
```

Registers a block of memory, such as a packed asset, under a file name. Anything that opens files by name, like `WavStream::load()` or `Surface::load()`, can then use it. Its folder doesn't need to exist, but `list_files()` on that folder lists it.

## Example

```cpp
// append a line to a log file on the drive
void log_line(const std::string &line) {
  File f("log.txt", OpenMode::read | OpenMode::write);
  if(!f.is_open())
    f.open("log.txt", OpenMode::write);
  f.write(f.get_length(), line.size(), line.c_str());
}
```

## Paths

Paths use `/` and are relative to the root of the storage: the CHILICHIP drive on the VGC Zero, the game's folder on the desktop. Avoid writing into `.blit/` and `.system/`, which belong to the OS.
