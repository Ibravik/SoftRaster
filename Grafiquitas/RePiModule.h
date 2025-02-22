#pragma once

#include <functional>

#include "RePiHelpers.h"

template <class T, typename Callback, typename... Args>
class RePiModule
{
public:
    static T& Instance()
    {
        if (IsShutDown())
        {
            RePiLog(RePiLogLevel::eERROR, "The module has not been started yet");
        }

        return *_Instance();
    }

    static bool IsReady()
    {
        return !IsShutDown();
    }

    static bool StartUp(
        Callback&& callback,
        Args&& ...args)
    {
        if (!IsShutDown())
        {
            return false;
        }

        _Instance() = new T(std::forward<Args>(args)...);

        IsShutDown() = false;

        if (callback)
        {
            callback();
        }

        return true;
    }

    static void ShutDown()
    {
        if (IsShutDown())
        {
            RePiLog(RePiLogLevel::eERROR, "The module has already been shut down");
        }

        IsShutDown() = true;
        delete(_Instance());
    }

protected:
    RePiModule()
    {
    }

    virtual ~RePiModule()
    {
        if (!IsShutDown())
        {
            RePiLog(RePiLogLevel::eERROR, "The module did not shut down properly");
        }

        _Instance() = nullptr;
        IsShutDown() = true;
    }

    static T*& _Instance()
    {
        static T* inst = nullptr;

        return inst;
    }

    static bool& IsShutDown()
    {
        static bool inst = true;

        return inst;
    }
};