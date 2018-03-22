#include "elf.h"


void print_symbols(elf64 *elf) {
  // If no symbol table is present, this is a stripped binary
  if (elf->symhdr == NULL) {
    printf("Stripped Binary");
    return;
  }

  // Use the symbol table header to get the symbol table section
  Elf64_Sym* symtab = (Elf64_Sym*) &elf->memory[elf->symhdr->sh_offset];

  // Get the string table for this section
  char* strtab = (char*) &elf->memory[elf->shdr[elf->symhdr->sh_link].sh_offset];

  printf("Symbol table '.symtab' entries:\n");
  printf("   Num: Value            Name\n");

  for (int j = 0; j < elf->symhdr->sh_size / sizeof(Elf64_Sym); j++, symtab++) {
    printf("    %2d: %016lx %s\n", j, symtab->st_value, &strtab[symtab->st_name]);
  }
}


void read_elf64(elf64 *elf, char* program_name) {
  elf->executable = program_name;
  int fd = open(elf->executable, O_RDONLY);

  struct stat st;
  fstat(fd, &st);
  elf->memory = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (elf->memory == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  elf->ehdr = (Elf64_Ehdr*) elf->memory;  // Get pointer to the ELF header
  elf->phdr = (Elf64_Phdr*) (elf->memory + elf->ehdr->e_phoff);  // Get pointer to the first program header
  elf->shdr = (Elf64_Shdr*) (elf->memory + elf->ehdr->e_shoff);  // Get pointer to the first section header

  // Get the entry point to the program
  elf->entry = elf->ehdr->e_entry;

  if (elf->memory[0] != 0x7f && !strcmp((char*) &elf->memory[1], "ELF")) {
    printf("%s is not an ELF file\n", elf->executable);
    exit(-1);
  }

  if (elf->ehdr->e_shstrndx == 0 || elf->ehdr->e_shoff == 0 || elf->ehdr->e_shnum == 0) {
    printf("Section header table not found\n");	
    exit(-1);
  }

  // Search each section to find the symbol table header
  elf->symhdr = NULL;
  for (int i = 0; i < elf->ehdr->e_shnum; i++) {
    if (elf->shdr[i].sh_type == SHT_SYMTAB) {
      elf->symhdr = &elf->shdr[i];
      break;
    }
  }

  close(fd);
}

