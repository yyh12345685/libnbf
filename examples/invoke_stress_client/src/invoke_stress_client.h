#pragma once

#include <atomic>

namespace bdf {
class ClientMgr;
}

void RunInvokeStress(bdf::ClientMgr& client_mgr,
                     const std::atomic<bool>& stop_requested);
