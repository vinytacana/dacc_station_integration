#include "GameCatalog.hpp"
#include "json.hpp"
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

bool validProtocolId(const std::string& id) {
    if (id.empty() || id.size() > 128) return false;
    for (unsigned char ch : id) {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.')) return false;
    }
    return true;
}

std::string defaultGameCatalogPath() {
    return (fs::read_symlink("/proc/self/exe").parent_path().parent_path() /
            "process-manager/games.json").string();
}

GameCatalog::GameCatalog(const std::string& path) {
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) throw std::runtime_error("catalog_unsafe: cannot open trusted regular file");
    struct stat st{};
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size > 1024 * 1024 ||
        (st.st_uid != geteuid() && st.st_uid != 0) || (st.st_mode & 0022) != 0) {
        close(fd);
        throw std::runtime_error("catalog_unsafe: require regular, trusted-owner, non-group/world-writable file <= 1 MiB");
    }
    std::string contents;
    contents.resize(static_cast<std::size_t>(st.st_size));
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const ssize_t count = read(fd, contents.data() + offset, contents.size() - offset);
        if (count > 0) offset += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else { close(fd); throw std::runtime_error("catalog_read_failed"); }
    }
    close(fd);
    auto root = nlohmann::json::parse(contents);
    if (!root.is_object() || !root.contains("games") || !root["games"].is_array() ||
        root["games"].size() > 1024) throw std::runtime_error("catalog_invalid");
    const auto base = fs::absolute(path).parent_path();
    for (const auto& entry : root["games"]) {
        const auto id = entry.at("id").get<std::string>();
        GameCommand game{entry.at("argv").get<std::vector<std::string>>(), entry.value("cwd", ".")};
        if (!validProtocolId(id) || game.argv.empty() || game.argv.size() > 128 ||
            game.argv[0].empty() || game.working_directory.empty() ||
            game.working_directory.find('\0') != std::string::npos) throw std::runtime_error("catalog_invalid");
        for (const auto& arg : game.argv) {
            if (arg.size() > 4096 || arg.find('\0') != std::string::npos) throw std::runtime_error("catalog_invalid");
        }
        game.working_directory = (base / game.working_directory).lexically_normal().string();
        // Only the server config selects argv and cwd; executable lookup never uses client PATH.
        game.argv[0] = (base / game.argv[0]).lexically_normal().string();
        if (!games_.emplace(id, std::move(game)).second) throw std::runtime_error("catalog_duplicate_id");
    }
}

const GameCommand* GameCatalog::find(const std::string& id) const {
    auto it = games_.find(id);
    return it == games_.end() ? nullptr : &it->second;
}

int GameCatalog::validateExecutable(const GameCommand& game) {
    struct stat st{};
    if (stat(game.argv[0].c_str(), &st) != 0) return errno;
    if (!S_ISREG(st.st_mode)) return EACCES;
    if (access(game.argv[0].c_str(), X_OK) != 0) return errno;
    if (stat(game.working_directory.c_str(), &st) != 0) return errno;
    if (!S_ISDIR(st.st_mode)) return ENOTDIR;
    if (access(game.working_directory.c_str(), X_OK) != 0) return errno;
    return 0;
}
