#include "metamod_minimal.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

class MockApi final : public SourceMM::ISmmAPI
{
public:
    void LogMsg(SourceMM::ISmmPlugin *, const char *message, ...) override
    {
        char output[1024];
        va_list args;
        va_start(args, message);
        vsnprintf(output, sizeof(output), message, args);
        va_end(args);
        puts(output);
    }

    CreateInterfaceFn GetEngineFactory(bool) override { return nullptr; }
    CreateInterfaceFn GetPhysicsFactory(bool) override { return nullptr; }
    CreateInterfaceFn GetFileSystemFactory(bool) override { return nullptr; }
    CreateInterfaceFn GetServerFactory(bool) override { return nullptr; }
    CGlobalVars *GetCGlobals() override { return nullptr; }
    bool RegisterConCommandBase(SourceMM::ISmmPlugin *, ConCommandBase *) override { return true; }
    void UnregisterConCommandBase(SourceMM::ISmmPlugin *, ConCommandBase *) override {}
};

using CreateInterfaceMMSFn = SourceMM::ISmmPlugin *(*)(const MetamodVersionInfo *, const MetamodLoaderInfo *);
using SetFn = void (*)(void *);
using GetFn = void *(*)();

int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;

    void *bad = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
    void *dedicated = dlopen(argv[2], RTLD_NOW | RTLD_GLOBAL);
    void *pluginLibrary = dlopen(argv[3], RTLD_NOW | RTLD_LOCAL);

    if (!bad || !dedicated || !pluginLibrary)
    {
        fprintf(stderr, "%s\n", dlerror());
        return 3;
    }

    auto setPersonality = reinterpret_cast<SetFn>(dlsym(dedicated, "mock_set_personality"));
    auto getPersonality = reinterpret_cast<GetFn>(dlsym(dedicated, "mock_get_personality"));
    void *badPersonality = dlsym(bad, "__gxx_personality_v0");
    void *libstdcpp = dlopen("libstdc++.so.6", RTLD_NOW | RTLD_NOLOAD | RTLD_LOCAL);
    void *goodPersonality = libstdcpp ? dlsym(libstdcpp, "__gxx_personality_v0") : nullptr;

    if (!setPersonality || !getPersonality || !badPersonality || !goodPersonality)
        return 4;

    setPersonality(badPersonality);
    if (getPersonality() != badPersonality)
        return 5;

    auto createInterface = reinterpret_cast<CreateInterfaceMMSFn>(dlsym(pluginLibrary, "CreateInterface_MMS"));
    if (!createInterface)
        return 6;

    MetamodVersionInfo version{17, 0, 5, 5, 15, 17, 999, "cstrike"};
    MetamodLoaderInfo loader{"srcds_shutdown_fix", argv[3]};
    SourceMM::ISmmPlugin *plugin = createInterface(&version, &loader);
    MockApi api;
    char error[512]{};

    if (!plugin || !plugin->Load(1, &api, error, sizeof(error), false))
    {
        fprintf(stderr, "load failed: %s\n", error);
        return 7;
    }

    if (strcmp(plugin->GetAuthor(), "Peter Brev") != 0 ||
        strcmp(plugin->GetName(), "SRCDS Shutdown Fix") != 0 ||
        strcmp(plugin->GetVersion(), "1.3.0") != 0)
    {
        return 12;
    }

    if (getPersonality() != goodPersonality)
        return 8;

    if (!plugin->QueryRunning(error, sizeof(error)))
        return 9;

    if (!plugin->Unload(error, sizeof(error)))
        return 10;

    if (getPersonality() != goodPersonality)
        return 11;

    return 0;
}
