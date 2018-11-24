#include "engine_race.h"

using namespace polar_race;

RetCode Engine::Open(const std::string& name, Engine** eptr)
try {
    return EngineRace::Open(name, eptr);
} catch (...) {
    return kIOError;
}

Engine::~Engine()
{
}


