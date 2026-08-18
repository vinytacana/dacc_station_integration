#include "ProcessManager.hpp"

#include "ipc/FramedSocket.hpp"
#include "json.hpp"

#include <spdlog/sinks/ostream_sink.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <poll.h>
#include <regex>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

using json = nlohmann::json;
using namespace std::chrono_literals;

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

class TempDirectory {
public:
    TempDirectory() {
        char modelo[] = "/tmp/dacc-pm-test-XXXXXX";
        char* criado = mkdtemp(modelo);
        exigir(criado != nullptr, "diretorio temporario deve ser criado");
        path_ = criado;
    }

    ~TempDirectory() {
        unlink((path_ + "/exit-zero.sh").c_str());
        unlink((path_ + "/exit-one.sh").c_str());
        unlink((path_ + "/create-marker.sh").c_str());
        unlink((path_ + "/orphan-marker.sh").c_str());
        unlink((path_ + "/slow-exec.sh").c_str());
        unlink((path_ + "/stop-before-exec.sh").c_str());
        unlink((path_ + "/unexpected-marker").c_str());
        unlink((path_ + "/orphan-marker").c_str());
        unlink((path_ + "/slow-exec-marker").c_str());
        unlink((path_ + "/daemon.sock").c_str());
        rmdir(path_.c_str());
    }

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

void criarScript(const std::string& path, const std::string& body) {
    std::ofstream script(path);
    exigir(script.is_open(), "script artificial deve ser criado");
    script << "#!/bin/bash\n" << body;
    script.close();
    exigir(chmod(path.c_str(), 0700) == 0, "script artificial deve ser executavel");
}

std::string caminhoAbsoluto(const std::string& path) {
    char* resolved = realpath(path.c_str(), nullptr);
    exigir(resolved != nullptr, "caminho do helper deve existir");
    std::string absolute_path(resolved);
    free(resolved);
    return absolute_path;
}

std::shared_ptr<spdlog::logger> criarLogger(std::ostringstream& output) {
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(output);
    return std::make_shared<spdlog::logger>("process-manager-test", std::move(sink));
}

int conectar(const std::string& socket_path) {
    const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    exigir(fd >= 0, "socket cliente deve ser criado");

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    exigir(socket_path.size() < sizeof(address.sun_path), "socket temporario deve caber");
    std::strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);
    exigir(
        connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
        "cliente deve conectar ao daemon temporario"
    );
    return fd;
}

std::optional<json> tentarReceberEvento(
    int fd,
    ipc::FrameReader& reader,
    std::chrono::milliseconds timeout
) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::array<char, 4096> buffer{};

    while (std::chrono::steady_clock::now() < deadline) {
        if (auto frame = reader.next()) {
            return json::parse(*frame);
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()
        );
        pollfd descriptor{fd, POLLIN, 0};
        const int poll_result = poll(
            &descriptor,
            1,
            static_cast<int>(std::max<std::int64_t>(1, remaining.count()))
        );
        if (poll_result < 0 && errno == EINTR) {
            continue;
        }
        exigir(poll_result >= 0, "poll do cliente nao deve falhar");
        if (poll_result == 0) {
            return std::nullopt;
        }

        const ssize_t count = recv(fd, buffer.data(), buffer.size(), 0);
        exigir(count > 0, "daemon deve manter conexao durante o teste");
        reader.feed(buffer.data(), static_cast<std::size_t>(count));
        exigir(!reader.overflowed(), "daemon nao deve enviar frame acima do limite");
    }

    return std::nullopt;
}

json receberEvento(int fd, ipc::FrameReader& reader, std::chrono::milliseconds timeout) {
    auto event = tentarReceberEvento(fd, reader, timeout);
    exigir(event.has_value(), "daemon deve responder dentro do prazo");
    return std::move(*event);
}

void exigirCorrelacao(
    const json& event,
    const std::string& request_id,
    const std::string& game_id
) {
    exigir(event.value("request_id", "") == request_id, "evento deve preservar request_id");
    exigir(event.value("game_id", "") == game_id, "evento deve preservar game_id");
}

void enviarStart(
    int fd,
    const std::string& request_id,
    const std::string& game_id,
    const std::string& path
) {
    const json command{
        {"action", "start"},
        {"request_id", request_id},
        {"game_id", game_id},
        {"path", path}
    };
    const ipc::SendResult send_result = ipc::sendMessage(fd, command.dump(), 500ms);
    exigir(send_result.status == ipc::SendStatus::Ok, "comando start deve ser enviado");
}

