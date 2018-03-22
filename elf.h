#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/wait.h>

typedef struct {
  char* executable;  // Name of the program
  uint8_t* memory;  // Pointer to the program in memory
  Elf64_Addr entry;  // Entry point into the program
  Elf64_Ehdr* ehdr;  // Pointer to the ELF header
  Elf64_Phdr* phdr;  // Program header
  Elf64_Shdr* shdr;  // Section header
  Elf64_Shdr* strhdr;  // String section header
  Elf64_Shdr* symhdr;  // Symbol table section
  struct user_regs_struct registers;
} elf64;

void print_symbols(elf64 *elf);
void read_elf64(elf64 *elf, char *program_name);

