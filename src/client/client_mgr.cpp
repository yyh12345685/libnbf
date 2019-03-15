#include "client/client_mgr.h"
#include "app/config_info.h"

namespace bdf {

int ClientMgr::Start(ClientRoutersConfig& routers_config){
  return 0;
}

int ClientMgr::Stop(){
  return 0;
}

ClientRouter* ClientMgr::GetClientRouter(const std::string& name) const {
  const auto it = router_.find(name);
  if (it != router_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

}
