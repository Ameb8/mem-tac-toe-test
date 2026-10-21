#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <sys/types.h>

namespace mem_tac_review {

struct ProcessOutputChunk {
    std::string text;
    int64_t timestamp_ms;
    std::string connection; // "p1", "p2", "shared"
};

class PtyProcess {
public:
    PtyProcess();
    ~PtyProcess();

    PtyProcess(const PtyProcess&) = delete;
    PtyProcess& operator=(const PtyProcess&) = delete;

    PtyProcess(PtyProcess&& other) noexcept;
    PtyProcess& operator=(PtyProcess&& other) noexcept;

    bool spawn(const std::vector<std::string>& cmd,
               const std::string& work_dir,
               const std::map<std::string, std::string>& env,
               const std::string& connection_name);

    bool is_running();
    void send_input(const std::string& text);
    void close_input(); // Close stdin

    // Reads whatever is currently in the master buffer without blocking
    std::string read_available();

    // Sends SIGTERM, waits briefly, sends SIGKILL if needed, reaps PID
    void terminate();

    int master_fd() const noexcept { return master_fd_; }
    pid_t pid() const noexcept { return pid_; }
    pid_t pgid() const noexcept { return pgid_; }
    const std::string& connection_name() const noexcept { return connection_name_; }
    bool has_exited() const noexcept { return exited_; }
    int exit_code() const noexcept { return exit_code_; }
    bool was_signaled() const noexcept { return was_signaled_; }
    int term_signal() const noexcept { return term_signal_; }
    const std::string& accumulated_output() const noexcept { return accumulated_output_; }

private:
    void cleanup();
    void check_status(bool blocking = false);

    int master_fd_ = -1;
    pid_t pid_ = -1;
    pid_t pgid_ = -1;
    std::string connection_name_;
    bool exited_ = false;
    int exit_code_ = 0;
    bool was_signaled_ = false;
    int term_signal_ = 0;
    std::string accumulated_output_;
};

} // namespace mem_tac_review
