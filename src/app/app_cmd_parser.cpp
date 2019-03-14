
#include <string.h>
#include <iostream>
#include "app/app_cmd_parser.h"

#ifndef __APP_VERSION__
#define __APP_VERSION__ "(date: 10:25:00 2019/3/10 +0800)"
#endif

namespace bdf{

static void UsageInner(const char *exe, bool help){
  std::cerr << "Copyright 2019 by yyh,"
    << " Basic Development Framework Model" << std::endl;
  std::cerr << "libbdf: " << __APP_VERSION__ << std::endl;
  if (help)
    std::cerr << "Usage: "
    << " --c config_file --l log_file [--d:daemon]" << std::endl;
}

int AppCmdParser::Usage(int argc, char* argv[]){
  if (argc == 2){
    if (0 == strcasecmp(argv[1], "-h") ||
      0 == strcasecmp(argv[1], "--help")){
      UsageInner(argv[0], true);
      help_mode_ = true;
      return 1;
    } else if (0 == strcasecmp(argv[1], "-v") ||
      0 == strcasecmp(argv[1], "--version")){
      UsageInner(argv[0], false);
      return 1;
    }
  }
  if (argc < 3){
    UsageInner(argv[0], true);
    return 2;
  }
  return 0;
}

int AppCmdParser::ParseArgv(int argc, char* argv[]){
  for (int i = 1; i < argc; ++i){
    if (0 == strcmp(argv[i], "--c")){
      if (++i < argc)
        application_config_ = argv[i];
      continue;
    }
    if (0 == strcmp(argv[i], "--d")){
      daemon_mode_ = true;
      continue;
    }
    if (0 == strcmp(argv[i], "--l")) {
      if (++i < argc)
        logger_config_ = argv[i];
      continue;
    }
    std::cerr << "unexpected argv: " << argv[i] << std::endl;
  }

  if (application_config_.empty()){
    std::cerr<<"configure file is not available" << std::endl;
    return 1;
  }
  return 0;
}

int AppCmdParser::ParseCmd(int argc, char* argv[]) {
  if ( 0 != Usage(argc,argv)){
    return -1;
  }

  if (0!= ParseArgv(argc, argv)){
    return -2;
  }

  return 0;
}

}