void exigirExecucao(
    int fd,
    ipc::FrameReader& reader,
    const std::string& request_id,
    const std::string& game_id,
    const std::string& path,
    int expected_exit_status
) {
    enviarStart(fd, request_id, game_id, path);
    const json started = receberEvento(fd, reader, 3s);
    exigir(started.value("event", "") == "game_started", "processo deve iniciar");
    exigirCorrelacao(started, request_id, game_id);
    exigir(started.contains("pid"), "game_started deve conter PID");

    const json finished = receberEvento(fd, reader, 3s);
    exigir(finished.value("event", "") == "game_finished", "processo deve terminar");
    exigirCorrelacao(finished, request_id, game_id);
    exigir(
        finished.value("exit_status", -1) == expected_exit_status,
        "game_finished deve preservar status de saida"
    );
}

void testarExecucoesDeterministicas() {
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string exit_one = temp.path() + "/exit-one.sh";
    const std::string marker_script = temp.path() + "/create-marker.sh";
    const std::string marker = temp.path() + "/unexpected-marker";
    const std::string missing = temp.path() + "/missing-executable";
    const std::string socket_path = temp.path() + "/daemon.sock";
    const std::string sigmask_helper = caminhoAbsoluto(SIGMASK_HELPER_PATH);

    criarScript(exit_zero, "exit 0\n");
    criarScript(exit_one, "exit 1\n");
    criarScript(marker_script, ": > \"" + marker + "\"\nexit 0\n");

    ProcessManager manager(nullptr, socket_path);
    const int client_fd = conectar(socket_path);
    ipc::FrameReader reader;

    const json missing_request_id{
        {"action", "start"},
        {"game_id", "must-not-start"},
        {"path", marker_script}
    };
    exigir(
        ipc::sendMessage(client_fd, missing_request_id.dump(), 500ms).status ==
            ipc::SendStatus::Ok,
        "comando sem request_id deve chegar ao daemon"
    );
    exigir(
        !tentarReceberEvento(client_fd, reader, 200ms).has_value(),
        "comando sem request_id deve ser descartado sem resposta"
    );
    exigir(access(marker.c_str(), F_OK) != 0, "comando descartado nao deve criar processo");

    exigirExecucao(client_fd, reader, "request-zero", "exit-zero", exit_zero, 0);
    exigirExecucao(client_fd, reader, "request-one", "exit-one", exit_one, 1);
    exigirExecucao(
        client_fd,
        reader,
        "request-signal-mask",
        "sigmask-helper",
        sigmask_helper,
        0
    );

    enviarStart(client_fd, "request-missing", "missing", missing);
    const json missing_failed = receberEvento(client_fd, reader, 3s);
    exigir(
        missing_failed.value("event", "") == "game_start_failed",
        "exec inexistente deve falhar antes de game_started"
    );
    exigirCorrelacao(missing_failed, "request-missing", "missing");
    exigir(missing_failed.value("error_code", 0) == ENOENT, "falha deve preservar errno");
    exigir(!missing_failed.value("message", "").empty(), "falha deve conter mensagem");
    close(client_fd);
}

void testarPrazoDaConfirmacaoDeExec() {
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string stopped_before_exec = temp.path() + "/stop-before-exec.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    criarScript(stopped_before_exec, "exit 0\n");

    ProcessManager manager(nullptr, socket_path);
    const int client_fd = conectar(socket_path);
    ipc::FrameReader reader;

    const auto started_at = std::chrono::steady_clock::now();
    enviarStart(client_fd, "request-timeout", "stopped-before-exec", stopped_before_exec);
    const json failed = receberEvento(client_fd, reader, 3s);
    const auto elapsed = std::chrono::steady_clock::now() - started_at;

    exigir(failed.value("event", "") == "game_start_failed", "timeout deve falhar start");
    exigirCorrelacao(failed, "request-timeout", "stopped-before-exec");
    exigir(failed.value("error_code", 0) == ETIMEDOUT, "timeout deve preservar ETIMEDOUT");
    exigir(elapsed >= 1800ms && elapsed < 3s, "deadline deve ocorrer proximo de dois segundos");

    exigirExecucao(
        client_fd,
        reader,
        "request-after-timeout",
        "exit-zero",
        exit_zero,
        0
    );
    close(client_fd);
}

