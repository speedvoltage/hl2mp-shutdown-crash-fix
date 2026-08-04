#pragma once

#include <cstddef>
#include "tier1/interface.h"

#define METAMOD_PLAPI_VERSION 17

class CGlobalVars;
class ConCommandBase;

namespace SourceMM
{
using PluginId = int;

class ISmmPlugin;

class ISmmAPI
{
public:
    virtual void LogMsg(ISmmPlugin *plugin, const char *message, ...) = 0;
    virtual CreateInterfaceFn GetEngineFactory(bool synthetic = true) = 0;
    virtual CreateInterfaceFn GetPhysicsFactory(bool synthetic = true) = 0;
    virtual CreateInterfaceFn GetFileSystemFactory(bool synthetic = true) = 0;
    virtual CreateInterfaceFn GetServerFactory(bool synthetic = true) = 0;
    virtual CGlobalVars *GetCGlobals() = 0;
    virtual bool RegisterConCommandBase(ISmmPlugin *plugin, ConCommandBase *command) = 0;
    virtual void UnregisterConCommandBase(ISmmPlugin *plugin, ConCommandBase *command) = 0;
};

class ISmmPlugin
{
public:
    virtual int GetApiVersion()
    {
        return METAMOD_PLAPI_VERSION;
    }

    virtual ~ISmmPlugin() = default;

    virtual bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlength, bool late) = 0;

    virtual void AllPluginsLoaded()
    {
    }

    virtual bool QueryRunning(char *, size_t)
    {
        return true;
    }

    virtual bool Unload(char *, size_t)
    {
        return true;
    }

    virtual bool Pause(char *, size_t)
    {
        return true;
    }

    virtual bool Unpause(char *, size_t)
    {
        return true;
    }

    virtual const char *GetAuthor() = 0;
    virtual const char *GetName() = 0;
    virtual const char *GetDescription() = 0;
    virtual const char *GetURL() = 0;
    virtual const char *GetLicense() = 0;
    virtual const char *GetVersion() = 0;
    virtual const char *GetDate() = 0;
    virtual const char *GetLogTag() = 0;
};
}

struct MetamodVersionInfo
{
    int api_major;
    int api_minor;
    int sh_iface;
    int sh_impl;
    int pl_min;
    int pl_max;
    int source_engine;
    const char *game_dir;

    const char *GetGameDir() const
    {
        if (pl_max < 15)
            return nullptr;
        return game_dir;
    }
};

struct MetamodLoaderInfo
{
    const char *pl_file;
    const char *pl_path;
};
