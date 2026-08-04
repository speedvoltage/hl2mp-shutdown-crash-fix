#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if !defined(__linux__)
#error SRCDS Shutdown Fix supports Linux only.
#endif

#if !defined(__x86_64__) || !defined(PLATFORM_64BITS) || !defined(X64BITS)
#error SRCDS Shutdown Fix supports x86-64 only.
#endif

#include "metamod_minimal.h"

#include <dlfcn.h>
#include <elf.h>
#include <limits.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void *CompatDlopen(const char *path, int mode) noexcept;
extern "C" int CompatDlclose(void *handle) noexcept;
extern "C" void *CompatDlsym(void *handle, const char *name) noexcept;
extern "C" int CompatDladdr(const void *address, Dl_info *info) noexcept;

__asm__(".symver CompatDlopen,dlopen@GLIBC_2.2.5");
__asm__(".symver CompatDlclose,dlclose@GLIBC_2.2.5");
__asm__(".symver CompatDlsym,dlsym@GLIBC_2.2.5");
__asm__(".symver CompatDladdr,dladdr@GLIBC_2.2.5");

extern "C" void *dlopen(const char *path, int mode) noexcept
{
    return CompatDlopen(path, mode);
}

extern "C" int dlclose(void *handle) noexcept
{
    return CompatDlclose(handle);
}

extern "C" void *dlsym(void *handle, const char *name) noexcept
{
    return CompatDlsym(handle, name);
}

extern "C" int dladdr(const void *address, Dl_info *info) noexcept
{
    return CompatDladdr(address, info);
}

void *operator new(size_t size)
{
    void *memory = malloc(size ? size : 1);
    if (!memory)
        abort();
    return memory;
}

void *operator new[](size_t size)
{
    void *memory = malloc(size ? size : 1);
    if (!memory)
        abort();
    return memory;
}

void operator delete(void *memory) noexcept
{
    free(memory);
}

void operator delete[](void *memory) noexcept
{
    free(memory);
}

void operator delete(void *memory, size_t) noexcept
{
    free(memory);
}

void operator delete[](void *memory, size_t) noexcept
{
    free(memory);
}

