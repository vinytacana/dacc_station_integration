#include "ProcessManager.hpp"

#include "ipc/FramedSocket.hpp"
#include "json.hpp"
#include "send_test_double.hpp"

#include <spdlog/sinks/base_sink.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <optional>
#include <poll.h>
#include <regex>
#include <signal.h>
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
        std::filesystem::remove_all(path_);
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

std::string criarCatalogo(const std::string& directory) {
    json games = json::array();
    for (const auto& file : std::filesystem::directory_iterator(directory)) {
        if (file.path().extension() == ".sh") games.push_back({
            {"id", file.path().stem().string()}, {"argv", {file.path().string()}}
        });
    }
    games.push_back({{"id", "stopped-before-exec"}, {"argv", {directory + "/stop-before-exec.sh"}}});
    games.push_back({{"id", "missing"}, {"argv", {directory + "/missing-executable"}}});
    games.push_back({{"id", "sigmask-helper"}, {"argv", {SIGMASK_HELPER_PATH}}});
    const auto path = directory + "/games.json";
    { std::ofstream file(path); file << json{{"games", games}}; }
    exigir(chmod(path.c_str(), 0600) == 0, "catalogo de teste privado");
    return path;
}

std::string caminhoAbsoluto(const std::string& path) {
    char* resolved = realpath(path.c_str(), nullptr);
    exigir(resolved != nullptr, "caminho do helper deve existir");
    std::string absolute_path(resolved);
    free(resolved);
    return absolute_path;
}

class LogCaptureSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    std::string snapshot() {
        std::lock_guard<std::mutex> lock(this->mutex_);
        return output_;
    }
protected:
    void sink_it_(const spdlog::details::log_msg& message) override {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(message, formatted);
        output_.append(formatted.data(), formatted.size());
    }
    void flush_() override {}
private:
    std::string output_;
};

class LogCapture {
public:
    LogCapture() : sink_(std::make_shared<LogCaptureSink>()) {}
    std::shared_ptr<spdlog::logger> logger() {
        return std::make_shared<spdlog::logger>("process-manager-test", sink_);
    }
    std::string str() { return sink_->snapshot(); }
private:
    std::shared_ptr<LogCaptureSink> sink_;
};

std::shared_ptr<spdlog::logger> criarLogger(LogCapture& output) {
    return output.logger();
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

void exigirSocketFechado(int fd, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::array<char, 4096> buffer{};

    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()
        );
        pollfd descriptor{fd, POLLIN | POLLHUP | POLLERR, 0};
        const int poll_result = poll(
            &descriptor,
            1,
            static_cast<int>(std::max<std::int64_t>(1, remaining.count()))
        );
        if (poll_result < 0 && errno == EINTR) {
            continue;
        }
        exigir(poll_result >= 0, "poll ao aguardar fechamento nao deve falhar");
        if (poll_result == 0) {
            continue;
        }

        const ssize_t count = recv(fd, buffer.data(), buffer.size(), MSG_DONTWAIT);
        if (count == 0) {
            return;
        }
        if (count < 0 && (errno == ECONNRESET || errno == ENOTCONN)) {
            return;
        }
        if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        }
        exigir(count >= 0, "recv ao aguardar fechamento nao deve falhar");
    }

    exigir(false, "daemon deve fechar o socket apos falha de evento terminal");
}

std::size_t contarOcorrencias(const std::string& texto, const std::string& trecho) {
    std::size_t total = 0;
    std::size_t posicao = 0;
    while ((posicao = texto.find(trecho, posicao)) != std::string::npos) {
        ++total;
        posicao += trecho.size();
    }
    return total;
}

void configurarFalhaAposEscritaParcial(const std::string& event_name) {
    test_support::PartialSendPlan partial_plan;
    partial_plan.payload_fragment = "\"event\":\"" + event_name + "\"";
    partial_plan.max_bytes_per_call = 8;
    partial_plan.successful_partial_sends_before_error = 1;
    partial_plan.error_code = EIO;
    test_support::configurePartialSends(partial_plan);
}

