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

	static WorkItem* SubmitWork(std::function<void()> work, std::function<void()> callback);
	static LRESULT CALLBACK HookMessages(int code, WPARAM wp, LPARAM lp);
};

