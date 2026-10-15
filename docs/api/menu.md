# Menu

`engine/menu.hpp`

`Menu` is a ready-made scrolling list with a title bar, for title screens, pause menus and options. You give it an array of items, call its `update()` and `render()`, and override a few functions to react to the player.

## Class

```cpp
class Menu {
public:
  static const uint16_t Separator = 65535;

  struct Item {
    uint16_t id;
    const char *label;
  };

  Menu(std::string_view title, const Item *items = nullptr, int num_items = 0,
       const Font &font = minimal_font);

  virtual void update(uint32_t time);
  void render();

  void set_items(const Item *items, int num_items);
  void set_display_rect(Rect rect);

  std::string_view title;

protected:
  virtual void item_activated(const Item &item);      // A was pressed on an item
  virtual void update_item(const Item &item);         // every update, for the selected item
  virtual void update_menu(uint32_t time);            // every update
  virtual void render_menu();
  virtual void render_item(const Item &item, int y, int index) const;
  virtual void render_header(int x, int y, int w);
  virtual void render_footer(int x, int y, int w);

  const Item *items;
  int num_items;
  int current_item = 0;
  ...
};
```

## Using it

Derive a class and override `item_activated()`:

```cpp
enum MenuId { PLAY, OPTIONS, QUIT };

const Menu::Item main_items[] = {
  {PLAY, "Play"},
  {OPTIONS, "Options"},
  {Menu::Separator, nullptr},
  {QUIT, "Quit"},
};

class MainMenu : public Menu {
public:
  MainMenu() : Menu("Chili Catch", main_items, 4) {}

protected:
  void item_activated(const Item &item) override {
    switch(item.id) {
      case PLAY:    start_game(); break;
      case OPTIONS: show_options(); break;
      case QUIT:    save_and_quit(); break;
    }
  }
};

MainMenu main_menu;

void update(uint32_t time) {
  if(state == TITLE)
    main_menu.update(time);
}

void render(uint32_t time) {
  if(state == TITLE)
    main_menu.render();
}
```

## Behaviour

`update(time)`
:   Moves the selection with UP and DOWN (repeating every 200 ms while held, and wrapping around), calls `item_activated()` when A is **released** on an item, and scrolls smoothly to keep the selection on screen. Then it calls `update_menu()` and `update_item()` for the selected item.

`render()`
:   Fills the menu's rectangle with the background colour and draws the header, items and footer. Call it in `render()`, after drawing whatever is behind the menu: the default background is translucent.

`set_display_rect(rect)`
:   Where the menu is drawn. By default it covers the whole screen, from the first `update()`.

`set_items(items, num_items)`
:   Replaces the items, for example to switch to a submenu, and selects the first one. The menu keeps a pointer to the array, so it must stay alive (a `const` global is best).

### Items

* `id` is yours: use it to tell items apart in `item_activated()`.
* An item with a `nullptr` label isn't drawn and can't be selected. With the id `Menu::Separator` it leaves a gap; with any other id it takes no space, which lets you hide an item without rebuilding the array.
* The first item should be selectable, because it's selected to begin with. A menu needs at least one selectable item: with none, pressing UP or DOWN loops forever looking for one.

### Changing labels

Labels are plain `const char *`. For a setting that shows its value, such as "Sound: on", keep the text in a buffer you update, or override `render_item()` and draw the value yourself:

```cpp
void render_item(const Item &item, int y, int index) const override {
  Menu::render_item(item, y, index);

  if(item.id == SOUND) {
    screen.pen = foreground_colour;
    screen.text(sound_on ? "on" : "off", font,
                Point(display_rect.x + display_rect.w - item_padding_x, y + item_adjust_y),
                true, TextAlign::top_right);
  }
}
```

`update_item()` is a good place to change a setting with LEFT and RIGHT while its item is selected.

## Appearance

The look is set by protected members you can change in your class's constructor:

| Member | Default | What |
| ------ | ------- | ---- |
| `background_colour` | `Pen(30, 30, 50, 200)` | behind the items |
| `foreground_colour` | white | item text |
| `selected_item_background` | `Pen(50, 50, 70)` | the selected item's bar |
| `header_background`, `header_foreground` | near-white, near-black | title and footer bars |
| `header_h`, `footer_h` | `16`, `16` | bar heights; `0` hides the footer |
| `item_h`, `item_spacing` | `9`, `1` | item height and gap |
| `item_padding_x` | `5` | text indent |
| `margin_y` | `5` | space between the header and the first item |
| `display_rect` | the screen | where it's drawn |

```cpp
MainMenu() : Menu("Chili Catch", main_items, 4, outline_font) {
  item_h = 12;
  footer_h = 0;
  selected_item_background = Pen(200, 40, 40);
  set_display_rect(Rect(16, 24, 96, 80));
}
```

Override `render_header()` or `render_footer()` to draw your own bars, for example button hints in the footer. Call the base version first: it fills the bar's background.
