#include "elf.h"

pid_t pid;  // Pid of the traced process


void usage(char* program_name) {
  printf("Usage: %s <exe>\n", program_name);
	exit(1);
}


void sighandler(int signum) {
  printf("Caught SIGINT: Detaching from %d\n", pid);

  if (ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0) {
    perror("PTRACE_DETACH");
    exit(-1);
  }
  exit(0);
}


long read_memory(long address) {
  long data = ptrace(PTRACE_PEEKDATA, pid, address, NULL);
  if (data < 0) {
    perror("PTRACE_PEEKDATA");
    exit(-1);
  }
  return data;
}


void write_memory(long address, long data) {
  if (ptrace(PTRACE_POKEDATA, pid, address, data) < 0) {
    perror("PTRACE_POKEDATA");
    exit(-1);
  }
}


void print_registers(elf64* program) {
  if (ptrace(PTRACE_GETREGS, pid, NULL, &program->registers) < 0) {
    perror("PTRACE_GETREGS");	
    exit(-1);
  }

  printf("%%rcx: %llx\n%%rdx: %llx\n%%rbx: %llx\n"
      "%%rax: %llx\n%%rdi: %llx\n%%rsi: %llx\n"
      "%%r8: %llx\n%%r9: %llx\n%%r10: %llx\n"
      "%%r11: %llx\n%%r12 %llx\n%%r13 %llx\n"
      "%%r14: %llx\n%%r15: %llx\n%%rsp: %llx\n", 
      program->registers.rcx, program->registers.rdx, program->registers.rbx, 
      program->registers.rax, program->registers.rdi, program->registers.rsi,
      program->registers.r8, program->registers.r9, program->registers.r10, 
      program->registers.r11, program->registers.r12, program->registers.r13, 
      program->registers.r14, program->registers.r15, program->registers.rsp);
}


void continue_execution() {
  if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0) {
    perror("PTRACE_CONT");
    exit(-1);
  }
}


void set_breakpoint(long address) {
  long data = read_memory(address);
  long trap = (data & ~0xff) | 0xcc;
  write_memory(address, trap);
}


int wait_for_signal() {
  int status = 0;
  int options = 0;
  waitpid(pid, &status, options);

  // Handle the status returned by `waitpid()`
  if (WIFEXITED(status)) {
    printf("Completed tracing pid: %d\nExit status: %d\n", pid, WEXITSTATUS(status));
    return 1;
  }
  else if (WIFSIGNALED(status)) {
    printf("Killed by signal %d\n", WTERMSIG(status));
    return 1;
  }
  else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
    printf("Hit breakpoint\n");
  }
  else if (WIFSTOPPED(status)) {
    printf("Stopped by signal %d\n", WSTOPSIG(status));

    if (WSTOPSIG(status) == SIGSEGV) {
      printf("Error: Segmentation Fault\n");
      exit(-1);
    }
    else if (WSTOPSIG(status) == SIGBUS) {
      printf("Error: Memory Fault\n");
      exit(-1);
    }
    else if (WSTOPSIG(status) == SIGFPE) {
      printf("Error: Floating point issue\n");
      exit(-1);
    }
    else if (WSTOPSIG(status) == SIGCHLD) {
      printf("Child exited\n");
      return 1;
    }
    else if (WSTOPSIG(status) == SIGABRT) {
      printf("SIGABRT recieved\n");
      return 1;
    }
  }
  else if (WIFCONTINUED(status)) {
    printf("Continuing...\n");
  }

  return 0;
}


int main(int argc, char** argv, char** envp) {
  signal(SIGINT, sighandler);

  if (argc < 2) {
    usage(argv[0]);
	}

  elf64 program;
  memset(&program, 0, sizeof(elf64));

  read_elf64(&program, argv[1]);
  print_symbols(&program);

  pid = fork();  // fork() the current process

  if (pid == 0) {
    // New process requests TRACEME and is replaced by execl()
    if (ptrace(PTRACE_TRACEME, pid, NULL, NULL) < 0) {
      perror("PTRACE_TRACEME");
      exit(-1);
    }

    char* args[2];
    args[0] = program.executable;
		args[1] = NULL;
    
    execve(program.executable, args, envp);
    exit(0);  // Unreachable
  }
  else if (0 < pid) {
    while(!wait_for_signal()) {
      print_registers(&program);

      printf("\nPlease hit any key to continue: ");
      getchar();

      continue_execution();
    }
  }
  else {
    perror("FORK");
    exit(-1);
  }

  return 0;
}

