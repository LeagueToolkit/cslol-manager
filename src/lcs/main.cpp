#include <iostream>
#include <chrono>
#include "wadindex.hpp"
#include "modindex.hpp"
#include "wadmergequeue.hpp"

using namespace std;

using namespace LCS;
int main() {
    ProgressMulti dummyProgress;
    ConflictHandler dummyConflictHandler;

    WadIndex wadIndex { "C:/Riot Games/League of Legends/Game" };
    ModIndex modIndex{"installed"};
    try {
        modIndex.install_from_zip("PROJECT Evelynn  - 1.0 (by Yoru).zip", dummyProgress);
    } catch(std::runtime_error const&) {}

    WadMergeQueue mergeQueue {"overlay", wadIndex};
    mergeQueue.addMod(modIndex.mods().begin()->second.get(), dummyConflictHandler);
    mergeQueue.write(dummyProgress);
    return 0;
}
