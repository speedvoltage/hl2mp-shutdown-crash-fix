#include <khook.hpp>
#include <chrono>
#include <cstdio>
#include <thread>

static bool stopped;
static unsigned lateRemovals;

extern "C" void RealRemove(KHook::HookID_t, bool, void (*)(KHook::HookID_t, void*), void*)
    asm("__real__ZN5KHook10RemoveHookEjbPFvjPvES0_");
extern "C" void WrappedRemove(KHook::HookID_t, bool, void (*)(KHook::HookID_t, void*), void*)
    asm("__wrap__ZN5KHook10RemoveHookEjbPFvjPvES0_");

extern "C" void WrappedRemove(KHook::HookID_t id, bool async,
                              void (*callback)(KHook::HookID_t, void*), void* context)
{
    if (stopped)
        ++lateRemovals;
    RealRemove(id, async, callback, context);
}

class Target
{
public:
    virtual void Run() {}
};

static KHook::Return<void> Before(Target*)
{
    return {KHook::Action::Ignore};
}

int main()
{
    Target target;
    {
        KHook::Virtual<Target, void> hook(&Target::Run, &Before, nullptr);
        hook.Add(&target);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        KHook::Shutdown();
        stopped = true;
    }
    std::printf("RemoveHook calls after Shutdown: %u\n", lateRemovals);
    return lateRemovals ? 1 : 0;
}
