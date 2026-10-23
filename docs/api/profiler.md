# Profiler

`engine/profiler.hpp`, `engine/running_average.hpp`

The profiler measures how long parts of your code take, in microseconds, and can show the results as an on-screen overlay or print them to the [debug output](../guides/debugging.md#printing-messages). See [Performance](../guides/performance.md) for what to do with the numbers.

For a one-off measurement, [`now_us()` and `us_diff()`](engine.md#time) are simpler.

## Quick start

```cpp
Profiler profiler(50);          // keep a running average of the last 50 samples
ProfilerProbe *update_probe;
ProfilerProbe *render_probe;

void init() {
  set_screen_mode(ScreenMode::hires);

  profiler.set_display_size(128, 128);
  profiler.set_rows(4);
  profiler.setup_graph_element(Profiler::dmCur, true, true, Pen(0, 255, 0));
  profiler.setup_graph_element(Profiler::dmAvg, true, false, Pen(0, 255, 255));
  profiler.display_history(false);

  update_probe = profiler.add_probe("update");
  render_probe = profiler.add_probe("render");
}

void update(uint32_t time) {
  ScopedProfilerProbe scope(update_probe);
  // ... game logic ...
}

void render(uint32_t time) {
  render_probe->start();
  // ... drawing ...
  render_probe->store_elapsed_us();

  if(buttons & Button::Y)
    profiler.display_probe_overlay(1);   // page 1
}
```

## `ProfilerProbe`

One measured section of code.

```cpp
class ProfilerProbe {
public:
  struct Metrics {
    uint32_t uElapsedUs;      // the last measurement
    uint32_t uMinElapsedUs;
    uint32_t uMaxElapsedUs;
    uint32_t uAvgElapsedUs;   // running average, or the last value without one
  };

  ProfilerProbe(const char *name, uint32_t running_average_size = 0,
                uint32_t running_average_span = 1);

  void start();
  uint32_t store_elapsed_us(bool restart = false);
  void clear();

  const Metrics &elapsed_metrics();
  const char *name();
  const RunningAverage<float> *get_running_average();

  void set_graph_time_us(uint32_t us);
  void set_graph_time_us_to_max();
  uint32_t get_graph_time_us();
};
```

`start()`
:   Records the start time.

`store_elapsed_us(restart)`
:   Records the time since `start()`, updates the minimum, maximum and average, and returns it. With `restart = true` it also starts the next measurement, for timing consecutive sections with one probe. Calls before the first `start()` record nothing.

`elapsed_metrics()`
:   The latest numbers.

`clear()`
:   Resets the metrics and the running average. Only call it on probes that have a running average (`running_average_size` above 0); on others it crashes.

`running_average_size`, `running_average_span`
:   The average covers the last `size` samples, taking one sample every `span` measurements. With `size` 0 there's no average, and `uAvgElapsedUs` is just the last value.

`set_graph_time_us(us)`
:   The time a full-width bar stands for, used when the profiler's own graph time is `0`.

## `ScopedProfilerProbe`

```cpp
explicit ScopedProfilerProbe(ProfilerProbe *probe);
```

Starts the probe when created and stores the elapsed time when it goes out of scope, so a whole function or block is measured with one line.

## `Profiler`

A collection of probes plus the overlay.

```cpp
class Profiler {
public:
  enum DisplayMetric { dmMin, dmCur, dmAvg, dmMax };

  Profiler(uint32_t running_average_size = 0, uint32_t running_average_span = 1);

  ProfilerProbe *add_probe(const char *name);
  ProfilerProbe *add_probe(const char *name, uint32_t running_average_size,
                           uint32_t running_average_span = 1);
  void start_all_probes();
  void clear_all_probes();
  void log_probes();
  size_t get_probe_count();
  size_t get_page_count();

  void set_display_size(uint16_t width, uint32_t height);
  void set_rows(uint8_t rows);
  void set_graph_time(uint32_t us);
  void set_alpha(uint8_t alpha);
  void setup_graph_element(DisplayMetric metric, bool label, bool graph, Pen colour);
  void display_history(bool show, Pen colour = Pen(0, 255, 0));
  void display_probe_overlay(uint8_t page);
};
```

`Profiler(size, span)`
:   The running average settings given to probes made with `add_probe(name)`.

`add_probe(name)`
:   Creates a probe owned by the profiler. `name` must stay valid (a string literal is ideal). There's no way to remove probes, so create them once in `init()`.

`log_probes()`
:   Prints each probe's name and min, current, average and max times with `debugf()`, one line per probe. Call it now and then (for example every second with a [Timer](timers-tweens.md#timer)), not every frame.

`start_all_probes()`, `clear_all_probes()`
:   Call `start()` or `clear()` on every probe.

### The overlay

`display_probe_overlay(page)`
:   Draws the probes over the screen: a header with the graph time and page, then one row per probe with its name, the numbers chosen with `setup_graph_element()`, and bars. Call it at the end of `render()`. `page` starts at 1; `0` draws nothing.

`set_display_size(w, h)`
:   The overlay's size. The default is 160×120, from the original 32blit, so set it to the screen size (`128, 128` in hires, `64, 64` in lores) in `init()`.

`set_rows(rows)`
:   How many probe rows fit, which sets the row height. Call it after `set_display_size()`.

`setup_graph_element(metric, label, graph, colour)`
:   For each of `dmMin`, `dmCur`, `dmAvg` and `dmMax`: whether to print its number (`label`) and whether to draw a bar for it (`graph`), and the bar's colour. By default only the current value is printed.

`set_graph_time(us)`
:   The time a full-width bar stands for; the default is 20 000 µs (20 ms). `0` uses each probe's own graph time.

`display_history(show, colour)`
:   Draws each probe's running average history as a line graph in its row.

`set_alpha(alpha)`
:   The overlay's opacity.

!!! warning "Set up the overlay before showing it"
    * Turn on a bar for at least one metric with `setup_graph_element(..., true, ...)`. With no bars, the overlay divides by zero, which crashes the desktop build.
    * Call `display_history()` once, with `true` or `false`, before the first `display_probe_overlay()`: until then the setting isn't initialised.
    * The page number only changes the header: every page lists all the probes from the first, so with more probes than rows the later ones run off the bottom. Keep to as many probes as fit.

## `RunningAverage`

```cpp
template<typename T>
class RunningAverage {
public:
  RunningAverage(std::size_t size);

  void add(T value);
  T average();
  void reset();

  const T &operator[](std::size_t i) const;  // oldest first
  std::size_t count() const;                 // how many values it holds
  std::size_t size() const;                  // capacity
};
```

The average of the last `size` values added, updated in constant time once full. It's useful on its own, for example to smooth a frames-per-second readout:

```cpp
RunningAverage<float> frame_ms(30);

void render(uint32_t time) {
  static uint32_t last = time;
  frame_ms.add(float(time - last));
  last = time;

  screen.pen = Pen(255, 255, 255);
  char buf[16];
  snprintf(buf, sizeof(buf), "%d fps", int(1000.0f / std::max(frame_ms.average(), 1.0f)));
  screen.text(buf, minimal_font, Point(2, 2));
}
```

Avoid `add_all()`: it writes to storage that `reset()` has just emptied.
