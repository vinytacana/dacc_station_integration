#ifndef CONFIG_DACC_NETWORK_PARSING_HPP
#define CONFIG_DACC_NETWORK_PARSING_HPP

#include <string>
#include <vector>

namespace config_dacc {
namespace network_parsing {

inline std::vector<std::string> split_nmcli_escaped_fields(const std::string& linha) {
    std::vector<std::string> campos;
    std::string atual;
    bool escape = false;

    for (char c : linha) {
        if (escape) {
            atual.push_back(c);
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == ':') {
            campos.push_back(atual);
            atual.clear();
            continue;
        }
        atual.push_back(c);
    }
    campos.push_back(atual);
    return campos;
}

} // namespace network_parsing
} // namespace config_dacc

#endif
