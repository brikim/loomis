
#include <cstdlib>
#include <optional>
#include <string>

namespace loomis
{
   inline std::optional<std::string> GetEnv(std::string_view name)
   {
#ifdef _WIN32
      char* buffer = nullptr;
      size_t size = 0;
      if (_dupenv_s(&buffer, &size, name.data()) == 0 && buffer != nullptr)
      {
         std::string value(buffer);
         std::free(buffer);
         return value;
      }
      return std::nullopt;
#else
      if (const char* value = std::getenv(name.data()); value != nullptr)
      {
         return std::string(value);
      }
      return std::nullopt;
#endif
   }
} // namespace loomis
