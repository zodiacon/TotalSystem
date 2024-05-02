#include "pch.h"
#include "UI.h"

LRESULT CALLBACK UI::HookMessages(int code, WPARAM wp, LPARAM lp) {
    if (wp == PM_REMOVE) {
        auto msg = (MSG*)lp;
        if (msg->message == WM_USER + 222) {
            auto w = (UI::WorkItem*)msg->lParam;
            std::invoke(w->Callback);
            delete w;
            return 0;
        }
    }
    return ::CallNextHookEx(nullptr, code, wp, lp);
}


UI::WorkItem* UI::SubmitWork(std::function<void()> work, std::function<void()> callback) {
    thread_local static bool hooked = false;
    if (!hooked) {
        ::SetWindowsHookEx(WH_GETMESSAGE, HookMessages, nullptr, ::GetCurrentThreadId());
        hooked = true;
    }

    auto item = new WorkItem {
        work, callback, ::GetCurrentThreadId()
    };
    ::TrySubmitThreadpoolCallback([](auto, auto p) {
        auto w = (WorkItem*)p;
        w->Work();
        ::PostThreadMessage(w->Tid, WM_USER + 222, 0, reinterpret_cast<LPARAM>(w));
        }, item, nullptr);

    return item;
}

