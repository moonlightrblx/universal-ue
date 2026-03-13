#pragma once
#include <cstdint>
namespace cache {
	namespace offsets {
		uintptr_t GWorld;
		uintptr_t PersistentLevel;
		uintptr_t GameState;
		uintptr_t GameInstance;
		uintptr_t LocalPlayers;
		uintptr_t PlayerArray;
		uintptr_t PawnPrivate;
	}
	uintptr_t GWorld;
	uintptr_t PersistentLevel;
	uintptr_t GameState;
	uintptr_t GameInstance;
	uintptr_t LocalPlayers;
	uintptr_t LocalPlayer;
	uintptr_t PlayerArray;
	int PlayerCount = 0;
}