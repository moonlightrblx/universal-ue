#include <iostream>
#include <thread>
#include "sdk/offsets/reader.h"
#include "sdk/game/cache.h"
#include "memory/driver.h"

void cache_thread() {
    for(;;) {
        cache::GWorld = read<uintptr_t>(base_addr + cache::offsets::GWorld);

        cache::PersistentLevel = read<uintptr_t>(cache::GWorld + cache::offsets::PersistentLevel);
        if (!cache::PersistentLevel) continue;

        cache::GameState = read<uintptr_t>(cache::GWorld + cache::offsets::GameState);
        if (!cache::GameState) continue;

        cache::GameInstance = read<uintptr_t>(cache::GWorld + cache::offsets::GameInstance);
        if (!cache::GameInstance) continue;

        cache::LocalPlayers = read<uintptr_t>(cache::GameInstance + 0x0038); // afaik this doesnt change
        if (!cache::LocalPlayers) continue;
       

        cache::LocalPlayer = read<uintptr_t>(cache::LocalPlayers);
        if (!cache::LocalPlayer) continue;


        cache::PlayerArray =read<uintptr_t>(cache::GameState + cache::offsets::PlayerArray); // 0x0348
        if (!cache::PlayerArray) continue;

        cache::PlayerCount = read<int>(cache::GameState + cache::offsets::PlayerArray + sizeof(uintptr_t));

        for (int i = 0; i < cache::PlayerCount; i++)
        {
            uintptr_t PlayerState = read<uintptr_t>(cache::PlayerArray + (i * sizeof(uintptr_t)));
            if (!PlayerState) continue;

            uintptr_t PawnPrivate = read<uintptr_t>(PlayerState + cache::offsets::PawnPrivate);
            if (!PawnPrivate) continue;
        
        }
        // std::cout << "finished all caching work. offsets are correct\n";
        Sleep(1000);     
	}
}
int main()
{
    offsets::globals::load("globals.json");
	
    offsets::sdk::load("sdk_data.json");

    driver::process_id = driver::find_process(L"FortniteClient-Win64-Shipping.exe");
    
    driver::find_driver();
    driver::fetch_cr3();

    base_addr = driver::find_image();
	
    std::cout << "Driver connected successfully.\n";
    std::cout << "PID: " << driver::process_id << "\n";
	std::cout << "Base Address -> 0x" << std::hex << base_addr << "\n";

    cache::offsets::GWorld = offsets::globals::data.Bases["UWorld"];
    cache::offsets::PersistentLevel = offsets::sdk::get_member("UWorld", "PersistentLevel").value().Offset;
    cache::offsets::GameState = offsets::sdk::get_member("UWorld", "GameState").value().Offset;
    cache::offsets::GameInstance = offsets::sdk::get_member("UWorld", "OwningGameInstance").value().Offset;
    cache::offsets::PlayerArray = offsets::sdk::get_member("AGameStateBase", "PlayerArray").value().Offset; // Class Engine.GameStateBase
    cache::offsets::PawnPrivate = offsets::sdk::get_member("APlayerState", "PawnPrivate").value().Offset;

  /*  auto it = offsets::sdk::Classes.find("AGameStateBase");
    if (it != offsets::sdk::Classes.end()) {
        std::cout << "Members of AGameStateBase:" << std::endl;
        for (auto& m : it->second.Members)
            std::cout << m.Name << " at offset 0x" << std::hex << m.Offset << std::endl;
    }
    */

        // afaik 0x0038 is always the offset for LocalPlayers and it doesnt change <3 

	std::cout << "PersistentLevel -> 0x" << cache::offsets::PersistentLevel << "\n";
	std::cout << "LocalPlayers -> 0x0038" << "\n";
	std::cout << "GameState -> 0x" << cache::offsets::GameState << "\n";
	std::cout << "GameInstance -> 0x" << cache::offsets::GameInstance << "\n";
	std::cout << "PlayerArray -> 0x" << cache::offsets::PlayerArray << "\n";

    std::thread(cache_thread).detach();
    // yay i dont have to do stupid driver decryption shit :D
        
    return 0;
}

