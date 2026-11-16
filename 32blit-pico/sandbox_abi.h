// Numbers shared by sandbox.cpp and sandbox_gate.S (32blit-sdk #91).
#pragma once

// Function pointers in blit::APIConst, after the version and the audio channels.
#define SANDBOX_API_SLOTS 50

// `svc #255` at sandbox_return ends a call into the game.
#define SANDBOX_SVC_RETURN 255
