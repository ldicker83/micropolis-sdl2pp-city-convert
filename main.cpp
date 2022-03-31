// This file is part of Micropolis-SDL2PP
// Micropolis-SDL2PP is based on Micropolis
//
// Copyright © 2022 Leeor Dicker
//
// Portions Copyright © 1989-2007 Electronic Arts Inc.
//
// Micropolis-SDL2PP is free software; you can redistribute it and/or modify
// it under the terms of the GNU GPLv3, with additional terms. See the README
// file, included in this distribution, for details.

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <algorithm>
#include <string>

#include <cstdint>
#include <stdio.h>


#define IS_INTEL 1  /// \fixme Find a better way to determine this

#ifdef IS_INTEL

/**
 * Swap upper and lower byte of all shorts in the array.
 */
static void swap_shorts(short* buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        *buf = ((*buf & 0xFF) << 8) | ((*buf & 0xFF00) >> 8);
        buf++;
    }
}


/**
 * Swap upper and lower words of all longs in the array.
 */
static void half_swap_longs(int32_t* buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        int32_t l = *buf;
        *buf = ((l & 0x0000ffff) << 16) | ((l & 0xffff0000) >> 16);
        buf++;
    }
}

#define SWAP_SHORTS(buf, len)        swap_shorts(buf, len)
#define HALF_SWAP_LONGS(buf, len)    half_swap_longs(buf, len)

#else

#define SWAP_SHORTS(buf, len)
#define HALF_SWAP_LONGS(buf, len)

#endif

namespace
{
    constexpr auto WORLD_W = 120;
    constexpr auto WORLD_H = 100;
};


namespace MicropolisSDL2PPMap
{
    constexpr auto HistoryLength = 120;
    constexpr auto MISCHISTLEN = 240;

    using GraphHistory = std::array<int, HistoryLength>;

    GraphHistory ResHis{};
    GraphHistory ComHis{};
    GraphHistory IndHis{};

    GraphHistory MoneyHis{};
    GraphHistory PollutionHis{};
    GraphHistory CrimeHis{};
    GraphHistory MiscHis{};

    GraphHistory ResHis120Years{};
    GraphHistory ComHis120Years{};
    GraphHistory IndHis120Years{};

    GraphHistory MoneyHis120Years{};
    GraphHistory PollutionHis120Years{};
    GraphHistory CrimeHis120Years{};
    GraphHistory MiscHis120Years{};

    std::array<std::array<int, WORLD_H>, WORLD_W> Map;
}


namespace ShortMap
{
    constexpr auto HISTORY_LENGTH = 480;
    constexpr auto MISC_HISTORY_LENGTH = 240;

    float roadPercent{};
    float policePercent{};
    float firePercent{};

    int32_t roadValue{};
    int32_t policeValue{};
    int32_t fireValue{};

    int32_t cityTime{};
    int32_t cityMonth{};
    int32_t cityYear{};
    int32_t totalFunds{};

    short cityTax{};
    short simSpeed{};
    short userSound{};

    short autoBulldoze{};
    short autoBudget{};
    short autoGoto{};

    unsigned short* mapBase{ nullptr };
    short* resHist{ nullptr };
    short* comHist{ nullptr };
    short* indHist{ nullptr };
    short* moneyHist{ nullptr };
    short* pollutionHist{ nullptr };
    short* crimeHist{ nullptr };
    short* miscHist{ nullptr };

    unsigned short* map[WORLD_W]{ nullptr };
};


static bool load_short(short* buf, int len, FILE* f)
{
    size_t result = fread(buf, sizeof(short), len, f);

    if ((int)result != len)
    {
        return false;
    }

    SWAP_SHORTS(buf, len);

    return true;
}


bool loadCityFile(const std::string& filename)
{
    FILE* f{ nullptr };
    int32_t size{};

    errno_t err;
    if ((err = fopen_s(&f, filename.c_str(), "rb")) != 0)
    {
        std::cout << "Cannot open file '" << filename << "'" << std::endl;
    }

    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    bool result =

        load_short(ShortMap::resHist, ShortMap::HISTORY_LENGTH / sizeof(short), f) &&
        load_short(ShortMap::comHist, ShortMap::HISTORY_LENGTH / sizeof(short), f) &&
        load_short(ShortMap::indHist, ShortMap::HISTORY_LENGTH / sizeof(short), f) &&
        load_short(ShortMap::crimeHist, ShortMap::HISTORY_LENGTH / sizeof(short), f) &&
        load_short(ShortMap::pollutionHist, ShortMap::HISTORY_LENGTH / sizeof(short), f) &&
        load_short(ShortMap::moneyHist, ShortMap::HISTORY_LENGTH / sizeof(short), f) &&
        load_short(ShortMap::miscHist, ShortMap::MISC_HISTORY_LENGTH / sizeof(short), f) &&
        load_short(((short*)&ShortMap::map[0][0]), WORLD_W * WORLD_H, f);

    fclose(f);

    return result;
}


