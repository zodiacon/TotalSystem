#pragma once

#include <functional>


struct UI abstract final {
public:
	struct WorkItem {
		friend struct UI;

		std::function<void()> Work;
		std::function<void()> Callback;
		uint32_t Tid;
	};

	struct WorkItemParam {
		friend struct UI;

		std::function<void*()> Work;
		std::function<void(void*)> Callback;
		uint32_t Tid;
		void* Result{ nullptr };
	};

	static WorkItem* SubmitWork(std::function<void()> work, std::function<void()> callback);
	static WorkItemParam* SubmitWorkWithResult(std::function<void* ()> work, std::function<void(void*)> callback);

private:
	static LRESULT CALLBACK HookMessages(int code, WPARAM wp, LPARAM lp);
	thread_local inline static bool s_Hooked{ false };
};