json exigirFalhaParcialInterceptada(const std::string& event_name) {
    exigir(
        test_support::successfulPartialSendCount() == 1,
        event_name + " deve escrever exatamente um fragmento antes da falha"
    );
    exigir(
        test_support::matchingSendFailureCount() == 1,
        event_name + " deve falhar exatamente uma vez apos a escrita parcial"
    );
    const json failed_event = json::parse(test_support::lastFailedSendPayload());
    exigir(
        failed_event.value("event", "") == event_name,
        "double deve preservar o evento completo que originou o frame truncado"
    );
    return failed_event;
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
    const std::string& /* path: server catalog is authoritative */
) {
    const json command{
        {"action", "start"},
        {"request_id", request_id},
        {"game_id", game_id}
    };
    const ipc::SendResult send_result = ipc::sendMessage(fd, command.dump(), 500ms);
    exigir(send_result.status == ipc::SendStatus::Ok, "comando start deve ser enviado");
}

void enviarComando(int fd, const json& command) {
    const auto result = ipc::sendMessage(fd, command.dump(), 500ms);
    exigir(result.status == ipc::SendStatus::Ok, "comando IPC deve ser enviado");
}

void enviarBytes(int fd, const std::string& bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = send(fd, bytes.data() + offset, bytes.size() - offset, MSG_NOSIGNAL);
        if (count > 0) offset += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else break;
    }
}

void exigirProcessoAusente(pid_t pid, std::chrono::milliseconds timeout = 2s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        errno = 0;
        if (kill(pid, 0) < 0 && errno == ESRCH) return;
        std::this_thread::sleep_for(10ms);
    }
    exigir(false, "processo deve estar encerrado e coletado: " + std::to_string(pid));
}

pid_t aguardarPidNoArquivo(const std::string& path, std::chrono::milliseconds timeout = 2s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        std::ifstream input(path);
        pid_t pid = -1;
        if (input >> pid && pid > 1) return pid;
        std::this_thread::sleep_for(10ms);
    }
    exigir(false, "helper deve registrar PID descendente");
    return -1;
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
    test_support::resetSendTestDouble();
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

    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
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
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string stopped_before_exec = temp.path() + "/stop-before-exec.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    criarScript(stopped_before_exec, "exit 0\n");

    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
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
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string orphan_script = temp.path() + "/orphan-marker.sh";
    const std::string marker = temp.path() + "/orphan-marker";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    criarScript(orphan_script, "sleep 0.8\n: > \"" + marker + "\"\nexit 0\n");

    LogCapture logs;
    {
        ProcessManager manager(criarLogger(logs), socket_path, criarCatalogo(temp.path()));
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
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string slow_script = temp.path() + "/slow-exec.sh";
    const std::string marker = temp.path() + "/slow-exec-marker";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    criarScript(slow_script, "sleep 0.5\n: > \"" + marker + "\"\nexit 0\n");

    LogCapture logs;
    {
        ProcessManager manager(criarLogger(logs), socket_path, criarCatalogo(temp.path()));
        const int owner_fd = conectar(socket_path);
        test_support::failMatchingSends("\"event\":\"game_started\"", EIO);
        enviarStart(owner_fd, "request-undelivered", "slow-exec", slow_script);
        exigirSocketFechado(owner_fd, 3s);

        exigir(
            test_support::matchingSendFailureCount() == 1,
            "game_started deve falhar exatamente uma vez no transporte"
        );
        const json failed_event = json::parse(test_support::lastFailedSendPayload());
        exigir(
            failed_event.value("event", "") == "game_started",
            "double deve interceptar game_started"
        );
        const pid_t failed_pid = failed_event.value("pid", -1);
        exigir(failed_pid > 0, "game_started interceptado deve identificar o PID");

        errno = 0;
        exigir(
            kill(failed_pid, 0) == -1 && errno == ESRCH,
            "processo sem confirmacao entregue deve estar sinalizado e colhido"
        );
        exigir(
            access(marker.c_str(), F_OK) != 0,
            "jogo sem confirmacao entregue deve ser encerrado"
        );

        const std::string reap_log =
            "Reaped terminated process PID=" + std::to_string(failed_pid);
        const std::string termination_log =
            "Terminating PID=" + std::to_string(failed_pid) +
            " because game_started could not be delivered";
        exigir(
            contarOcorrencias(logs.str(), termination_log) == 1,
            "processo sem confirmacao deve ser sinalizado exatamente uma vez"
        );
        exigir(
            contarOcorrencias(logs.str(), reap_log) == 1,
            "processo sem confirmacao deve ser colhido exatamente uma vez"
        );
        exigir(
            logs.str().find("owner client disconnected") == std::string::npos,
            "desconexao nao deve tentar encerrar novamente processo ja removido"
        );

        close(owner_fd);
        const int replacement_fd = conectar(socket_path);
        ipc::FrameReader replacement_reader;
        exigirExecucao(
            replacement_fd,
            replacement_reader,
            "request-after-undelivered-start",
            "exit-zero",
            exit_zero,
            0
        );
        close(replacement_fd);
    }

    exigir(
        logs.str().find("request-undelivered") != std::string::npos,
        "daemon deve processar o start antes da falha de entrega"
    );
    exigir(
        logs.str().find("game_started could not be delivered") != std::string::npos,
        "daemon deve registrar falha ao entregar game_started"
    );
}

