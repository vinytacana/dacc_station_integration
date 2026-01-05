#include "ProcessManager.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h> 
#include <sys/signalfd.h> 
#include <signal.h>
#include <sys/wait.h>

#include <cstdlib>
#include <cstring>
#include <cerrno> 
#include <vector>
#include <stdexcept>
#include <thread>


ProcessManager::ProcessManager(std::unique_ptr<ILogger> logger) : logger_{std::move(logger)} {

    // a data type to represent multiple signals
    // "signal_mask" is showing wich signals are blocked
    // all blocked signals can't reach the parent process
    sigset_t signal_mask;
    
    // clean initializing 
    sigemptyset(&signal_mask);

    // adding "SIGCHILD" to blocked signals list
    sigaddset(&signal_mask, SIGCHLD);

    // other threads created will inherit a copy of the signal mask
    if (pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr) != 0) {
        throw std::runtime_error("Failed to block SIGCHLD");
    }

    // If the fd argument is -1, then the call creates a new file descriptor and associates the signal set specified in mask with that file descriptor.
    signal_fd_ = signalfd(-1, &signal_mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (signal_fd_ == -1) {
        throw std::runtime_error("Failed to create signalfd");
    }

    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);   
    if (epoll_fd_ == -1) {
        close(signal_fd_);
        throw std::runtime_error("Failed to create epoll instance");
    }

    epoll_event event{};
    event.events = EPOLLIN; 
    event.data.fd = signal_fd_;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_fd_, &event) == -1) {
        close(epoll_fd_);
        close(signal_fd_);
        throw std::runtime_error("Failed to add signalfd to epoll");
    }

    std::future<void> ready_future = ready_promise.get_future();
    try {
        event_loop_thread_ = std::thread(&ProcessManager::eventLoop, this);
    
    } catch (const std::system_error& e) {

        close(epoll_fd_);
        close(signal_fd_);

        throw std::runtime_error("Failed to start event loop thread: " + std::string(e.what()));
    }
    ready_future.get();
}

ProcessManager::~ProcessManager() {
    if (logger_) logger_->log("Shutting down ProcessManager...");
    stop_loop_ = true;

    // Se a thread estiver "juntável" (ou seja, não foi movida, etc.)
    if (event_loop_thread_.joinable()) {
        // Espera a thread terminar sua execução.
        // O loop vai sair porque stop_loop_ é true.
        event_loop_thread_.join();
    }

    // Fecha os file descriptors para liberar os recursos
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    if (signal_fd_ != -1) {
        close(signal_fd_);
    }
    if (logger_) logger_->log("ProcessManager shut down cleanly.");
}

void ProcessManager::eventLoop() {
    // sinaliza que a thread esta pronta
    ready_promise.set_value();
    epoll_event events[MAX_EVENTS];
    if (logger_) logger_->log("Event loop started and is ready.");
    
    while (!stop_loop_) {
        int n_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
        if (n_events < 0){
            if (errno == EINTR) // Se for interrompido por um sinal, apenas continue o loop
                continue;
            // Para outros erros, saia do loop.
            if (logger_) logger_->log("epoll_wait error: " + std::string(strerror(errno)));
            break;
        }

        for (int i = 0; i < n_events; ++i) {
            if (events[i].data.fd == signal_fd_) {
                if (logger_) logger_->log("received signal on signal_fd. Handling child signal");
                handleChildSignal();
            }
        }
    }
    if (logger_) logger_->log("Event loop finished.");
}

void ProcessManager::handleChildSignal() {
    signalfd_siginfo ssi;
        while (read(signal_fd_, &ssi, sizeof(ssi)) == sizeof(ssi)) {
        // continue lendo ate que não haja mais nada
    }
    
    int status;
    pid_t child_pid;
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        auto end_time = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(processes_mutex_);
        auto it = running_processes_.find(child_pid);
        
        if (it != running_processes_.end()) {
            auto start_time = it->second;
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
            
            if (logger_) {
                logger_->log("Process PID=" + std::to_string(child_pid) + " finished.");
                logger_->log("Total execution time: " + std::to_string(duration.count()) + " seconds.");
            }
            
            running_processes_.erase(it);
        }
        else{
            if (logger_) logger_->log("Reaped untracked child process PID=" + std::to_string(child_pid));
        }
    }
}

pid_t ProcessManager::createProcess(){
    pid_t child_process;
    child_process = fork();
    switch (child_process)
    {
    case -1:
        throw std::runtime_error("fork() failed: " + std::string(strerror(errno)));
    case 0:
        return child_process; // returning 0   CHILD
    default:
        return child_process; // returning > 0 PARENT
        break;
    }
}

bool ProcessManager::isAppValid(const ApplicationDefinition& app) const {
    const std::string& app_path = app.getPath();

    if (app_path.empty()){ 
        if (logger_) logger_->log("Application executable path is empty for app_id: " + app.getId());
        return false;
    }
    
    return true;
}
    

pid_t ProcessManager::startApplication(ApplicationDefinition& app){
    if(not isAppValid(app)) return -1;    
    
    pid_t child_pid;
    try{   
        child_pid = createProcess();
    }catch(const std::runtime_error& e)
    {
        if (logger_) logger_->log(e.what());
        return -1;
    }
    
    if (child_pid == 0){
        // Child process
        // execlp espera (file, arg0, arg1, ..., NULL)
        // arg0 deve ser o nome do arquivo sendo executado por convenção
        execlp(app.getPath().c_str(), app.getId().c_str(), (char *) NULL);
        
        // Se execlp retornar, houve erro.
        // Como o logger não é fork-safe (sockets/fd), melhor usar stderr ou exit direto
        std::cerr << "CRITICAL: execlp failed for " << app.getPath() << ": " << strerror(errno) << std::endl;
        _exit(EXIT_FAILURE);
        return -1; // Não alcançado
    }

    // parent process
    auto start_time = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(processes_mutex_);
        running_processes_[child_pid] = start_time;
    }

    if (logger_) {
        logger_->log("App " + app.getId() + " started successfully");
        logger_->log("PID=" + std::to_string(child_pid));
    }
    
    return child_pid;
}   

