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
                auto w = (UI::WorkItemParam*)msg->lParam;
                std::invoke(w->Callback, w->Result);
                delete w;
                return 0;
        }
    }
    return ::CallNextHookEx(nullptr, code, wp, lp);
}


UI::WorkItem* UI::SubmitWork(std::function<void()> work, std::function<void()> callback) {
    if (!s_Hooked) {
        ::SetWindowsHookEx(WH_GETMESSAGE, HookMessages, nullptr, ::GetCurrentThreadId());
        s_Hooked = true;
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

UI::WorkItemParam* UI::SubmitWorkWithResult(std::function<void* ()> work, std::function<void(void*)> callback) {
    if (!s_Hooked) {
        ::SetWindowsHookEx(WH_GETMESSAGE, HookMessages, nullptr, ::GetCurrentThreadId());
        s_Hooked = true;
    }

    auto item = new WorkItemParam {
        work, callback, ::GetCurrentThreadId()
    };
    ::TrySubmitThreadpoolCallback([](auto, auto p) {
        auto w = (WorkItemParam*)p;
        w->Result = w->Work();
        ::PostThreadMessage(w->Tid, WM_USER + 223, 0, reinterpret_cast<LPARAM>(w));
        }, item, nullptr);

    return item;
}