void testarFalhaAoEntregarGameStartFailed() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string missing = temp.path() + "/missing-executable";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");

    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
    const int owner_fd = conectar(socket_path);
    test_support::failMatchingSends("\"event\":\"game_start_failed\"", EIO);
    enviarStart(
        owner_fd,
        "request-undelivered-failure",
        "missing",
        missing
    );
    exigirSocketFechado(owner_fd, 3s);

    exigir(
        test_support::matchingSendFailureCount() == 1,
        "game_start_failed deve falhar exatamente uma vez no transporte"
    );
    const json failed_event = json::parse(test_support::lastFailedSendPayload());
    exigir(
        failed_event.value("event", "") == "game_start_failed",
        "double deve interceptar game_start_failed"
    );
    exigirCorrelacao(failed_event, "request-undelivered-failure", "missing");
    close(owner_fd);
    const int replacement_fd = conectar(socket_path);
    ipc::FrameReader replacement_reader;
    exigirExecucao(
        replacement_fd,
        replacement_reader,
        "request-undelivered-failure",
        "exit-zero",
        exit_zero,
        0
    );
    close(replacement_fd);
}

void testarFalhaAoEntregarGameFinished() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");

    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
    const int owner_fd = conectar(socket_path);
    ipc::FrameReader owner_reader;
    test_support::failMatchingSends("\"event\":\"game_finished\"", EIO);
    enviarStart(owner_fd, "request-undelivered-finish", "exit-zero", exit_zero);

    const json started = receberEvento(owner_fd, owner_reader, 3s);
    exigir(
        started.value("event", "") == "game_started",
        "game_started deve ser entregue antes da falha terminal"
    );
    exigirCorrelacao(started, "request-undelivered-finish", "exit-zero");
    exigirSocketFechado(owner_fd, 3s);

    exigir(
        test_support::matchingSendFailureCount() == 1,
        "game_finished deve falhar exatamente uma vez no transporte"
    );
    const json failed_event = json::parse(test_support::lastFailedSendPayload());
    exigir(
        failed_event.value("event", "") == "game_finished",
        "double deve interceptar game_finished"
    );
    exigirCorrelacao(failed_event, "request-undelivered-finish", "exit-zero");
    close(owner_fd);
    const int replacement_fd = conectar(socket_path);
    ipc::FrameReader replacement_reader;
    exigirExecucao(
        replacement_fd,
        replacement_reader,
        "request-after-undelivered-finish",
        "exit-zero",
        exit_zero,
        0
    );
    close(replacement_fd);
}

void testarEscritaParcialDeGameStarted() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string slow_script = temp.path() + "/slow-exec.sh";
    const std::string marker = temp.path() + "/slow-exec-marker";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    criarScript(slow_script, "sleep 0.5\n: > \"" + marker + "\"\nexit 0\n");

    LogCapture logs;
    ProcessManager manager(criarLogger(logs), socket_path, criarCatalogo(temp.path()));
    const int owner_fd = conectar(socket_path);
    configurarFalhaAposEscritaParcial("game_started");
    enviarStart(owner_fd, "request-partial-start", "slow-exec", slow_script);
    exigirSocketFechado(owner_fd, 3s);

    const json failed_event = exigirFalhaParcialInterceptada("game_started");
    exigirCorrelacao(failed_event, "request-partial-start", "slow-exec");
    const pid_t failed_pid = failed_event.value("pid", -1);
    exigir(failed_pid > 0, "game_started parcial deve identificar o PID");

    errno = 0;
    exigir(
        kill(failed_pid, 0) == -1 && errno == ESRCH,
        "processo com game_started parcial deve estar sinalizado e colhido"
    );
    exigir(
        access(marker.c_str(), F_OK) != 0,
        "processo com game_started parcial nao deve concluir o script"
    );

    const std::string termination_log =
        "Terminating PID=" + std::to_string(failed_pid) +
        " because game_started could not be delivered";
    const std::string reap_log =
        "Reaped terminated process PID=" + std::to_string(failed_pid);
    exigir(
        contarOcorrencias(logs.str(), termination_log) == 1,
        "game_started parcial deve sinalizar o processo exatamente uma vez"
    );
    exigir(
        contarOcorrencias(logs.str(), reap_log) == 1,
        "game_started parcial deve colher o processo exatamente uma vez"
    );
    exigir(
        logs.str().find("owner client disconnected") == std::string::npos,
        "desconexao nao deve repetir teardown apos game_started parcial"
    );

    close(owner_fd);
    test_support::resetSendTestDouble();
    const int replacement_fd = conectar(socket_path);
    ipc::FrameReader replacement_reader;
    exigirExecucao(
        replacement_fd,
        replacement_reader,
        "request-after-partial-start",
        "exit-zero",
        exit_zero,
        0
    );
    close(replacement_fd);
}

