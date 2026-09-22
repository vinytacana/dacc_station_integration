#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct GameCommand {
    std::vector<std::string> argv;
    std::string working_directory;
};

bool validProtocolId(const std::string& id);
std::string defaultGameCatalogPath();

class GameCatalog {
public:
    explicit GameCatalog(const std::string& path);
    const GameCommand* find(const std::string& id) const;
    static int validateExecutable(const GameCommand& game);
private:
    std::unordered_map<std::string, GameCommand> games_;
};
