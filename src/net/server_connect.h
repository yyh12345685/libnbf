#pragma once

#include "net/connect.h"
#include "common/logger.h"

namespace bdf {

class EventMessage;

class ServerConnect :public Connecting {

public:
  ServerConnect();
  virtual ~ServerConnect();

  virtual void Destroy();

  virtual void OnClose();
  virtual void OnDecodeMessage(EventMessage* message);
  virtual int RegisterDel(int fd);

  virtual bool IsServer() { return true; }

  int64_t GetConnectId() { return connect_id_; }
  void SetConnectId(int64_t connect_id) { connect_id_ = connect_id; }

private:
  LOGGER_CLASS_DECL(logger_);

  int64_t connect_id_;
};

}