void testarEscritaParcialDeGameStartFailed() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string missing = temp.path() + "/missing-executable";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");

    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
    const int owner_fd = conectar(socket_path);
    configurarFalhaAposEscritaParcial("game_start_failed");
    enviarStart(owner_fd, "request-partial-failure", "missing", missing);
    exigirSocketFechado(owner_fd, 3s);

    const json failed_event = exigirFalhaParcialInterceptada("game_start_failed");
    exigirCorrelacao(failed_event, "request-partial-failure", "missing");

    close(owner_fd);
    test_support::resetSendTestDouble();
    const int replacement_fd = conectar(socket_path);
    ipc::FrameReader replacement_reader;
    exigirExecucao(
        replacement_fd,
        replacement_reader,
        "request-partial-failure",
        "exit-zero",
        exit_zero,
        0
    );
    close(replacement_fd);
}

void testarEscritaParcialDeGameFinished() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");

    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
    const int owner_fd = conectar(socket_path);
    ipc::FrameReader owner_reader;
    configurarFalhaAposEscritaParcial("game_finished");
    enviarStart(owner_fd, "request-partial-finish", "exit-zero", exit_zero);

    const json started = receberEvento(owner_fd, owner_reader, 3s);
    exigir(
        started.value("event", "") == "game_started",
        "game_started deve chegar antes do game_finished parcial"
    );
    exigirCorrelacao(started, "request-partial-finish", "exit-zero");
    exigirSocketFechado(owner_fd, 3s);

    const json failed_event = exigirFalhaParcialInterceptada("game_finished");
    exigirCorrelacao(failed_event, "request-partial-finish", "exit-zero");

    close(owner_fd);
    test_support::resetSendTestDouble();
    const int replacement_fd = conectar(socket_path);
    ipc::FrameReader replacement_reader;
    exigirExecucao(
        replacement_fd,
        replacement_reader,
        "request-after-partial-finish",
        "exit-zero",
        exit_zero,
        0
    );
    close(replacement_fd);
}

void testarGeracaoEmFdReutilizado() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string socket_path = temp.path() + "/daemon.sock";
    LogCapture logs;

    {
        ProcessManager manager(criarLogger(logs), socket_path, criarCatalogo(temp.path()));
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

void testarCatalogoEProtocoloAutoritativo() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string executable = temp.path() + "/exit-zero.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(executable, "exit 0\n");
    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
    const int fd = conectar(socket_path);
    ipc::FrameReader reader;

    auto exigirFalha = [&](const json& request, const std::string& code) {
        enviarComando(fd, request);
        const json response = receberEvento(fd, reader, 2s);
        exigir(response.value("event", "") == "game_start_failed", "pedido deve falhar");
        exigir(response.value("code", "") == code, "falha deve ter codigo " + code);
    };

    exigirFalha({{"action", "start"}, {"request_id", "unknown"}, {"game_id", "not-listed"}}, "unknown_game");
    exigirFalha({{"action", "start"}, {"request_id", "malformed"}, {"game_id", "../escape"}}, "invalid_game_id");
    const std::array<const char*, 4> forbidden{{"path", "argv", "cwd", "graphics"}};
    for (const char* field : forbidden) {
        json request{{"action", "start"}, {"request_id", std::string("forbidden-") + field}, {"game_id", "exit-zero"}};
        request[field] = field == std::string("argv") ? json::array({"/bin/true"}) : json("attacker-controlled");
        exigirFalha(request, "client_command_forbidden");
    }

    const json duplicate{{"action", "start"}, {"request_id", "unknown"}, {"game_id", "not-listed"}};
    enviarComando(fd, duplicate);
    const json duplicate_response = receberEvento(fd, reader, 2s);
    exigir(duplicate_response.value("event", "") == "request_rejected", "request_id duplicado deve ser rejeitado");
    exigir(duplicate_response.value("code", "") == "duplicate_request", "duplicata deve ter codigo explicito");
    close(fd);
}