namespace
{
constexpr const char *kVersion = "1.3.0";
constexpr const char *kDedicatedModuleName = "dedicated_srv.so";
constexpr const char *kPersonalitySymbol = "__gxx_personality_v0";
constexpr const char *kBadProviderName = "libsteam_api.so";
constexpr const char *kGoodProviderName = "libstdc++.so.6";

struct LoadedModule
{
    char path[PATH_MAX];
    uintptr_t base;
    uintptr_t writableStart;
    uintptr_t writableEnd;
};

int g_PluginApiVersion = METAMOD_PLAPI_VERSION;
void **g_PersonalitySlot = nullptr;
void *g_OriginalPersonality = nullptr;
void *g_CorrectPersonality = nullptr;
bool g_Patched = false;

void CopyError(char *error, size_t maxlength, const char *message)
{
    if (!error || maxlength == 0)
        return;

    snprintf(error, maxlength, "%s", message ? message : "Unknown error");
}

void PrintMeta(const char *message)
{
    fprintf(stdout, "[META] %s\n", message);
    fflush(stdout);
}

bool EndsWith(const char *value, const char *suffix)
{
    if (!value || !suffix)
        return false;

    const size_t valueLength = strlen(value);
    const size_t suffixLength = strlen(suffix);

    return suffixLength <= valueLength && memcmp(value + valueLength - suffixLength, suffix, suffixLength) == 0;
}

struct ModuleSearch
{
    const char *suffix;
    LoadedModule *module;
};

int FindModuleCallback(dl_phdr_info *info, size_t, void *data)
{
    auto *search = static_cast<ModuleSearch *>(data);

    if (!info || !search || !search->module || !info->dlpi_name || !info->dlpi_name[0])
        return 0;

    if (!EndsWith(info->dlpi_name, search->suffix))
        return 0;

    LoadedModule result{};
    snprintf(result.path, sizeof(result.path), "%s", info->dlpi_name);
    result.base = static_cast<uintptr_t>(info->dlpi_addr);

    for (ElfW(Half) index = 0; index < info->dlpi_phnum; ++index)
    {
        const ElfW(Phdr) &header = info->dlpi_phdr[index];

        if (header.p_type != PT_LOAD || (header.p_flags & PF_W) == 0)
            continue;

        const uintptr_t start = result.base + static_cast<uintptr_t>(header.p_vaddr);
        const uintptr_t end = start + static_cast<uintptr_t>(header.p_memsz);

        if (result.writableStart == 0 || start < result.writableStart)
            result.writableStart = start;
        if (end > result.writableEnd)
            result.writableEnd = end;
    }

    *search->module = result;
    return 1;
}

bool FindModule(const char *suffix, LoadedModule &module)
{
    module = LoadedModule{};
    ModuleSearch search{suffix, &module};
    dl_iterate_phdr(FindModuleCallback, &search);
    return module.path[0] != '\0' && module.base != 0 && module.writableStart != 0 && module.writableEnd > module.writableStart;
}

bool ReadSpan(FILE *file, uint64_t offset, size_t size, void *output)
{
    if (!file || !output || size == 0 || offset > static_cast<uint64_t>(INT64_MAX))
        return false;

    if (fseeko(file, static_cast<off_t>(offset), SEEK_SET) != 0)
        return false;

    return fread(output, 1, size, file) == size;
}

void **ResolvePersonalityRelocation(const LoadedModule &module)
{
    FILE *file = fopen(module.path, "rb");
    if (!file)
        return nullptr;

    Elf64_Ehdr header{};
    if (fread(&header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header.e_ident, ELFMAG, SELFMAG) != 0 ||
        header.e_ident[EI_CLASS] != ELFCLASS64 ||
        header.e_ident[EI_DATA] != ELFDATA2LSB ||
        header.e_machine != EM_X86_64 ||
        header.e_shoff == 0 ||
        header.e_shnum == 0 ||
        header.e_shentsize != sizeof(Elf64_Shdr))
    {
        fclose(file);
        return nullptr;
    }

    const size_t sectionBytes = static_cast<size_t>(header.e_shnum) * sizeof(Elf64_Shdr);
    auto *sections = static_cast<Elf64_Shdr *>(malloc(sectionBytes));

    if (!sections || !ReadSpan(file, header.e_shoff, sectionBytes, sections))
    {
        free(sections);
        fclose(file);
        return nullptr;
    }

    void **result = nullptr;

    for (Elf64_Half index = 0; index < header.e_shnum && !result; ++index)
    {
        const Elf64_Shdr &relocationSection = sections[index];

        if (relocationSection.sh_type != SHT_RELA ||
            relocationSection.sh_entsize != sizeof(Elf64_Rela) ||
            relocationSection.sh_size == 0 ||
            relocationSection.sh_link >= header.e_shnum)
        {
            continue;
        }

        const Elf64_Shdr &symbolSection = sections[relocationSection.sh_link];
        if (symbolSection.sh_type != SHT_DYNSYM ||
            symbolSection.sh_entsize != sizeof(Elf64_Sym) ||
            symbolSection.sh_link >= header.e_shnum)
        {
            continue;
        }

        const Elf64_Shdr &stringSection = sections[symbolSection.sh_link];
        auto *relocations = static_cast<Elf64_Rela *>(malloc(static_cast<size_t>(relocationSection.sh_size)));
        auto *symbols = static_cast<Elf64_Sym *>(malloc(static_cast<size_t>(symbolSection.sh_size)));
        auto *strings = static_cast<char *>(malloc(static_cast<size_t>(stringSection.sh_size)));

        if (!relocations || !symbols || !strings)
        {
            free(relocations);
            free(symbols);
            free(strings);
            continue;
        }

        const bool readOk =
            ReadSpan(file, relocationSection.sh_offset, static_cast<size_t>(relocationSection.sh_size), relocations) &&
            ReadSpan(file, symbolSection.sh_offset, static_cast<size_t>(symbolSection.sh_size), symbols) &&
            ReadSpan(file, stringSection.sh_offset, static_cast<size_t>(stringSection.sh_size), strings);

        if (readOk)
        {
            const size_t relocationCount = static_cast<size_t>(relocationSection.sh_size / sizeof(Elf64_Rela));
            const size_t symbolCount = static_cast<size_t>(symbolSection.sh_size / sizeof(Elf64_Sym));
            const size_t stringSize = static_cast<size_t>(stringSection.sh_size);

            for (size_t relocationIndex = 0; relocationIndex < relocationCount; ++relocationIndex)
            {
                const Elf64_Rela &relocation = relocations[relocationIndex];
                const size_t symbolIndex = static_cast<size_t>(ELF64_R_SYM(relocation.r_info));

                if (ELF64_R_TYPE(relocation.r_info) != R_X86_64_64 || relocation.r_addend != 0 || symbolIndex >= symbolCount)
                    continue;

                const Elf64_Sym &symbol = symbols[symbolIndex];
                if (symbol.st_name == 0 || symbol.st_name >= stringSize)
                    continue;

                if (strcmp(strings + symbol.st_name, kPersonalitySymbol) != 0)
                    continue;

                const uintptr_t slotAddress = module.base + static_cast<uintptr_t>(relocation.r_offset);
                if (slotAddress < module.writableStart || slotAddress + sizeof(void *) > module.writableEnd)
                    continue;

                result = reinterpret_cast<void **>(slotAddress);
                break;
            }
        }

        free(relocations);
        free(symbols);
        free(strings);
    }

    free(sections);
    fclose(file);
    return result;
}

const char *ProviderName(void *address, char *buffer, size_t length)
{
    Dl_info info{};
    if (!address || dladdr(address, &info) == 0 || !info.dli_fname || !info.dli_fname[0])
    {
        snprintf(buffer, length, "%s", "unknown");
        return buffer;
    }

    const char *slash = strrchr(info.dli_fname, '/');
    snprintf(buffer, length, "%s", slash ? slash + 1 : info.dli_fname);
    return buffer;
}

bool ProviderMatches(const char *provider, const char *soname)
{
    if (!provider || !soname)
        return false;

    const size_t length = strlen(soname);
    return strcmp(provider, soname) == 0 || (strncmp(provider, soname, length) == 0 && provider[length] == '.');
}

bool ApplyPersonalityFix(char *error, size_t maxlength)
{
    LoadedModule dedicated{};
    if (!FindModule(kDedicatedModuleName, dedicated))
    {
        CopyError(error, maxlength, "Could not locate loaded dedicated_srv.so");
        return false;
    }

    g_PersonalitySlot = ResolvePersonalityRelocation(dedicated);
    if (!g_PersonalitySlot)
    {
        CopyError(error, maxlength, "Could not locate dedicated_srv.so __gxx_personality_v0 relocation");
        return false;
    }

    void *libstdcpp = dlopen(kGoodProviderName, RTLD_NOW | RTLD_NOLOAD | RTLD_LOCAL);
    if (!libstdcpp)
        libstdcpp = dlopen(kGoodProviderName, RTLD_NOW | RTLD_LOCAL);

    if (!libstdcpp)
    {
        CopyError(error, maxlength, "Could not open libstdc++.so.6");
        return false;
    }

    g_CorrectPersonality = dlsym(libstdcpp, kPersonalitySymbol);
    dlclose(libstdcpp);

    if (!g_CorrectPersonality)
    {
        CopyError(error, maxlength, "Could not resolve libstdc++ __gxx_personality_v0");
        return false;
    }

    char correctProvider[PATH_MAX];
    ProviderName(g_CorrectPersonality, correctProvider, sizeof(correctProvider));
    if (!ProviderMatches(correctProvider, kGoodProviderName))
    {
        CopyError(error, maxlength, "Resolved personality routine is not provided by libstdc++.so.6");
        return false;
    }

    g_OriginalPersonality = __atomic_load_n(g_PersonalitySlot, __ATOMIC_ACQUIRE);
    char originalProvider[PATH_MAX];
    ProviderName(g_OriginalPersonality, originalProvider, sizeof(originalProvider));

    if (g_OriginalPersonality == g_CorrectPersonality || ProviderMatches(originalProvider, kGoodProviderName))
    {
        g_Patched = true;
        return true;
    }

    if (!ProviderMatches(originalProvider, kBadProviderName))
    {
        char message[512];
        snprintf(message, sizeof(message), "Refusing unexpected dedicated C++ personality provider: %.400s", originalProvider);
        CopyError(error, maxlength, message);
        return false;
    }

    __atomic_store_n(g_PersonalitySlot, g_CorrectPersonality, __ATOMIC_RELEASE);
    void *verified = __atomic_load_n(g_PersonalitySlot, __ATOMIC_ACQUIRE);

    if (verified != g_CorrectPersonality)
    {
        CopyError(error, maxlength, "Failed to update dedicated C++ personality binding");
        return false;
    }

    g_Patched = true;
    return true;
}

class ShutdownFixPlugin final : public SourceMM::ISmmPlugin
{
public:
    int GetApiVersion() override
    {
        return g_PluginApiVersion;
    }

