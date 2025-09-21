#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "SDL.h"
#include <iostream>

#include "Input.hpp"
#include "Multiplayer.hpp"
#include "System.hpp"
#include "Renderer.hpp"
#include "Audio.hpp"
#include "UserCode.hpp"
#include "32blit.hpp" // for blit::Button enum used in virtual key polling

#include "contrib.hpp"

#ifdef VIDEO_CAPTURE
#include "VideoCapture.hpp"
#endif

static bool running = true;

SDL_Window* window = nullptr;

System *blit_system;
Input *blit_input;
Multiplayer *blit_multiplayer;
Renderer *blit_renderer;
Audio *blit_audio;

const char *launch_path = nullptr;

#ifdef VIDEO_CAPTURE
VideoCapture *blit_capture;
unsigned int last_record_startstop = 0;
#endif

void handle_event(SDL_Event &event) {
	switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;

		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				blit_renderer->resize(event.window.data1, event.window.data2);
			}
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			blit_input->handle_mouse(event.button.button, event.type == SDL_MOUSEBUTTONDOWN, event.button.x, event.button.y);
			break;

		case SDL_MOUSEMOTION:
			if (event.motion.state & SDL_BUTTON_LMASK) {
				blit_input->handle_mouse(SDL_BUTTON_LEFT, event.motion.state & SDL_MOUSEBUTTONDOWN, event.motion.x, event.motion.y);
			}
			break;

		case SDL_KEYDOWN: // fall-though
		case SDL_KEYUP:
			if (!blit_input->handle_keyboard(event.key.keysym.sym, event.type == SDL_KEYDOWN)) {
#ifdef VIDEO_CAPTURE
				switch (event.key.keysym.sym) {
				case SDLK_r:
					if (event.type == SDL_KEYDOWN && SDL_GetTicks() - last_record_startstop > 1000) {
						if (blit_capture->recording()) blit_capture->stop();
						else blit_capture->start();
						last_record_startstop = SDL_GetTicks();
					}
				}
#endif
			}
			break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			blit_input->handle_controller_button(event.cbutton.button, event.type == SDL_CONTROLLERBUTTONDOWN);
			break;

		case SDL_CONTROLLERAXISMOTION:
			blit_input->handle_controller_motion(event.caxis.axis, event.caxis.value);
			break;

		case SDL_CONTROLLERDEVICEADDED:
      blit_input->handle_controller_added(event.cdevice.which);
			break;

		case SDL_CONTROLLERDEVICEREMOVED:
      blit_input->handle_controller_removed(event.cdevice.which);
			break;

#if SDL_VERSION_ATLEAST(2, 0, 14)
    case SDL_CONTROLLERSENSORUPDATE:
      if(event.csensor.sensor == SDL_SENSOR_ACCEL)
        blit_input->handle_controller_accel(event.csensor.data);
      break;
#endif

		case SDL_RENDER_TARGETS_RESET:
			std::cout << "Targets reset" << std::endl;
			break;

		case SDL_RENDER_DEVICE_RESET:
			std::cout << "Device reset" << std::endl;
			break;

		default:
			if(event.type == System::loop_event) {
				blit_renderer->update(blit_system);
				blit_system->notify_redraw();
				blit_renderer->present();
#ifdef VIDEO_CAPTURE
				if (blit_capture->recording()) blit_capture->capture(blit_renderer);
#endif
			} else if (event.type == System::timer_event) {
				switch(event.user.code) {
					case 0:
						SDL_SetWindowTitle(window, metadata_title);
						break;
					case 1:
						SDL_SetWindowTitle(window, (std::string(metadata_title) + " [SLOW]").c_str());
						break;
					case 2:
						SDL_SetWindowTitle(window, (std::string(metadata_title) + " [FROZEN]").c_str());
						break;
				}
			}
			break;
	}
}

