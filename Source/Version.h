#pragma once

// Injected by CMake from project(VERSION ...) so the editor, standalone host,
// and tooling all report the same number as the VST3 bundle metadata.
#ifndef CERBERUS_VERSION
 #define CERBERUS_VERSION "0.0.0"
#endif

namespace cerberus
{
    inline constexpr const char* version = CERBERUS_VERSION;
} // namespace cerberus
