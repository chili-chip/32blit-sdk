#pragma once

// HOME short press: opens the system menu over a running game, or closes it
void system_menu_toggle();
bool system_menu_is_open();
// close the menu without resuming, before quitting the game
void system_menu_reset();

// defined in main.cpp
void quit_to_launcher();
