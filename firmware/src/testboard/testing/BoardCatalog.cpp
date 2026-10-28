// EnvNode: see repository LICENSE.
#include "TestModel.h"
#include <cstring>
namespace EnvNode { namespace Testing {
#include "WeatherDefinition.inc"
#include "MiniDefinition.inc"
#include "Weather03Definition.inc"
#include "Mini09Definition.inc"
const BoardDefinition& weatherBoard() { return WeatherBoard; }
const BoardDefinition& miniBoard() { return MiniBoard; }
size_t boardCount() { return 4; }
const BoardDefinition* boardAt(size_t index) {
    const BoardDefinition* boards[] = {&WeatherBoard, &MiniBoard, &Weather03Board, &Mini09Board};
    return index < boardCount() ? boards[index] : nullptr;
}
const BoardDefinition* findBoard(const char* id) {
    if (id) for (size_t i = 0; i < boardCount(); ++i)
        if (!strcmp(boardAt(i)->id, id)) return boardAt(i);
    return nullptr;
}
} }
