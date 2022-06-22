// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	// Took the help from bhavya singh


	if (!(
			(err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) && 
			(uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW)))
		panic("not copy-on-write");

	addr = ROUNDDOWN(addr, PGSIZE);
	if (sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P) < 0)
		panic("we are in the sys_page_alloc");
	memcpy(PFTEMP, addr, PGSIZE);
	if (sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P) < 0)
		panic("we are currently in the sys_page_map");
	if (sys_page_unmap(0, PFTEMP) < 0)
		panic("we are currently in the sys_page_unmap");
	return;

	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t environmentid, unsigned pn)
{

	int r;
	// LAB 4: Your code here.
	// Took the help from bhavya singh
	void *address = (void*) (pn*PGSIZE);
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		if (sys_page_map(0, address, environmentid, address, PTE_COW | PTE_U | PTE_P) < 0)
		{
			panic("2 we are in the duppage");
		}
		if (sys_page_map(0, address, 0, address, PTE_COW | PTE_U | PTE_P) < 0)
		{
			panic("3 we are in the duppage");
		}
	}
	else {
		sys_page_map(0, address, environmentid, address, PTE_U | PTE_P);
	}
	return 0;
	panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.

	// Took the help from bhavya singh
	set_pgfault_handler(pgfault);

	envid_t environmentid;
	uint32_t i;
	environmentid = sys_exofork();
	if (environmentid == 0) {// check for the environment condeation?
		// panic("child");
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// cprintf("sys_exofork: %x\n", envid);
	if (environmentid < 0)// check for the envuironment id is in negative
		panic("sys_exofork: %e", environmentid);//chek the size of the environment

	for (i = 0; i < USTACKTOP; i += PGSIZE)
	{
		if ((uvpd[PDX(i)] & PTE_P) && (uvpt[PGNUM(i)] & PTE_P)//check for the PDX and PTE_P and PTE_p are correct
			&& (uvpt[PGNUM(i)] & PTE_U)) {//check for the PDX and PTE_P and PTE_p are correct
			duppage(environmentid, PGNUM(i));//dump the page element
		}
	}
	if (sys_page_alloc(environmentid, (void*)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P) < 0) {// check for page is allocated?
		panic("1 we are in the fork");
	}
	extern void _pgfault_upcall();
	sys_env_set_pgfault_upcall(environmentid, _pgfault_upcall);

	if (sys_env_set_status(environmentid, ENV_RUNNABLE) < 0)// Check for the page stateus is valid or not
	{
		panic("sys_env_set_status");
	}

	return environmentid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
