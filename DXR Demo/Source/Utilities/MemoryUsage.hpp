#pragma once
#include <Psapi.h>

class MemoryUsage
{
public:
    static void ReadRAM()
    {
        MEMORYSTATUSEX mem{};
        mem.dwLength = sizeof(MEMORYSTATUSEX);

        ::GlobalMemoryStatusEx(&mem);

        // for convertion to MB and GB
        const DWORD dwMBFactor = 0x00100000;

        PROCESS_MEMORY_COUNTERS_EX pcmex{};
        if (!::GetProcessMemoryInfo(::GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pcmex, sizeof(PROCESS_MEMORY_COUNTERS_EX)))
            return;

        if (!::GlobalMemoryStatusEx(&mem))
            return;

        MemoryInUse = static_cast<float>(pcmex.PrivateUsage) / (1024.0f * 1024.0f);
    }

    inline static float MemoryInUse{ 0.0f };
};

