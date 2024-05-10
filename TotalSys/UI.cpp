#include "pch.h"
#include "UI.h"

LRESULT CALLBACK UI::HookMessages(int code, WPARAM wp, LPARAM lp) {
    if (wp == PM_REMOVE) {
        auto msg = (MSG*)lp;
        switch (msg->message) {
            case WM_USER + 222:
            {
                auto w = (UI::WorkItem*)msg->lParam;
                std::invoke(w->Callback);
                delete w;
                return 0;
            }

            case WM_USER + 223:
            {
                auto w = (UI::WorkItemParam*)msg->lParam;
                std::invoke(w->Callback, w->Result);
                delete w;
                return 0;
            }
        }
    }
    return ::CallNextHookEx(nullptr, code, wp, lp);
}


UI::WorkItem* UI::SubmitWork(std::function<void()> work, std::function<void()> callback) noexcept {
    Init();

    auto item = new WorkItem {
        work, callback, ::GetCurrentThreadId()
    };
    auto success = ::TrySubmitThreadpoolCallback([](auto, auto p) {
        auto w = (WorkItem*)p;
        w->Running = true;
        w->Work();
        ::PostThreadMessage(w->Tid, WM_USER + 222, 0, reinterpret_cast<LPARAM>(w));
        w->Running = false;
        }, item, &s_TpEnviron);
    assert(success);

    return item;
}

UI::WorkItemParam* UI::SubmitWorkWithResult(std::function<void* ()> work, std::function<void(void*)> callback) noexcept {
    Init();
    auto item = new WorkItemParam {
        work, callback, ::GetCurrentThreadId()
    };
    auto success = ::TrySubmitThreadpoolCallback([](auto, auto p) {
        auto w = (WorkItemParam*)p;
        w->Running = true;
        w->Result = w->Work();
        ::PostThreadMessage(w->Tid, WM_USER + 223, 0, reinterpret_cast<LPARAM>(w));
        w->Running = false;
        }, item, &s_TpEnviron);
    assert(success);

    return item;
}

bool UI::Init() {
    if (s_TpEnviron.Version == 0) {
        if (!s_ThreadPool.Create())
            return false;
        s_ThreadPool.SetMaxThreads(::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) * 2);
        InitializeThreadpoolEnvironment(&s_TpEnviron);
        ::SetThreadpoolCallbackPool(&s_TpEnviron, s_ThreadPool);
    }
    if (!s_Hooked) {
        ::SetWindowsHookEx(WH_GETMESSAGE, HookMessages, nullptr, ::GetCurrentThreadId());
        s_Hooked = true;
    }
    return true;
}