void testarSaidasSinaisEEventoUnico() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string crash = temp.path() + "/crash.sh";
    const std::string self_term = temp.path() + "/self-term.sh";
    const std::string wait_signal = temp.path() + "/wait-signal.sh";
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(crash, "kill -ABRT $$\n");
    criarScript(self_term, "kill -TERM $$\n");
    criarScript(wait_signal, "while :; do sleep 1; done\n");
    criarScript(exit_zero, "exit 0\n");
    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));
    const int fd = conectar(socket_path);
    ipc::FrameReader reader;

    exigirExecucao(fd, reader, "crash-request", "crash", crash, 128 + SIGABRT);
    exigirExecucao(fd, reader, "self-term-request", "self-term", self_term, 128 + SIGTERM);

    enviarStart(fd, "external-term", "wait-signal", wait_signal);
    const json started = receberEvento(fd, reader, 2s);
    const pid_t pid = started.value("pid", -1);
    exigir(pid > 1, "jogo sinalizado deve iniciar");
    exigir(kill(pid, SIGTERM) == 0, "teste deve sinalizar o jogo");
    const json finished = receberEvento(fd, reader, 3s);
    exigir(finished.value("event", "") == "game_finished", "sinal deve gerar evento terminal");
    exigir(finished.value("exit_status", -1) == 128 + SIGTERM, "status deve preservar SIGTERM");
    exigir(!tentarReceberEvento(fd, reader, 200ms).has_value(), "evento terminal deve ser emitido uma unica vez");
    exigirProcessoAusente(pid);

    enviarStart(fd, "normal-latency", "exit-zero", exit_zero);
    const json normal_started = receberEvento(fd, reader, 2s);
    exigir(normal_started.value("event", "") == "game_started", "jogo normal deve iniciar");
    const auto latency_start = std::chrono::steady_clock::now();
    const json normal_finished = receberEvento(fd, reader, 2s);
    const auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - latency_start
    );
    exigir(normal_finished.value("event", "") == "game_finished", "jogo normal deve terminar");
    exigir(normal_finished.value("exit_status", -1) == 0, "jogo normal deve preservar exit 0");
    exigir(latency < 200ms, "exit normal nao deve aguardar grace period completo");
    std::cout << "Normal exit -> game_finished latency: " << latency.count() << " ms\n";
    close(fd);
}

void testarUnicoJogoEConcorrencia() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string active = temp.path() + "/active.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(active, "trap '' TERM\nwhile :; do sleep 1; done\n");
    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));

    int first = conectar(socket_path);
    ipc::FrameReader first_reader;
    enviarStart(first, "first-active", "active", active);
    const json started = receberEvento(first, first_reader, 2s);
    const pid_t first_pid = started.value("pid", -1);
    enviarStart(first, "same-active", "active", active);
    const json busy = receberEvento(first, first_reader, 2s);
    exigir(busy.value("code", "") == "game_already_running", "segundo launch deve ser rejeitado");
    close(first);
    exigirProcessoAusente(first_pid);

    int left = conectar(socket_path);
    int right = conectar(socket_path);
    ipc::FrameReader left_reader, right_reader;
    std::thread left_send([&] { enviarStart(left, "concurrent-left", "active", active); });
    std::thread right_send([&] { enviarStart(right, "concurrent-right", "active", active); });
    left_send.join();
    right_send.join();
    const json left_event = receberEvento(left, left_reader, 2s);
    const json right_event = receberEvento(right, right_reader, 2s);
    const int started_count = (left_event.value("event", "") == "game_started") +
                              (right_event.value("event", "") == "game_started");
    const int busy_count = (left_event.value("code", "") == "game_already_running") +
                           (right_event.value("code", "") == "game_already_running");
    exigir(started_count == 1, "apenas uma requisicao concorrente deve iniciar");
    exigir(busy_count == 1, "uma requisicao concorrente deve receber busy");
    const pid_t running_pid = left_event.value("pid", right_event.value("pid", -1));
    close(left);
    close(right);
    exigirProcessoAusente(running_pid);
}

