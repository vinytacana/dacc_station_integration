#ifndef CONFIG_DACC_CONFIG_RESULT_HPP
#define CONFIG_DACC_CONFIG_RESULT_HPP

#include "config-dacc/functions.hpp"

#include <string>

namespace config_result {

inline system_result success(const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = true;
    result.codigo = "ok";
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

inline system_result error(
    const std::string& codigo,
    const std::string& mensagem,
    const std::string& detalhes = ""
) {
    system_result result;
    result.ok = false;
    result.codigo = codigo;
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

} // namespace config_result

#endif
