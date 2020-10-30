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
    if (!((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) &&(uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW))){
        panic("not copy-on-write");
    }
    // Allocate a new page, map it at a temporary location (PFTEMP),
    // copy the data from the old page to the new page, then move the new
    // page to the old page's address.
    // Hint:
    //   You should make three system calls.
    
    // LAB 4: Your code here.
    addr = ROUNDDOWN(addr, PGSIZE);
    if (sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P) < 0){
        panic("sys_page_alloc");
    }
    memcpy(PFTEMP, addr, PGSIZE);
    if (sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P) < 0){
        panic("sys_page_map");
    }
    if (sys_page_unmap(0, PFTEMP) < 0){
        panic("sys_page_unmap");
    }
    return;
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
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
    void *addr = (void *)(pn * PGSIZE);
    if (uvpt[pn] & (PTE_W|PTE_COW)) {
        if ((r = sys_page_map(0, addr, envid, addr, PTE_COW|PTE_U|PTE_P)) < 0){
            panic("sys_page_map COW:%e", r);
        }
        if ((r = sys_page_map(0, addr, 0, addr, PTE_COW|PTE_U|PTE_P)) < 0){
            panic("sys_page_map COW:%e", r);
        }
    }
    else{
        if ((r = sys_page_map(0, addr, envid, addr, PTE_U|PTE_P)) < 0){
            panic("sys_page_map UP:%e", r);
        }
    }
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
    extern void _pgfault_upcall(void);
    envid_t myenvid = sys_getenvid();
    envid_t envid;
    uint32_t i, j, pn;
    //set page fault handler
    set_pgfault_handler(pgfault);
    //create a child
    if((envid = sys_exofork()) < 0){
        return -1;
    }
    if(envid == 0){
        thisenv = &envs[ENVX(sys_getenvid())];
        return envid;
    }
    //copy address space to child
    i = PDX(UTEXT);
    while(i < PDX(UXSTACKTOP)){
        if(uvpd[i] & PTE_P){
            j = 0;
            while(j < NPTENTRIES){
                pn = PGNUM(PGADDR(i, j, 0));
                if(pn == PGNUM(UXSTACKTOP - PGSIZE)){
                    break;
                }
                if(uvpt[pn] & PTE_P){
                    duppage(envid, pn);
                }
                j++;
            }
        }
        i++;
    }
    if((sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_P | PTE_W) < 0) || (sys_page_map(envid, (void *)(UXSTACKTOP - PGSIZE), myenvid, PFTEMP, PTE_U | PTE_P | PTE_W) < 0)){
        return -1;
    }
    memmove((void *)(UXSTACKTOP - PGSIZE), PFTEMP, PGSIZE);
    if((sys_page_unmap(myenvid, PFTEMP) < 0) || (sys_env_set_pgfault_upcall(envid, _pgfault_upcall) < 0) || (sys_env_set_status(envid, ENV_RUNNABLE) < 0))
    {
        return -1;
    }
    return envid;
    panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
