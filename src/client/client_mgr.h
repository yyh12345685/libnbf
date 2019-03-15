#pragma once

#include <map>
#include "common/logger.h"
#include "common/common.h"

namespace bdf{

struct ClientRoutersConfig;
class ClientRouter;

class ClientMgr{

public:
  static ClientMgr& GetInstance() {
    static ClientMgr inst;
    return inst;
  }

  int Start(ClientRoutersConfig& routers_config);
  int Stop();

  ClientRouter* GetClientRouter(const std::string& name) const;

private:
  std::map<std::string, ClientRouter*> router_;

  DISALLOW_COPY_ASSIGN_CONSTRUCTION(ClientMgr)

};

}

