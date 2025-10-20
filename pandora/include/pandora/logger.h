#ifndef PANDORA_LOGGER_H_
#define PANDORA_LOGGER_H_

#include <iostream>
#include <string>

namespace pandora {

class Logger {
 public:
  enum Level { VERBOSE, DEBUG, INFO, WARN, ERROR };
  static bool debug;
  static Level level;
  static std::string tag;

  static void Println(Level lvl, const std::string& t, const std::string& msg) {
    // TODO: 推荐使用 spdlog，当前用 std::cout
    if (debug && Require(lvl)) {
      std::cout << "[" << t << "] " << msg << std::endl;
    }
  }

  static void SetTag(const std::string& t) { tag = t; }
  static void SetLevel(Level lvl) { level = lvl; }
  static bool Require(Level lvl) { return level <= lvl; }
};

inline bool Logger::debug = false;
inline Logger::Level Logger::level = Logger::DEBUG;
inline std::string Logger::tag = "Pandora";

}  // namespace pandora

#endif  // PANDORA_LOGGER_H_

