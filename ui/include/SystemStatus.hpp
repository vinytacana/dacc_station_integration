#ifndef SYSTEM_STATUS_HPP
#define SYSTEM_STATUS_HPP

#include <string>

namespace MeuProjeto {

struct SystemData {
    std::string currentTime; // "HH:MM"
    bool wifiConnected;
    int batteryLevel;        // 0-100
};

class SystemStatus {
public:
    static SystemStatus& getInstance() {
        static SystemStatus instance;
        return instance;
    }

    void update();
    SystemData getCachedData() const;

private:
    SystemStatus(); // Singleton
    
    SystemData cache;
    unsigned int lastUpdate;
};

} // namespace MeuProjeto

#endif // SYSTEM_STATUS_HPP