#ifdef __EMSCRIPTEN__
void em_loop() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		handle_event(event);
	}

	// 1. Poll virtual (touch) keys from JS. (We keep the bitfield in Module._vk_bits.)
	EM_ASM({
	  if(Module.VirtualKeys){
	    var vk = Module.VirtualKeys;
	    var bits = 0;
	    if(vk['a']) bits |= 1;      // left
	    if(vk['d']) bits |= 2;      // right
	    if(vk['w']) bits |= 4;      // up
	    if(vk['s']) bits |= 8;      // down
	    if(vk['z']) bits |= 16;     // A
	    if(vk['x']) bits |= 32;     // B
	    if(vk['c']) bits |= 64;     // X
	    if(vk['v']) bits |= 128;    // Y
	    if(vk['2']) bits |= 256;    // Menu
	    if(vk['1']) bits |= 512;    // Home
	    Module._vk_bits = bits;
	  }
	});

	uint32_t vk_bits = (uint32_t)emscripten_run_script_int("Module._vk_bits || 0");

	// 2. Poll physical keyboard state directly (so we don't rely on earlier SDL_KEYDOWN ordering)
	const Uint8 *keystate = SDL_GetKeyboardState(nullptr);
	uint32_t kb_bits = 0;
	if(keystate[SDL_SCANCODE_LEFT]  || keystate[SDL_SCANCODE_A]) kb_bits |= blit::Button::DPAD_LEFT;
	if(keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) kb_bits |= blit::Button::DPAD_RIGHT;
	if(keystate[SDL_SCANCODE_UP]    || keystate[SDL_SCANCODE_W]) kb_bits |= blit::Button::DPAD_UP;
	if(keystate[SDL_SCANCODE_DOWN]  || keystate[SDL_SCANCODE_S]) kb_bits |= blit::Button::DPAD_DOWN;
	if(keystate[SDL_SCANCODE_Z]) kb_bits |= blit::Button::A;
	if(keystate[SDL_SCANCODE_X]) kb_bits |= blit::Button::B;
	if(keystate[SDL_SCANCODE_C]) kb_bits |= blit::Button::X;
	if(keystate[SDL_SCANCODE_V]) kb_bits |= blit::Button::Y;
	if(keystate[SDL_SCANCODE_2] || keystate[SDL_SCANCODE_ESCAPE]) kb_bits |= blit::Button::MENU; // ESC maps to MENU too
	if(keystate[SDL_SCANCODE_1]) kb_bits |= blit::Button::HOME;

	// 3. Combine (OR) virtual + keyboard so either source can hold a button.
	uint32_t combined = vk_bits | kb_bits;

	// 4. Apply combined mask (authoritative) each frame.
	blit_system->set_button(blit::Button::DPAD_LEFT,  (combined & blit::Button::DPAD_LEFT));
	blit_system->set_button(blit::Button::DPAD_RIGHT, (combined & blit::Button::DPAD_RIGHT));
	blit_system->set_button(blit::Button::DPAD_UP,    (combined & blit::Button::DPAD_UP));
	blit_system->set_button(blit::Button::DPAD_DOWN,  (combined & blit::Button::DPAD_DOWN));
	blit_system->set_button(blit::Button::A,          (combined & blit::Button::A));
	blit_system->set_button(blit::Button::B,          (combined & blit::Button::B));
	blit_system->set_button(blit::Button::X,          (combined & blit::Button::X));
	blit_system->set_button(blit::Button::Y,          (combined & blit::Button::Y));
	blit_system->set_button(blit::Button::MENU,       (combined & blit::Button::MENU));
	blit_system->set_button(blit::Button::HOME,       (combined & blit::Button::HOME));

	blit_multiplayer->update();
	blit_system->loop();
	blit_renderer->update(blit_system);
	blit_renderer->present();
}
#endif

