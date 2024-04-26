#include "pch.h"
#include "ProcessHelper.h"
#include <ranges>
#include "ProcessInfoEx.h"

using namespace std;
using namespace std::views;
using namespace std::ranges;

vector<uint32_t> ProcessHelper::GetProcessIdsByName(std::vector<std::shared_ptr<ProcessInfoEx>> const& processes, wstring_view name) {
    auto pids = processes | filter([&](auto& p) { return _wcsicmp(p->GetImageName().c_str(), name.data()) == 0; }) | views::transform([](auto& p) { return p->Id; }) | to<vector>();
    return pids;

}

ImColor ProcessHelper::GetColorByCPU(float cpu) {
    return ImColor::HSV(1.0f, cpu / 60 + .4f, .3f);
}
