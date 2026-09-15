#include "metamod_minimal.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

struct MetamodVersionInfo21
{
    int api_major;
    int api_minor;
    int pl_min;
    int pl_max;
    int source_engine;
    const char *game_dir;
};

using CreateInterfaceMMSFn = SourceMM::ISmmPlugin *(*)(const void *, const MetamodLoaderInfo *);
using SetFn = void (*)(void *);
using GetFn = void *(*)();

int main(int argc, char **argv)
{
    if (argc != 5)
        return 2;

    void *bad = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
    void *dedicated = dlopen(argv[2], RTLD_NOW | RTLD_GLOBAL);
    if (!bad || !dedicated)
        return 3;

    auto setPersonality = reinterpret_cast<SetFn>(dlsym(dedicated, "mock_set_personality"));
    auto getPersonality = reinterpret_cast<GetFn>(dlsym(dedicated, "mock_get_personality"));
    void *badPersonality = dlsym(bad, "__gxx_personality_v0");
    void *libstdcpp = dlopen("libstdc++.so.6", RTLD_NOW | RTLD_LOCAL);
    void *goodPersonality = libstdcpp ? dlsym(libstdcpp, "__gxx_personality_v0") : nullptr;
    if (!setPersonality || !getPersonality || !badPersonality || !goodPersonality)
        return 4;

    setPersonality(badPersonality);
    for (int iteration = 0; iteration < 2; ++iteration)
    {
        void *library = dlopen(argv[3], RTLD_NOW | RTLD_LOCAL);
        if (!library)
            return 5;

        auto factory = reinterpret_cast<CreateInterfaceFn>(dlsym(library, "CreateInterface"));
        auto advanced = reinterpret_cast<CreateInterfaceMMSFn>(dlsym(library, "CreateInterface_MMS"));
        if (!factory || !advanced)
        {
            fprintf(stderr, "Metamod entry point missing\n");
            return 6;
        }

        int code = -1;
        if (factory("ISmmPlugin999", &code) || code != IFACE_FAILED || factory(nullptr, nullptr))
            return 7;
        SourceMM::ISmmPlugin *plugin = static_cast<SourceMM::ISmmPlugin *>(factory("ISmmPlugin", &code));
        if (!plugin || code != IFACE_OK || plugin->GetApiVersion() != 16)
            return 8;

        int expectedApi = 16;
        MetamodLoaderInfo loader{argv[3], "addons/srcds_shutdown_fix/bin"};
        if (strcmp(argv[4], "legacy16") == 0 || strcmp(argv[4], "legacy17") == 0)
        {
            expectedApi = strcmp(argv[4], "legacy16") == 0 ? 16 : 17;
            MetamodVersionInfo version{2, 0, 5, 5, 14, expectedApi, 15, "hl2mp"};
            if (advanced(&version, &loader) != plugin)
                return 9;
        }
        else if (strcmp(argv[4], "sourcehook21") == 0)
        {
            expectedApi = 17;
            MetamodVersionInfo version{2, 1, 5, 5, 14, 17, 17, "tf2classified"};
            if (advanced(&version, &loader) != plugin)
                return 18;
        }
        else if (strcmp(argv[4], "modern18") == 0)
        {
            expectedApi = 18;
            MetamodVersionInfo21 version{2, 1, 18, 18, 15, "hl2mp"};
            if (advanced(&version, &loader) != plugin)
                return 10;
        }
        else if (strcmp(argv[4], "unsupported") == 0)
        {
            MetamodVersionInfo21 version{2, 1, 19, 19, 15, "hl2mp"};
            if (advanced(&version, &loader) || factory("ISmmPlugin", &code) || code != IFACE_FAILED)
                return 11;
            if (getPersonality() != badPersonality)
                return 12;
            dlclose(library);
            continue;
        }
        else if (strcmp(argv[4], "classic") != 0)
        {
            return 13;
        }

        if (plugin->GetApiVersion() != expectedApi || factory("ISmmPlugin", nullptr) != plugin)
            return 14;

        char error[512]{};
        if (!plugin->Load(1, nullptr, error, sizeof(error), iteration != 0))
        {
            fprintf(stderr, "Load failed: %s\n", error);
            return 15;
        }
        if (strcmp(plugin->GetVersion(), "1.3.2") != 0 || getPersonality() != goodPersonality ||
            !plugin->QueryRunning(error, sizeof(error)) || !plugin->Unload(error, sizeof(error)))
            return 16;

        if (dlclose(library) != 0 || getPersonality() != goodPersonality)
            return 17;
    }

    dlclose(libstdcpp);
    dlclose(dedicated);
    dlclose(bad);
    return 0;
}