int main(int argc, char *argv[]) {
  int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
  bool fullscreen = false;

  std::cout << metadata_title << " " << metadata_version << std::endl;
  std::cout << "Powered by 32Blit SDL2 runtime - github.com/32blit/32blit-sdk" << std::endl << std::endl;

	auto mp_mode = Multiplayer::Mode::Auto;
	std::string mp_address = "localhost";

	for(int i = 1; i < argc; i++) {
		std::string arg_str(argv[i]);
		if(arg_str == "--connect" && i + 1 < argc) {
			mp_mode = Multiplayer::Mode::Connect;
			mp_address = std::string(argv[i + 1]);
			i++;
		}
		else if(arg_str == "--launch_path" && i + 1 < argc) {
			launch_path = argv[++i];
		}
		else if(arg_str == "--listen")
			mp_mode = Multiplayer::Mode::Listen;
		else if(arg_str == "--position") {
			SDL_sscanf(argv[i+1], "%d,%d", &x, &y);
		}	else if(arg_str == "--size" && i + 1 < argc) {
      int w, h;
			if(SDL_sscanf(argv[i+1], "%d,%d", &w, &h) == 2) {
        if(w * y < System::max_width * System::max_height) {
          System::width = w;
          System::height = h;
        }
      }
      i++;
		} else if(arg_str == "--fullscreen")
			fullscreen = true;
    else if(arg_str == "--credits") {
			std::cout << "32Blit was made possible by:" << std::endl;
			std::cout << std::endl;
			for(auto name : contributors) {
				if(name != nullptr) {
					std::cout << " * " << name << std::endl;
				}
			}
			std::cout << std::endl;
			std::cout << "Special thanks to:" << std::endl;
			std::cout << std::endl;
			for(auto name : special_thanks) {
				if(name != nullptr) {
					std::cout << " * " << name << std::endl;
				}
			}
			std::cout << std::endl;
			SDL_Quit();
			return 0;
		}
		else if(arg_str == "--info") {
			std::cout << metadata_description << std::endl << std::endl;
			std::cout << " Category: " << metadata_category << std::endl;
			std::cout << " Author:   " << metadata_author << std::endl;
			std::cout << " URL:      " << metadata_url << std::endl << std::endl;
			SDL_Quit();
			return 0;
		}
		else if(arg_str == "--help") {
			std::cout << "Usage: " << argv[0] << " <options>" << std::endl << std::endl;
			std::cout << " --connect <addr>     -- Connect to a listening game instance." << std::endl;
			std::cout << " --listen             -- Listen for incoming connections." << std::endl;
			std::cout << " --position x,y       -- Set window position." << std::endl;
      std::cout << " --size w,h           -- Set display size. (max 320x240)" << std::endl;
			std::cout << " --launch_path <file> -- Emulates the file associations on the console." << std::endl;
			std::cout << " --credits            -- Print contributor credits and exit." << std::endl;
			std::cout << " --info               -- Print metadata info and exit." << std::endl << std::endl;
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 0;
		}
	}

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO) < 0) {
		std::cerr << "could not initialize SDL2: " << SDL_GetError() << std::endl;
		return 1;
	}

	window = SDL_CreateWindow(
		metadata_title,
		x, y,
		System::width*2, System::height*2,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
	);

	if (window == nullptr) {
		std::cerr << "could not create window: " << SDL_GetError() << std::endl;
		return 1;
	}
	SDL_SetWindowMinimumSize(window, System::width, System::height);

  blit_system = new System();
  blit_input = new Input(blit_system);
	blit_multiplayer = new Multiplayer(mp_mode, mp_address);
	blit_renderer = new Renderer(window, System::width, System::height);
	blit_audio = new Audio();

#ifdef VIDEO_CAPTURE
	blit_capture = new VideoCapture(argv[0]);
#endif

	blit_system->run();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(em_loop, 0, 1);
#else
	SDL_Event event;

	while (running && SDL_WaitEvent(&event)) {
		handle_event(event);
	}
#endif

	if (running) {
		std::cerr << "Main loop exited with error: " << SDL_GetError() << std::endl;
		running = false; // ensure timer thread quits
	}

#ifdef VIDEO_CAPTURE
	if (blit_capture->recording()) blit_capture->stop();
	delete blit_capture;
#endif

	blit_system->stop();
	delete blit_system;
  delete blit_input;
	delete blit_multiplayer;
	delete blit_renderer;

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
