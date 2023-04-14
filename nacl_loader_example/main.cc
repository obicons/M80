#include <cassert>
#include <cstdio>
#include <iostream>
#include "elfio/elfio.hpp"

extern "C" {
#include <sys/mman.h>
}

using namespace std;

bool setup_elf(const string &file);

int main(int argc, char **argv) {
    puts("starting...");
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " [elf file]" << endl;
        return 1;
    }

    if (setup_elf(argv[1]))
        return 1;
}

bool setup_elf(const string &file) {
    ELFIO::elfio reader;
    if (!reader.load(file)) {
        cerr << "Can't find or process ELF file " << file << endl;
        return true;
    }

    size_t total_section_size = 0;
    for (int i = 0; i < reader.sections.size(); i++) {
        const ELFIO::section *psec = reader.sections[i];
        total_section_size += psec->get_size();
    }

    char *executable_space =
        (char*) mmap((void*) (0xffffffffUL / 4096UL * 4096UL + 4096UL), 0xffffffff,
                     PROT_READ | PROT_WRITE | PROT_EXEC, 
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (executable_space == MAP_FAILED) {
        perror("mmap()");
        return false;
    }

    const unsigned long trampoline_offset = (0xfffffffful / 32ul) * 32ul - 64;

    int (*f)(void) = nullptr;
    for (int i = 0; i < reader.sections.size(); i++) {
        ELFIO::section *pspec = reader.sections[i];
        if (pspec->get_type() == ELFIO::SHT_SYMTAB ) {
            const ELFIO::symbol_section_accessor symbols(reader, pspec);
            for (unsigned int j = 0; j < symbols.get_symbols_num(); ++j) {
                std::string name;
                ELFIO::Elf64_Addr value;
                ELFIO::Elf_Xword size;
                unsigned char bind;
                unsigned char type;
                ELFIO::Elf_Half section_index;
                unsigned char other;
                symbols.get_symbol(j, name, value, size, bind,
                                   type, section_index, other);
                if (name == "f") {
                    f = (int(*)(void)) (executable_space + value);
                }
            }
        } 

        if (pspec->get_name() == ".text") {
            const char *data = pspec->get_data();
            if (data != NULL) {
                assert(pspec->get_address() + pspec->get_size() < trampoline_offset);
                memcpy(executable_space + pspec->get_address(), data, pspec->get_size());
            }
        }
    }

    char trampoline_data[] = 
        "\x55"
        "\x48\x89\xe5"
        "\x90\x90\x90\x90"
        "\x90\x90\x90\x90"
        "\x90\x90\x90\x90"
        "\x90\x90\x90\x90"
        "\x90\x90\x90\x90"
        "\x90\x90\x90\x90"
        "\xff\x54\x24\x10"
        "\x5d"
        "\xc3";
    char *trampoline_addr = executable_space + trampoline_offset;
    memcpy(trampoline_addr, trampoline_data, sizeof(trampoline_data));

    char *stack_top = trampoline_offset - 16 + executable_space;
    if (f != nullptr) {
        int result = -42;
        __asm__ volatile(
            "movq %1, %%r15\n\t"
            "pushq %%rbp\n\t"
            "movq %%rsp, %%rbp\n\t"
            "movq %3, %%rsp\n\t"
            "pushq %2\n\t"
            "callq *%4\n\t"
            "movl %%eax, %0\n\t"
            "movq %%rbp, %%rsp\n\t"
            "popq %%rbp\n\t"
            : "=r"(result)
            : "r"(executable_space), "r"(f), "r"(stack_top), "r"(trampoline_addr)
            : "%r15"
        );
        printf("Got: %d\n", result);
    }

    return false;
}
