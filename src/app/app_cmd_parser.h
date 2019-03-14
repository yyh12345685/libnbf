#pragma once

#include <string>

namespace bdf{

class AppCmdParser{
public:
  int ParseCmd(int argc, char* argv[]);

  uint32_t start_time_;
  bool help_mode_ = false;
  bool daemon_mode_ = false;
  std::string logger_config_;
  std::string application_config_;

private:
  int Usage(int argc, char* argv[]);
  int ParseArgv(int argc, char* argv[]);
};

}
