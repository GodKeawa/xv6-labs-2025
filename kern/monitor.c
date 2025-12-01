// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "showva2pa", "Show the physical address mapped to a given virtual address or range", mon_showva2pa },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	return 0;
}

int
mon_showva2pa(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 1) {
		cprintf("Usage: showva2pa <virtual address> or showva2pa <start address> <end address>\n");
	} else if (argc == 2) {
		pte_t* pte_store;
		void* va = (void*)strtol(argv[1], NULL, 0);
		struct PageInfo* pp = page_lookup(kern_pgdir, va, &pte_store);
		if (pp) {
			cprintf("VA: 0x%08x, PA: 0x%08x, pp_ref: %u, PTE_W: %u, PTE_U: %u\n", 
				(uintptr_t) va, page2pa(pp), 
				pp->pp_ref,
				(((uint32_t)*(pte_store) & PTE_W) != 0) ? 1 : 0,
				(((uint32_t)*(pte_store) & PTE_U) != 0) ? 1 : 0
			);
		} else {
			cprintf("VA 0x%08x does not have a mapped physical page!\n", va);
		}
	} else if (argc == 3) {
		void* start_va = (void*)strtol(argv[1], NULL, 0);
		void* end_va = (void*)strtol(argv[2], NULL, 0);
		pte_t* pte_store;
		for (void* va = start_va; va <= end_va; va += PGSIZE) {
			struct PageInfo* pp = page_lookup(kern_pgdir, va, &pte_store);
			if (pp) {
				cprintf("VA: 0x%08x, PA: 0x%08x, pp_ref: %u, PTE_W: %u, PTE_U: %u\n", 
					(uintptr_t)va, page2pa(pp), 
					pp->pp_ref,
					(((uint32_t)*(pte_store) & PTE_W) != 0) ? 1 : 0,
					(((uint32_t)*(pte_store) & PTE_U) != 0) ? 1 : 0
				);
			} else {
				cprintf("VA 0x%08x does not have a mapped physical page!\n", (uintptr_t)va);
			}
		}
	} else {
		cprintf("Usage: showva2pa <virtual address> or showva2pa <start address> <end address>\n");
	}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
