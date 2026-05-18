#pragma once

#include <string>

#include "config-dacc/functions.hpp"

namespace MeuProjeto {

std::string mensagemResultadoUi(const system_result& resultado, const std::string& fallback);
std::string mensagemRecursoIndisponivelUi(const std::string& recurso);
void registrarResultadoErroUi(const std::string& origem, const std::string& operacao, const system_result& resultado);

} // namespace MeuProjeto
