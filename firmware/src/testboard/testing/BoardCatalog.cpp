// EnvNode: see repository LICENSE.
#include "TestModel.h"
#include <cstring>
namespace EnvNode { namespace Testing {
#include "WeatherDefinition.inc"
#include "MiniDefinition.inc"
const BoardDefinition& weatherBoard() { return WeatherBoard; }
const BoardDefinition& miniBoard() { return MiniBoard; }
size_t boardCount() { return 2; }
const BoardDefinition* boardAt(size_t index) {
    return index == 0 ? &WeatherBoard : index == 1 ? &MiniBoard : nullptr;
}
const BoardDefinition* findBoard(const char* id) {
    if (id) for (size_t i = 0; i < boardCount(); ++i)
        if (!strcmp(boardAt(i)->id, id)) return boardAt(i);
    return nullptr;
}
} }