void testarDescendentesTermKillEShutdown() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string child_file = temp.path() + "/child.pid";
    const std::string term_marker = temp.path() + "/term.marker";
    const std::string tree = temp.path() + "/tree.sh";
    const std::string detached_after_leader = temp.path() + "/leader-exits.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(
        tree,
        "trap 'echo term > \"" + term_marker + "\"; trap \"\" TERM' TERM\n"
        "(trap '' TERM; while :; do sleep 1; done) &\n"
        "echo $! > \"" + child_file + "\"\n"
        "while :; do sleep 1; done\n"
    );
    criarScript(
        detached_after_leader,
        "(trap '' TERM; while :; do sleep 1; done) &\n"
        "echo $! > \"" + child_file + "\"\nexit 0\n"
    );
    const std::string catalog = criarCatalogo(temp.path());

    pid_t leader = -1;
    pid_t child = -1;
    const auto shutdown_started = std::chrono::steady_clock::now();
    {
        auto manager = std::make_unique<ProcessManager>(nullptr, socket_path, catalog);
        const int fd = conectar(socket_path);
        ipc::FrameReader reader;
        enviarStart(fd, "tree-shutdown", "tree", tree);
        leader = receberEvento(fd, reader, 2s).value("pid", -1);
        child = aguardarPidNoArquivo(child_file);
        manager.reset();
        close(fd);
    }
    const auto shutdown_elapsed = std::chrono::steady_clock::now() - shutdown_started;
    exigir(shutdown_elapsed < 2s, "shutdown com jogo deve ter limite finito");
    exigir(access(term_marker.c_str(), F_OK) == 0, "grupo deve receber SIGTERM antes de SIGKILL");
    exigirProcessoAusente(leader);
    exigirProcessoAusente(child);
    exigir(access(socket_path.c_str(), F_OK) != 0, "shutdown deve remover apenas seu socket");

    unlink(child_file.c_str());
    {
        ProcessManager manager(nullptr, socket_path, catalog);
        const int fd = conectar(socket_path);
        ipc::FrameReader reader;
        enviarStart(fd, "leader-exits", "leader-exits", detached_after_leader);
        const pid_t parent = receberEvento(fd, reader, 2s).value("pid", -1);
        const pid_t descendant = aguardarPidNoArquivo(child_file);
        const json finished = receberEvento(fd, reader, 3s);
        exigir(finished.value("exit_status", -1) == 0, "saida do lider deve ser preservada");
        exigirProcessoAusente(parent);
        exigirProcessoAusente(descendant);
        close(fd);
    }

    const auto empty_started = std::chrono::steady_clock::now();
    { ProcessManager manager(nullptr, socket_path, catalog); }
    exigir(std::chrono::steady_clock::now() - empty_started < 1s, "shutdown sem jogo deve ser imediato");
    { ProcessManager restarted(nullptr, socket_path, catalog); }
}

