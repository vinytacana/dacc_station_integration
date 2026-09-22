#include "GameCatalog.hpp"
#include "json.hpp"

#include <cassert>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

template<class F> void rejected(F action) {
    bool failed = false;
    try { action(); } catch (const std::exception&) { failed = true; }
    assert(failed);
}

void writeFile(const fs::path& path, const std::string& contents, mode_t mode = 0600) {
    std::ofstream output(path, std::ios::binary);
    assert(output.is_open());
    output << contents;
    output.close();
    assert(chmod(path.c_str(), mode) == 0);
}

int main() {
    char pattern[] = "/tmp/dacc-catalog-test-XXXXXX";
    const char* made = mkdtemp(pattern);
    assert(made);
    const fs::path root(made);
    const fs::path executable = root / "fake-game";
    writeFile(executable, "#!/bin/sh\nexit 0\n", 0700);

    const fs::path valid = root / "valid.json";
    writeFile(valid, json{{"games", json::array({
        {{"id", "valid-game"}, {"argv", json::array({"fake-game", "literal;touch", "$(false)"})}, {"cwd", "."}}
    })}}.dump());
    GameCatalog catalog(valid.string());
    const GameCommand* game = catalog.find("valid-game");
    assert(game && game->argv.size() == 3);
    assert(game->argv[1] == "literal;touch" && game->argv[2] == "$(false)");
    assert(GameCatalog::validateExecutable(*game) == 0);
    assert(catalog.find("unknown") == nullptr);

    const fs::path malformed = root / "malformed.json";
    writeFile(malformed, json{{"games", json::array({
        {{"id", "../bad"}, {"argv", json::array({"fake-game"})}}
    })}}.dump());
    rejected([&] { GameCatalog value(malformed.string()); });

    const fs::path duplicate = root / "duplicate.json";
    writeFile(duplicate, json{{"games", json::array({
        {{"id", "same"}, {"argv", json::array({"fake-game"})}},
        {{"id", "same"}, {"argv", json::array({"fake-game"})}}
    })}}.dump());
    rejected([&] { GameCatalog value(duplicate.string()); });

    const fs::path oversized = root / "oversized.json";
    writeFile(oversized, std::string(1024 * 1024 + 1, 'x'));
    rejected([&] { GameCatalog value(oversized.string()); });

    assert(chmod(valid.c_str(), 0620) == 0);
    rejected([&] { GameCatalog value(valid.string()); });
    assert(chmod(valid.c_str(), 0600) == 0);

    const fs::path catalog_link = root / "catalog-link.json";
    assert(symlink(valid.c_str(), catalog_link.c_str()) == 0);
    rejected([&] { GameCatalog value(catalog_link.string()); });

    GameCommand missing{{(root / "missing").string()}, root.string()};
    assert(GameCatalog::validateExecutable(missing) == ENOENT);
    const fs::path noexec = root / "noexec";
    writeFile(noexec, "data", 0600);
    GameCommand not_executable{{noexec.string()}, root.string()};
    assert(GameCatalog::validateExecutable(not_executable) == EACCES);
    GameCommand missing_cwd{{executable.string()}, (root / "missing-cwd").string()};
    assert(GameCatalog::validateExecutable(missing_cwd) == ENOENT);
    const fs::path denied_cwd = root / "denied-cwd";
    assert(mkdir(denied_cwd.c_str(), 0000) == 0);
    if (geteuid() != 0) {
        GameCommand inaccessible{{executable.string()}, denied_cwd.string()};
        assert(GameCatalog::validateExecutable(inaccessible) == EACCES);
    }

    assert(chmod(denied_cwd.c_str(), 0700) == 0);
    fs::remove_all(root);
    std::cout << "Game catalog security tests passed.\n";
}
