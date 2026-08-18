#include "ipc/FramedSocket.hpp"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

using namespace std::chrono_literals;

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

struct SocketPair {
    std::array<int, 2> fd{-1, -1};

    SocketPair() {
        exigir(
            socketpair(AF_UNIX, SOCK_STREAM, 0, fd.data()) == 0,
            "socketpair deve ser criado"
        );
    }

    ~SocketPair() {
        for (int descriptor : fd) {
            if (descriptor >= 0) {
                close(descriptor);
            }
        }
    }

    void closePeer() {
        close(fd[1]);
        fd[1] = -1;
    }
};

std::string receberExato(int fd, std::size_t tamanho) {
    std::string recebido;
    recebido.resize(tamanho);
    std::size_t total = 0;
    while (total < tamanho) {
        const ssize_t count = recv(fd, recebido.data() + total, tamanho - total, 0);
        exigir(count > 0, "peer deve entregar todos os bytes esperados");
        total += static_cast<std::size_t>(count);
    }
    return recebido;
}

void testarMensagemUnica() {
    SocketPair sockets;
    const ipc::SendResult result = ipc::sendMessage(sockets.fd[0], "unica", 100ms);
    exigir(
        result.status == ipc::SendStatus::Ok,
        "mensagem unica deve ser enviada (status=" +
            std::to_string(static_cast<int>(result.status)) +
            ", errno=" + std::to_string(result.err) + ")"
    );
    exigir(result.bytes_sent == 6, "delimitador deve contar nos bytes enviados");

    const std::string bytes = receberExato(sockets.fd[1], result.bytes_sent);
    ipc::FrameReader reader;
    reader.feed(bytes.data(), bytes.size());
    auto frame = reader.next();
    exigir(frame && *frame == "unica", "mensagem unica deve ser reconstruida");
    exigir(!reader.next(), "nao deve existir frame adicional");
}

void testarMensagensConcatenadas() {
    SocketPair sockets;
    const std::string concatenadas = "primeira\nsegunda\nterceira\n";
    exigir(
        send(sockets.fd[0], concatenadas.data(), concatenadas.size(), MSG_NOSIGNAL) ==
            static_cast<ssize_t>(concatenadas.size()),
        "mensagens concatenadas devem ser escritas"
    );

    const std::string bytes = receberExato(sockets.fd[1], concatenadas.size());
    ipc::FrameReader reader;
    reader.feed(bytes.data(), bytes.size());
    exigir(reader.next() == std::optional<std::string>{"primeira"}, "primeiro frame incorreto");
    exigir(reader.next() == std::optional<std::string>{"segunda"}, "segundo frame incorreto");
    exigir(reader.next() == std::optional<std::string>{"terceira"}, "terceiro frame incorreto");
}

void testarMensagemFragmentadaPorByte() {
    SocketPair sockets;
    const std::string mensagem = "fragmentada\n";
    ipc::FrameReader reader;

    for (char byte : mensagem) {
        exigir(
            send(sockets.fd[0], &byte, 1, MSG_NOSIGNAL) == 1,
            "cada fragmento deve ser enviado"
        );
        char recebido = '\0';
        exigir(recv(sockets.fd[1], &recebido, 1, 0) == 1, "cada fragmento deve ser recebido");
        reader.feed(&recebido, 1);
    }

    exigir(
        reader.next() == std::optional<std::string>{"fragmentada"},
        "fragmentos de um byte devem formar uma mensagem"
    );
}

void testarLimiteDeMensagem() {
    SocketPair sockets;
    const std::string grande(ipc::kMaxFrameBytes + 1, 'x');
    const ipc::SendResult result = ipc::sendMessage(sockets.fd[0], grande, 100ms);
    exigir(result.status == ipc::SendStatus::TooLarge, "payload acima do limite deve falhar");
    exigir(result.bytes_sent == 0, "payload grande nao deve enviar bytes");

    ipc::FrameReader reader;
    reader.feed(grande.data(), grande.size());
    exigir(reader.overflowed(), "reader deve detectar frame acima do limite");
    exigir(reader.bufferedBytes() == 0, "reader deve liberar frame grande imediatamente");
}

void testarPeerFechado() {
    SocketPair sockets;
    sockets.closePeer();
    const ipc::SendResult result = ipc::sendMessage(sockets.fd[0], "sem-peer", 100ms);
    exigir(
        result.status == ipc::SendStatus::Disconnected,
        "peer fechado deve ser classificado como desconectado sem SIGPIPE"
    );
}

void reduzirBufferDeEnvio(int fd) {
    int tamanho = 1024;
    exigir(
        setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &tamanho, sizeof(tamanho)) == 0,
        "SO_SNDBUF deve ser configurado"
    );
}

void testarEnvioParcialAteConcluir() {
    SocketPair sockets;
    reduzirBufferDeEnvio(sockets.fd[0]);
    const std::string payload(ipc::kMaxFrameBytes, 'p');
    ipc::SendResult result;

    std::thread sender([&] {
        result = ipc::sendMessage(sockets.fd[0], payload, 2s);
    });

    std::this_thread::sleep_for(20ms);
    const std::string recebido = receberExato(sockets.fd[1], payload.size() + 1);
    sender.join();

    exigir(result.status == ipc::SendStatus::Ok, "envio parcial deve concluir dentro do prazo");
    exigir(result.bytes_sent == payload.size() + 1, "todos os bytes devem ser contabilizados");
    exigir(recebido.substr(0, payload.size()) == payload, "payload parcial deve chegar integro");
    exigir(recebido.back() == '\n', "payload parcial deve terminar com delimitador");
}

void testarPrazoAposEnvioParcial() {
    SocketPair sockets;
    reduzirBufferDeEnvio(sockets.fd[0]);
    const std::string payload(ipc::kMaxFrameBytes, 't');
    const auto inicio = std::chrono::steady_clock::now();
    const ipc::SendResult result = ipc::sendMessage(sockets.fd[0], payload, 50ms);
    const auto duracao = std::chrono::steady_clock::now() - inicio;

    exigir(result.bytes_sent > 0, "teste deve forcar ao menos um envio parcial");
    exigir(
        result.status == ipc::SendStatus::Desynced,
        "prazo expirado apos envio parcial deve invalidar o stream"
    );
    exigir(result.bytes_sent < payload.size() + 1, "timeout deve preservar contagem parcial");
    exigir(duracao >= 40ms && duracao < 500ms, "timeout deve respeitar prazo monotonicamente");
}

} // namespace

int main() {
    testarMensagemUnica();
    testarMensagensConcatenadas();
    testarMensagemFragmentadaPorByte();
    testarLimiteDeMensagem();
    testarPeerFechado();
    testarEnvioParcialAteConcluir();
    testarPrazoAposEnvioParcial();
    std::cout << "Framed socket tests passed." << std::endl;
    return 0;
}
