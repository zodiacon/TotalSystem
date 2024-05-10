#pragma once

#include <functional>
#include <WinLowLevel.h>

struct UI abstract final {
public:
	struct WorkItem {
		friend struct UI;

		std::function<void()> Work;
		std::function<void()> Callback;
		uint32_t Tid;
		std::atomic<bool> Running{ false };
	};

	struct WorkItemParam {
		friend struct UI;

		std::function<void*()> Work;
		std::function<void(void*)> Callback;
		uint32_t Tid;
		void* Result{ nullptr };
		std::atomic<bool> Running{ false };
	};

	static WorkItem* SubmitWork(std::function<void()> work, std::function<void()> callback) noexcept;
	static WorkItemParam* SubmitWorkWithResult(std::function<void* ()> work, std::function<void(void*)> callback) noexcept;
	static bool Init();

private:
	static LRESULT CALLBACK HookMessages(int code, WPARAM wp, LPARAM lp);
	thread_local inline static bool s_Hooked{ false };
	inline static TP_CALLBACK_ENVIRON s_TpEnviron{};
	inline static WinLL::ThreadPool s_ThreadPool;
};

