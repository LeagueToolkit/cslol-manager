#pragma once
#include <cinttypes>
#include <cstddef>
#include <string>

class File;
struct Link
{
    std::string path = {};
};

extern void read(File const& file, Link& link);