bool loadFile(const std::string& filename)
{
    if (!loadCityFile(filename))
    {
        return false;
    }
    
    int32_t n = *(int32_t*)(ShortMap::miscHist + 50);
    HALF_SWAP_LONGS(&n, 1);
    ShortMap::totalFunds = n;

    n = *(int32_t*)(ShortMap::miscHist + 8);
    HALF_SWAP_LONGS(&n, 1);
    ShortMap::cityTime = n;

    n = *(reinterpret_cast<int*>(ShortMap::miscHist + 58));
    HALF_SWAP_LONGS(&n, 1);
    ShortMap::policePercent = static_cast<float>(n / 65536.0f);

    n = *(reinterpret_cast<int*>(ShortMap::miscHist + 60));
    HALF_SWAP_LONGS(&n, 1);
    ShortMap::firePercent = static_cast<float>(n / 65536.0f);

    n = *(reinterpret_cast<int*>(ShortMap::miscHist + 62));
    HALF_SWAP_LONGS(&n, 1);
    ShortMap::roadPercent = static_cast<float>(n / 65536.0f);

    ShortMap::cityTime = std::max(0, ShortMap::cityTime);

    ShortMap::cityTax = ShortMap::miscHist[56];
    if (ShortMap::cityTax > 20 || ShortMap::cityTax < 0)
    {
        ShortMap::cityTax = 7;
    }

    ShortMap::simSpeed = ShortMap::miscHist[57];
    if (ShortMap::simSpeed < 0 || ShortMap::simSpeed > 3)
    {
        ShortMap::simSpeed = 3;
    }

    ShortMap::userSound = ShortMap::miscHist[55] != 0;
    
    ShortMap::autoBulldoze = ShortMap::miscHist[52] != 0;
    ShortMap::autoBudget = ShortMap::miscHist[53] != 0;
    ShortMap::autoGoto = ShortMap::miscHist[54] != 0;

    return true;
}


bool saveFile(const std::string& filename)
{
    std::ofstream outfile(filename, std::ofstream::binary);
    if (outfile.fail())
    {
        outfile.close();
        return false;
    }

    MicropolisSDL2PPMap::MiscHis[8] = ShortMap::cityTime;
    MicropolisSDL2PPMap::MiscHis[50] = ShortMap::totalFunds;

    MicropolisSDL2PPMap::MiscHis[52] = ShortMap::autoBulldoze;
    MicropolisSDL2PPMap::MiscHis[53] = ShortMap::autoBudget;
    MicropolisSDL2PPMap::MiscHis[54] = ShortMap::autoGoto;
    MicropolisSDL2PPMap::MiscHis[55] = ShortMap::userSound;
    MicropolisSDL2PPMap::MiscHis[57] = ShortMap::simSpeed;
    MicropolisSDL2PPMap::MiscHis[56] = ShortMap::cityTax;

    MicropolisSDL2PPMap::MiscHis[58] = 100;
    MicropolisSDL2PPMap::MiscHis[60] = 100;
    MicropolisSDL2PPMap::MiscHis[62] = 100;

    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::ResHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::ComHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::IndHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::CrimeHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::PollutionHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::MoneyHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::MiscHis.data()), sizeof(MicropolisSDL2PPMap::GraphHistory));
    outfile.write(reinterpret_cast<char*>(MicropolisSDL2PPMap::Map.data()), sizeof(MicropolisSDL2PPMap::Map));

    outfile.close();
    return true;
}


void initMapArrays()
{
    if (!ShortMap::mapBase)
    {
        ShortMap::mapBase = (unsigned short*)malloc(sizeof(unsigned short) * WORLD_W * WORLD_H);
    }

    for (int i = 0; i < WORLD_W; i++)
    {
        ShortMap::map[i] = (unsigned short*)(ShortMap::mapBase + (i * WORLD_H));
    }

    ShortMap::resHist = (short*)malloc(ShortMap::HISTORY_LENGTH);
    ShortMap::comHist = (short*)malloc(ShortMap::HISTORY_LENGTH);
    ShortMap::indHist = (short*)malloc(ShortMap::HISTORY_LENGTH);
    ShortMap::moneyHist = (short*)malloc(ShortMap::HISTORY_LENGTH);
    ShortMap::pollutionHist = (short*)malloc(ShortMap::HISTORY_LENGTH);
    ShortMap::crimeHist = (short*)malloc(ShortMap::HISTORY_LENGTH);
    ShortMap::miscHist = (short*)malloc(ShortMap::MISC_HISTORY_LENGTH);
}


void deinitMapArrays()
{
    free(ShortMap::mapBase);
    free(ShortMap::resHist);
    free(ShortMap::comHist);
    free(ShortMap::indHist);
    free(ShortMap::moneyHist);
    free(ShortMap::pollutionHist);
    free(ShortMap::crimeHist);
    free(ShortMap::miscHist);
}


void copyArrays()
{
    for (size_t row = 0; row < WORLD_W; ++row)
    {
        for (size_t col = 0; col < WORLD_H; ++col)
        {
            MicropolisSDL2PPMap::Map[row][col] = ShortMap::map[row][col];
        }
    }
}


int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        std::cout << "Must be called with one argument specifying a file to be opened." << std::endl;
        std::cout << "EXAMPLE: simcity2micropolis-sdl2pp about.cty" << std::endl;

        return 0;
    }

    const std::string filename{ argv[1] };

    if (!std::filesystem::exists(argv[1]))
    {
        std::cout << "Unable to find '" << filename << "'." << std::endl;
        return 0;
    }

    initMapArrays();
    
    if (!loadFile(filename))
    {
        std::cout << "Unable to load '" << filename << "'." << std::endl;
        return 0;
    }

    copyArrays();

    saveFile(filename);

    std::cout << std::endl << std::endl;

    deinitMapArrays();
}