void testarDestrutorDuranteLaunchECleanupIncompleto() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string stopped = temp.path() + "/stop-before-exec.sh";
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(stopped, "exit 0\n");
    criarScript(exit_zero, "exit 0\n");
    const std::string catalog = criarCatalogo(temp.path());

    auto manager = std::make_unique<ProcessManager>(nullptr, socket_path, catalog);
    const int launch_fd = conectar(socket_path);
    enviarStart(launch_fd, "destroy-launch", "stopped-before-exec", stopped);
    std::this_thread::sleep_for(100ms);
    const auto destroy_started = std::chrono::steady_clock::now();
    manager.reset();
    exigir(std::chrono::steady_clock::now() - destroy_started < 1500ms, "destrutor deve cancelar launch em andamento");
    close(launch_fd);

    {
        ProcessManager fail_closed(nullptr, socket_path, catalog);
        fail_closed.forceCleanupIncompleteForTest();
        const int fd = conectar(socket_path);
        ipc::FrameReader reader;
        enviarStart(fd, "cleanup-incomplete", "exit-zero", exit_zero);
        const json failed = receberEvento(fd, reader, 2s);
        exigir(failed.value("code", "") == "process_cleanup_incomplete", "cleanup incompleto deve bloquear launch");
        close(fd);
    }

    LogCapture logs;
    ProcessManager launch_failures(criarLogger(logs), socket_path, catalog);
    const int fd = conectar(socket_path);
    ipc::FrameReader reader;
    for (const char* stage : {"setpgid", "fchdir"}) {
        setenv("DACC_PM_TEST_CHILD_FAILURE", stage, 1);
        enviarStart(fd, std::string("failure-") + stage, "exit-zero", exit_zero);
        const json failed = receberEvento(fd, reader, 2s);
        exigir(failed.value("event", "") == "game_start_failed", "falha de setup deve impedir launch");
        unsetenv("DACC_PM_TEST_CHILD_FAILURE");
    }
    launch_failures.forceRegistrationFailureForTest();
    enviarStart(fd, "registration-failure", "exit-zero", exit_zero);
    const json registration = receberEvento(fd, reader, 2s);
    exigir(registration.value("code", "") == "state_registration_failed", "falha de registro deve ser explicita");
    const std::regex pid_pattern(R"(PID=([0-9]+))");
    std::smatch match;
    const std::string log_text = logs.str();
    exigir(std::regex_search(log_text, match, pid_pattern), "launch deve registrar PID antes da falha injetada");
    exigirProcessoAusente(static_cast<pid_t>(std::stoi(match[1].str())));
    exigirExecucao(fd, reader, "after-launch-failures", "exit-zero", exit_zero, 0);
    close(fd);
}

void testarRobustezDoIpc() {
    test_support::resetSendTestDouble();
    TempDirectory temp;
    const std::string exit_zero = temp.path() + "/exit-zero.sh";
    const std::string socket_path = temp.path() + "/daemon.sock";
    criarScript(exit_zero, "exit 0\n");
    ProcessManager manager(nullptr, socket_path, criarCatalogo(temp.path()));

    int fd = conectar(socket_path);
    ipc::FrameReader reader;
    const std::string invalid_json = "{not-json}\n";
    enviarBytes(fd, invalid_json);
    exigir(!tentarReceberEvento(fd, reader, 150ms).has_value(), "JSON invalido nao deve iniciar processo");
    const std::string command = json{{"action", "start"}, {"request_id", "partial-frame"}, {"game_id", "exit-zero"}}.dump() + "\n";
    const auto split = command.size() / 2;
    enviarBytes(fd, command.substr(0, split));
    exigir(!tentarReceberEvento(fd, reader, 100ms).has_value(), "frame parcial deve aguardar delimitador");
    enviarBytes(fd, command.substr(split));
    const json started = receberEvento(fd, reader, 2s);
    const json finished = receberEvento(fd, reader, 2s);
    exigir(started.value("event", "") == "game_started", "frame parcial completo deve iniciar");
    exigir(finished.value("event", "") == "game_finished", "frame parcial deve terminar normalmente");
    close(fd);

    fd = conectar(socket_path);
    enviarBytes(fd, std::string(ipc::kMaxFrameBytes + 1, 'x') + "\n");
    exigirSocketFechado(fd, 2s);
    close(fd);

    fd = conectar(socket_path);
    enviarComando(fd, {{"action", "start"}, {"request_id", "../invalid"}, {"game_id", "exit-zero"}});
    exigirSocketFechado(fd, 2s);
    close(fd);

    fd = conectar(socket_path);
    close(fd); // desconexao abrupta sem frame completo
}

} // namespace

int main() {
    testarExecucoesDeterministicas();
    testarPrazoDaConfirmacaoDeExec();
    testarEncerramentoAoDesconectar();
    testarFalhaAoEntregarGameStarted();
    testarFalhaAoEntregarGameStartFailed();
    testarFalhaAoEntregarGameFinished();
    testarEscritaParcialDeGameStarted();
    testarEscritaParcialDeGameStartFailed();
    testarEscritaParcialDeGameFinished();
    testarGeracaoEmFdReutilizado();
    testarCatalogoEProtocoloAutoritativo();
    testarSaidasSinaisEEventoUnico();
    testarUnicoJogoEConcorrencia();
    testarDescendentesTermKillEShutdown();
    testarDestrutorDuranteLaunchECleanupIncompleto();
    testarRobustezDoIpc();
    std::cout << "Process Manager integration tests passed." << std::endl;
    return 0;
}