    bool Load(SourceMM::PluginId, SourceMM::ISmmAPI *, char *error, size_t maxlength, bool) override
    {
        if (!ApplyPersonalityFix(error, maxlength))
            return false;

        PrintMeta("Loaded SRCDS Shutdown Fix");
        return true;
    }

    bool QueryRunning(char *error, size_t maxlen) override
    {
        if (g_Patched && g_PersonalitySlot && __atomic_load_n(g_PersonalitySlot, __ATOMIC_ACQUIRE) == g_CorrectPersonality)
            return true;

        CopyError(error, maxlen, "Dedicated C++ personality binding is not corrected");
        return false;
    }

    bool Unload(char *, size_t) override
    {
        PrintMeta("SRCDS Shutdown Fix unloaded, but retaining shutdown fix!");
        return true;
    }

    bool Pause(char *error, size_t maxlen) override
    {
        CopyError(error, maxlen, "Pause is unsupported");
        return false;
    }

    bool Unpause(char *, size_t) override
    {
        return true;
    }

    const char *GetAuthor() override
    {
        return "Peter Brev";
    }

    const char *GetName() override
    {
        return "SRCDS Shutdown Fix";
    }

    const char *GetDescription() override
    {
        return "Corrects the Linux x64 dedicated C++ personality binding used by pthread cancellation";
    }

    const char *GetURL() override
    {
        return "";
    }

    const char *GetLicense() override
    {
        return "MIT";
    }

    const char *GetVersion() override
    {
        return kVersion;
    }

    const char *GetDate() override
    {
        return __DATE__;
    }

    const char *GetLogTag() override
    {
        return "SRCDS-SHUTDOWN";
    }
};

ShutdownFixPlugin g_Plugin;
}

extern "C" __attribute__((visibility("default"))) SourceMM::ISmmPlugin *CreateInterface_MMS(
    const MetamodVersionInfo *version,
    const MetamodLoaderInfo *)
{
    if (!version)
        return nullptr;

    if (version->pl_max < 15 || version->pl_min > METAMOD_PLAPI_VERSION)
        return nullptr;

    g_PluginApiVersion = version->pl_max < METAMOD_PLAPI_VERSION ? version->pl_max : METAMOD_PLAPI_VERSION;

    return &g_Plugin;
}

extern "C" __attribute__((visibility("default"))) void UnloadInterface_MMS()
{
}
