#pragma once
#include <memory>
#include <optional>
#include <string>

extern "C" {
#include <sys/mman.h>
}

const unsigned long executable_space_size = 0xffffffffUL;

class NaClContext {
public:
    std::optional<std::string> call();

    // May return nullptr if something fails.
    static std::unique_ptr<NaClContext> create_context(const std::string &executable);

    ~NaClContext() {
        munmap(executable_space_start, executable_space_size);
    }

    NaClContext(char *executable_space_start, char *f) :
        executable_space_start(executable_space_start), f(f) {}

    NaClContext(const NaClContext &other) = delete;
    NaClContext(const NaClContext &&other) = delete;
    NaClContext& operator=(const NaClContext &other) = delete;

private:
    char *executable_space_start;
    char *f;
};