#include "nova64/kernel.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace nova64 {
namespace {
std::string trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return begin < end ? std::string(begin, end) : std::string{};
}
}

void filesystem::mkdir(const std::string& path) {
    dirs_[path] = true;
}

bool filesystem::exists(const std::string& path) const {
    return files_.count(path) || dirs_.count(path);
}

bool filesystem::is_dir(const std::string& path) const {
    return dirs_.count(path);
}

std::vector<std::string> filesystem::list_dir(const std::string& path) const {
    std::vector<std::string> items;
    for (const auto& [name, _] : files_) {
        if (name.rfind(path, 0) == 0) {
            items.push_back(name);
        }
    }
    for (const auto& [name, _] : dirs_) {
        if (name.rfind(path, 0) == 0) {
            items.push_back(name + "/");
        }
    }
    return items;
}

void filesystem::write_file(const std::string& path, const std::string& contents) {
    files_[path] = contents;
}

std::string filesystem::read_file(const std::string& path) const {
    auto it = files_.find(path);
    if (it == files_.end()) {
        return {};
    }
    return it->second;
}

void filesystem::remove(const std::string& path) {
    files_.erase(path);
    dirs_.erase(path);
}

std::string filesystem::pwd() const { return cwd_; }
void filesystem::cd(const std::string& path) { cwd_ = path.empty() ? "/" : path; }

kernel::kernel(memory& mem, cpu& cpu) : mem_(mem), cpu_(cpu) {}

void kernel::boot() {
    log("Nova64 boot ROM initialized");
    for (std::uint32_t i = 0; i < 3; ++i) {
        processes_.push_back(process_entry{"init", 100u + i, 0u, "ready"});
    }
}

void kernel::handle_syscall(std::uint64_t id) {
    std::ostringstream out;
    out << "syscall " << id;
    log(out.str());
}

void kernel::shell() {
    std::cout << "================================================\n";
    std::cout << "Nova64 integrated shell\n";
    std::cout << "Type 'help' for commands, 'edit <file>' to open the editor, 'build <target>' to assemble, or 'monitor' for live system info.\n";
    std::cout << "================================================\n";
    while (running_) {
        std::cout << "novaos> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        run_command(line);
    }
}

void kernel::log(const std::string& message) { log_.push_back(message); }
std::vector<std::string> kernel::log_entries() const { return log_; }
std::vector<process_entry> kernel::processes() const { return processes_; }
std::string kernel::cpu_info() const { return "Nova64 CPU @ 1GHz"; }
std::string kernel::mem_info() const { return "16 MiB RAM"; }

void kernel::process_command(const std::string& line) {
    run_command(line);
}

void kernel::run_command(const std::string& line) {
    const auto command = trim(line);
    if (command == "help") {
        log("help echo clear ls cd mkdir touch cat rm cp mv pwd edit time date meminfo cpuinfo reboot shutdown build monitor newasm run debug trace");
    } else if (command == "cpuinfo") {
        log(cpu_info());
    } else if (command == "meminfo") {
        log(mem_info());
    } else if (command == "monitor") {
        show_monitor();
    } else if (command.rfind("edit ", 0) == 0) {
        open_editor(command.substr(5));
    } else if (command.rfind("build ", 0) == 0) {
        run_build(command.substr(6));
    } else if (command == "newasm") {
        fs_.write_file("main.asm", "start:\n    loadi r0 1\n    hlt\n");
        log("created main.asm");
    } else if (command == "run") {
        log("run <program.asm> available from the shell launcher");
    } else if (command == "debug") {
        log("debug trace enabled");
        cpu_.set_trace(true);
    } else if (command == "trace") {
        const auto traces = cpu_.trace_log();
        for (const auto& item : traces) {
            log(item);
        }
    } else if (command == "reboot") {
        log("reboot requested");
    } else if (command == "shutdown") {
        running_ = false;
        log("shutdown requested");
    } else {
        log("unknown command: " + command);
    }
}

void kernel::run_build(const std::string& target) {
    std::ostringstream out;
    out << "building target: " << target;
    log(out.str());
    try {
        assembler as;
        const auto exe = as.assemble(target);
        out.str("");
        out << "built " << exe.sections.size() << " section(s) from " << target;
        log(out.str());
    } catch (const std::exception& ex) {
        log(std::string("build failed: ") + ex.what());
    }
}

void kernel::show_monitor() const {
    std::cout << "[monitor] cpu cycles: " << cpu_.cycle_count() << "\n";
    std::cout << "[monitor] pc: 0x" << std::hex << cpu_.pc() << std::dec << "\n";
    std::cout << "[monitor] sp: 0x" << std::hex << cpu_.sp() << std::dec << "\n";
    std::cout << "[monitor] flags: 0x" << std::hex << cpu_.flags() << std::dec << "\n";
}

void kernel::open_editor(const std::string& path) {
    std::ifstream stream(path);
    std::string contents;
    if (stream) {
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        contents = buffer.str();
    }
    std::cout << "[editor] opening " << path << "\n";
    std::cout << "[editor] --- built-in terminal editor ---\n";
    std::cout << "[editor] commands: save, quit, line, search <term>\n";
    std::string line;
    std::istringstream input(contents);
    std::vector<std::string> lines;
    std::string current;
    while (std::getline(input, current)) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::cout << (i + 1) << ": " << lines[i] << "\n";
    }
    std::cout << "[editor] ready\n";
    while (true) {
        std::cout << "editor> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (line == "quit") {
            break;
        }
        if (line == "save") {
            std::ofstream out(path);
            for (const auto& item : lines) {
                out << item << "\n";
            }
            std::cout << "[editor] saved " << path << "\n";
        } else if (line.rfind("search ", 0) == 0) {
            const auto term = line.substr(7);
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (lines[i].find(term) != std::string::npos) {
                    std::cout << "[editor] match line " << (i + 1) << ": " << lines[i] << "\n";
                }
            }
        } else if (line.rfind("line ", 0) == 0) {
            const auto index = std::stoll(line.substr(5)) - 1;
            if (index >= 0 && index < static_cast<long long>(lines.size())) {
                std::cout << "[editor] line " << (index + 1) << ": " << lines[index] << "\n";
            }
        } else {
            lines.push_back(line);
        }
    }
}
}  // namespace nova64
