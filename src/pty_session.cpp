#include "mem_tac_review/pty_session.hpp"

#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

namespace mem_tac_review {

PtyProcess::PtyProcess() = default;

PtyProcess::~PtyProcess() {
    terminate();
}

PtyProcess::PtyProcess(PtyProcess&& other) noexcept
    : master_fd_(other.master_fd_),
      pid_(other.pid_),
      pgid_(other.pgid_),
      connection_name_(std::move(other.connection_name_)),
      exited_(other.exited_),
      exit_code_(other.exit_code_),
      was_signaled_(other.was_signaled_),
      term_signal_(other.term_signal_),
      accumulated_output_(std::move(other.accumulated_output_)) {
    other.master_fd_ = -1;
    other.pid_ = -1;
    other.pgid_ = -1;
}

PtyProcess& PtyProcess::operator=(PtyProcess&& other) noexcept {
    if (this != &other) {
        terminate();
        master_fd_ = other.master_fd_;
        pid_ = other.pid_;
        pgid_ = other.pgid_;
        connection_name_ = std::move(other.connection_name_);
        exited_ = other.exited_;
        exit_code_ = other.exit_code_;
        was_signaled_ = other.was_signaled_;
        term_signal_ = other.term_signal_;
        accumulated_output_ = std::move(other.accumulated_output_);

        other.master_fd_ = -1;
        other.pid_ = -1;
        other.pgid_ = -1;
    }
    return *this;
}

bool PtyProcess::spawn(const std::vector<std::string>& cmd,
                       const std::string& work_dir,
                       const std::map<std::string, std::string>& env,
                       const std::string& connection_name) {
    if (cmd.empty()) {
        return false;
    }

    connection_name_ = connection_name;
    exited_ = false;
    exit_code_ = 0;
    was_signaled_ = false;
    term_signal_ = 0;
    accumulated_output_.clear();

    int master_fd = -1;
    int slave_fd = -1;

    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) < 0) {
        return false;
    }

    // Set master non-blocking
    int flags = fcntl(master_fd, F_GETFL, 0);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    pid_t pid = fork();
    if (pid < 0) {
        close(master_fd);
        close(slave_fd);
        return false;
    }

    if (pid == 0) {
        // Child process
        close(master_fd);

        // Become session leader
        setsid();
        ioctl(slave_fd, TIOCSCTTY, 0);

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);

        setpgid(0, 0);

        // Change directory if needed
        if (!work_dir.empty() && work_dir != ".") {
            if (chdir(work_dir.c_str()) != 0) {
                // If chdir fails, exit with error
                _exit(126);
            }
        }

        // Apply environment variables
        for (const auto& [k, v] : env) {
            setenv(k.c_str(), v.c_str(), 1);
        }

        if (cmd.size() == 1) {
            // Execute via shell to support command strings
            execl("/bin/sh", "sh", "-c", cmd[0].c_str(), nullptr);
        } else {
            std::vector<char*> argv;
            for (const auto& arg : cmd) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
        }

        _exit(127);
    }

    // Parent process
    close(slave_fd);
    master_fd_ = master_fd;
    pid_ = pid;
    pgid_ = pid; // child sets pgid to its pid
    return true;
}

bool PtyProcess::is_running() {
    if (pid_ <= 0) return false;
    if (exited_) return false;
    check_status(false);
    return !exited_;
}

void PtyProcess::send_input(const std::string& text) {
    if (master_fd_ < 0 || exited_) return;
    size_t total_written = 0;
    while (total_written < text.size()) {
        ssize_t n = write(master_fd_, text.data() + total_written, text.size() - total_written);
        if (n > 0) {
            total_written += n;
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            break;
        } else {
            break;
        }
    }
}

void PtyProcess::close_input() {
    if (master_fd_ < 0 || exited_) return;
    // Send EOF (Ctrl-D) to the pseudo terminal
    char eof_char = 4; // ASCII EOT (Ctrl-D)
    (void)write(master_fd_, &eof_char, 1);
}

std::string PtyProcess::read_available() {
    if (master_fd_ < 0) return "";

    std::string result;
    char buffer[4096];

    while (true) {
        ssize_t count = read(master_fd_, buffer, sizeof(buffer));
        if (count > 0) {
            result.append(buffer, count);
        } else if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else if (errno == EIO) {
                // Under Linux, EIO on master PTY indicates slave side has been closed
                check_status(false);
                break;
            } else {
                break;
            }
        } else {
            // EOF reached
            check_status(false);
            break;
        }
    }

    if (!result.empty()) {
        accumulated_output_ += result;
    }

    return result;
}

void PtyProcess::check_status(bool blocking) {
    if (pid_ <= 0 || exited_) return;

    int status = 0;
    pid_t ret = waitpid(pid_, &status, blocking ? 0 : WNOHANG);
    if (ret == pid_) {
        exited_ = true;
        if (WIFEXITED(status)) {
            exit_code_ = WEXITSTATUS(status);
            was_signaled_ = false;
        } else if (WIFSIGNALED(status)) {
            was_signaled_ = true;
            term_signal_ = WTERMSIG(status);
            exit_code_ = 128 + term_signal_;
        }
    }
}

void PtyProcess::terminate() {
    if (pid_ <= 0) {
        if (master_fd_ >= 0) {
            close(master_fd_);
            master_fd_ = -1;
        }
        return;
    }

    check_status(false);

    if (!exited_ && pgid_ > 0) {
        // Send SIGTERM to entire process group
        kill(-pgid_, SIGTERM);

        // Wait up to 300ms for graceful shutdown
        for (int i = 0; i < 30; ++i) {
            check_status(false);
            if (exited_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Escalate to SIGKILL if still running
        if (!exited_) {
            kill(-pgid_, SIGKILL);
            check_status(true);
        }
    }

    // Read any remaining output buffered in master
    read_available();

    if (master_fd_ >= 0) {
        close(master_fd_);
        master_fd_ = -1;
    }
    pid_ = -1;
    pgid_ = -1;
}

} // namespace mem_tac_review