void testarEncerramentoAoDesconectar() {
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string orphan_script = temp.path() + "/orphan-marker.sh";
    const std::string marker = temp.path() + "/orphan-marker";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    criarScript(orphan_script, "sleep 0.8\n: > \"" + marker + "\"\nexit 0\n");

    std::ostringstream logs;
    {
        ProcessManager manager(criarLogger(logs), socket_path);
        const int owner_fd = conectar(socket_path);
        ipc::FrameReader owner_reader;
        enviarStart(owner_fd, "request-orphan", "orphan-marker", orphan_script);
        const json started = receberEvento(owner_fd, owner_reader, 3s);
        exigir(started.value("event", "") == "game_started", "jogo orfao deve iniciar");
        exigirCorrelacao(started, "request-orphan", "orphan-marker");
        close(owner_fd);

        std::this_thread::sleep_for(1200ms);
        exigir(access(marker.c_str(), F_OK) != 0, "jogo orfao deve ser encerrado");

        const int replacement_fd = conectar(socket_path);
        ipc::FrameReader reader;
        exigirExecucao(
            replacement_fd,
            reader,
            "request-after-disconnect",
            "exit-zero",
            exit_zero,
            0
        );
        close(replacement_fd);
    }

    exigir(
        logs.str().find("request-orphan") != std::string::npos,
        "daemon deve ter processado o start antes da desconexao"
    );
    exigir(
        logs.str().find("owner client disconnected") != std::string::npos,
        "daemon deve registrar desconexao do cliente dono"
    );
    exigir(
        logs.str().find("game_started could not be delivered") == std::string::npos,
        "entrega confirmada nao deve ser registrada como falha"
    );
}

void testarFalhaAoEntregarGameStarted() {
    TempDirectory temp;
    const std::string slow_script = temp.path() + "/slow-exec.sh";
    const std::string marker = temp.path() + "/slow-exec-marker";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(slow_script, "sleep 0.5\n: > \"" + marker + "\"\nexit 0\n");

    std::ostringstream logs;
    {
        ProcessManager manager(criarLogger(logs), socket_path);
        const int owner_fd = conectar(socket_path);
        enviarStart(owner_fd, "request-undelivered", "slow-exec", slow_script);
        close(owner_fd);

        std::this_thread::sleep_for(1200ms);
        exigir(
            access(marker.c_str(), F_OK) != 0,
            "jogo sem confirmacao entregue deve ser encerrado"
        );
    }

    exigir(
        logs.str().find("request-undelivered") != std::string::npos,
        "daemon deve processar o start antes da falha de entrega"
    );
    exigir(
        logs.str().find("game_started could not be delivered") != std::string::npos,
        "daemon deve registrar falha ao entregar game_started"
    );
    exigir(
        logs.str().find("owner client disconnected") == std::string::npos,
        "falha de entrega nao deve ser registrada como desconexao posterior"
    );
}

void testarGeracaoEmFdReutilizado() {
    TempDirectory temp;
    const std::string socket_path = temp.path() + "/daemon.sock";
    std::ostringstream logs;

    {
        ProcessManager manager(criarLogger(logs), socket_path);
        const int first_client = conectar(socket_path);
        std::this_thread::sleep_for(100ms);
        close(first_client);
        std::this_thread::sleep_for(100ms);

        const int second_client = conectar(socket_path);
        std::this_thread::sleep_for(100ms);
        close(second_client);
        std::this_thread::sleep_for(100ms);
    }

    const std::regex connection_pattern(
        R"(New client connection accepted \(FD: ([0-9]+), generation: ([0-9]+)\))"
    );
    std::vector<std::pair<int, std::uint64_t>> connections;
    const std::string output = logs.str();
    for (auto match = std::sregex_iterator(output.begin(), output.end(), connection_pattern);
         match != std::sregex_iterator();
         ++match) {
        connections.emplace_back(
            std::stoi((*match)[1].str()),
            std::stoull((*match)[2].str())
        );
    }

    exigir(connections.size() >= 2, "duas conexoes devem ser registradas");
    bool found_reused_fd = false;
    for (std::size_t first = 0; first < connections.size(); ++first) {
        for (std::size_t second = first + 1; second < connections.size(); ++second) {
            if (connections[first].first == connections[second].first &&
                connections[first].second != connections[second].second) {
                found_reused_fd = true;
            }
        }
    }
    exigir(found_reused_fd, "fd reutilizado deve receber uma nova geracao");
}

} // namespace

int main() {
    testarExecucoesDeterministicas();
    testarPrazoDaConfirmacaoDeExec();
    testarEncerramentoAoDesconectar();
    testarFalhaAoEntregarGameStarted();
    testarGeracaoEmFdReutilizado();
    std::cout << "Process Manager integration tests passed." << std::endl;
    return 0;
}
