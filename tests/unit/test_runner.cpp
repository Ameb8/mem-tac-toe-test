#include "mem_tac_review/pty_session.hpp"
#include "mem_tac_review/session_runner.hpp"
#include "mem_tac_review/scenario.hpp"

#include <cassert>
#include <iostream>
#include <thread>

using namespace mem_tac_review;

void test_pty_echo() {
    PtyProcess proc;
    std::vector<std::string> cmd = {"/bin/sh", "-c", "read line; echo \"RECEIVED: $line\""};
    bool ok = proc.spawn(cmd, ".", {}, "test_conn");
    assert(ok);
    assert(proc.is_running());

    proc.send_input("hello_pty\n");

    // Wait and read output
    std::string out;
    for (int i = 0; i < 50; ++i) {
        std::string s = proc.read_available();
        out += s;
        if (out.find("RECEIVED: hello_pty") != std::string::npos) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    assert(out.find("RECEIVED: hello_pty") != std::string::npos);
    proc.terminate();
    assert(!proc.is_running());
}

void test_pty_termination() {
    PtyProcess proc;
    std::vector<std::string> cmd = {"/bin/sh", "-c", "sleep 60"};
    bool ok = proc.spawn(cmd, ".", {}, "test_sleep");
    assert(ok);
    assert(proc.is_running());

    proc.terminate();
    assert(!proc.is_running());
    assert(proc.has_exited());
}

void test_runner_build() {
    SessionRunner runner;
    Profile p;
    p.set_build_cmd("echo 'Mock build success'");
    auto res = runner.build(p);
    assert(res.success);
    assert(res.exit_code == 0);
    assert(res.output.find("Mock build success") != std::string::npos);

    // Failing build test
    Profile bad_p;
    bad_p.set_build_cmd("exit 42");
    auto bad_res = runner.build(bad_p);
    assert(!bad_res.success);
    assert(bad_res.exit_code == 42);
}

int main() {
    test_pty_echo();
    test_pty_termination();
    test_runner_build();

    std::cout << "All PtySession and SessionRunner tests passed!" << std::endl;
    return 0;
}
