#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/stat.h>

#include <spdlog/spdlog.h>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int verbose = 0;
  for (int optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
      size_t arglen = strlen(argv[optind]);
      if (arglen >= 2 && argv[optind][1] == '-') break;
      for (size_t i = 1; i < arglen; i++)
          if (argv[optind][i] == 'v') verbose++;
  }

  if (verbose >= 3) {
      spdlog::set_level(spdlog::level::trace);
  } else if (verbose == 2) {
      spdlog::set_level(spdlog::level::debug);
  } else if (verbose == 1) {
      spdlog::set_level(spdlog::level::info);
  } else {
      spdlog::set_level(spdlog::level::warn);
  }

  return RUN_ALL_TESTS();
}
