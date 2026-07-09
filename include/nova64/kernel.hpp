#pragma once

#include "nova64/assembler.hpp"
#include "nova64/common.hpp"
#include "nova64/cpu.hpp"

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace nova64 {
struct process_entry {
    std::string name;
    std::uint32_t pid{};
    std::uint64_t cycles{};
    std::string state;
};

class filesystem {
public:
    void mkdir(const std::string& path);
    bool exists(const std::string& path) const;
    bool is_dir(const std::string& path) const;
    std::vector<std::string> list_dir(const std::string& path) const;
    void write_file(const std::string& path, const std::string& contents);
    std::string read_file(const std::string& path) const;
    void remove(const std::string& path);
    std::string pwd() const;
    void cd(const std::string& path);

private:
    std::map<std::string, std::string> files_;
    std::map<std::string, bool> dirs_;
    std::string cwd_ = "/";
};

class kernel {
public:
    explicit kernel(memory& mem, cpu& cpu);
    void boot();
    void handle_syscall(std::uint64_t id);
    void shell();
    void log(const std::string& message);
    std::vector<std::string> log_entries() const;
    std::vector<process_entry> processes() const;
    std::string cpu_info() const;
    std::string mem_info() const;
    void run_build(const std::string& target);
    void show_monitor() const;
    void open_editor(const std::string& path);

private:
    void run_command(const std::string& line);
public:
    void process_command(const std::string& line);

private:
    std::vector<std::string> history_;
    std::vector<std::string> log_;
    std::vector<process_entry> processes_;
    memory& mem_;
    cpu& cpu_;
    filesystem fs_;
    bool running_ = true;
};
}  // namespace nova64
