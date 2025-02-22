#pragma once

#include <iostream>
#include <windows.h>
#include <stdexcept>

enum RePiLogLevel
{
    eINFO = 0,
    eWARNING,
    eERROR 
};

inline void RePiLog(
    RePiLogLevel Level,
    const std::string Message)
{
    switch (Level) 
    {
    case eINFO:

        std::cout << "Info: " << Message << std::endl;

        break;
    case eWARNING:

        std::cerr << "Warning: " << Message << std::endl;

        break;
    case eERROR:

        throw std::runtime_error(Message);

        break;
    }
}

inline void RePiLog(
    RePiLogLevel Level,
    const HRESULT Code)
{
    char* Msg = nullptr;

    DWORD Flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageA(Flags, nullptr, Code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&Msg, 0, nullptr);

    std::string Message("Undefined");

    if (nullptr != Msg)
    {
        Message = std::string(Msg);
        LocalFree(Msg);
    }

    RePiLog(Level, Message);
}
