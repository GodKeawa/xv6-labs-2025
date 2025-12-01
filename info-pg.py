import gdb

# x86 分页标志位
PTE_P = 0x001  # Present
PTE_W = 0x002  # Writeable
PTE_U = 0x004  # User
PTE_PWT = 0x008  # Write-Through
PTE_PCD = 0x010  # Cache-Disable
PTE_A = 0x020  # Accessed
PTE_D = 0x040  # Dirty
PTE_PS = 0x080  # Page Size (4MB)
PTE_G = 0x100  # Global

# JOS 内核通常将物理内存映射到 0xF0000000
# 我们需要利用这个偏移量来读取页表内容
KERNBASE = 0xF0000000


def get_cr3():
    try:
        cr3 = gdb.parse_and_eval("$cr3")
        return int(cr3) & ~0xFFF
    except gdb.error:
        print("Error: Could not read CR3.")
        return None


def phys_to_virt(phys_addr):
    return phys_addr + KERNBASE


def read_mem_u32(addr):
    inferior = gdb.selected_inferior()
    try:
        mem = inferior.read_memory(addr, 4)
        return int.from_bytes(mem, byteorder="little")
    except gdb.MemoryError:
        return None


class PageRange:
    """帮助类：用于缓存并合并连续的页表项"""

    def __init__(self):
        self.reset()

    def reset(self):
        self.start_va = 0
        self.end_va = 0
        self.start_pa = 0
        self.perms = 0
        self.is_active = False

    def flush(self):
        """打印当前缓存的块"""
        if not self.is_active:
            return

        size = self.end_va - self.start_va

        # 格式化大小显示 (KB 或 MB)
        if size >= 1024 * 1024:
            size_str = f"{size // (1024 * 1024)}MB"
        else:
            size_str = f"{size // 1024}KB"

        perm_str = self.format_perms(self.perms)

        print(
            f"VA: [{hex(self.start_va)}-{hex(self.end_va)}) -> "
            f"PA: [{hex(self.start_pa)}-{hex(self.start_pa + size)}) "
            f"Size: {size_str}  Perms: {perm_str}"
        )

        self.reset()

    def add(self, va, pa, perms, page_size):
        """尝试添加一个新的页，如果无法合并则先 flush"""

        # 判定是否可以合并：
        # 1. 当前必须有正在缓存的块
        # 2. 虚拟地址必须紧接上一个结束位置
        # 3. 物理地址必须紧接上一个结束位置 (物理内存也必须连续)
        # 4. 权限必须完全一致
        can_merge = (
            self.is_active
            and va == self.end_va
            and pa == (self.start_pa + (self.end_va - self.start_va))
            and perms == self.perms
        )

        if can_merge:
            self.end_va += page_size
        else:
            self.flush()  # 打印旧的
            # 开始新的
            self.start_va = va
            self.end_va = va + page_size
            self.start_pa = pa
            self.perms = perms
            self.is_active = True

    def format_perms(self, entry):
        s = ""
        s += "P" if (entry & PTE_P) else "-"
        s += "W" if (entry & PTE_W) else "-"
        s += "U" if (entry & PTE_U) else "-"
        s += "T" if (entry & PTE_PWT) else "-"
        s += "C" if (entry & PTE_PCD) else "-"
        s += "A" if (entry & PTE_A) else "-"
        s += "D" if (entry & PTE_D) else "-"
        s += "G" if (entry & PTE_G) else "-"
        if entry & PTE_PS:
            s += " [Huge]"
        return s


class InfoPgCommand(gdb.Command):
    """
    Merged 'info pg' for JOS.
    Merges consecutive pages with same permissions and physical continuity.
    """

    def __init__(self):
        super(InfoPgCommand, self).__init__("info-pg", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        cr3 = get_cr3()
        if cr3 is None:
            return

        print(f"Scanning Page Table (CR3={hex(cr3)})...")

        pd_phys = cr3
        pagerange = PageRange()

        # 遍历 Page Directory
        for pdx in range(1024):
            pde_addr = phys_to_virt(pd_phys + (pdx * 4))
            pde = read_mem_u32(pde_addr)

            if not pde or not (pde & PTE_P):
                continue

            pde_virt_base = pdx << 22  # PDX index * 4MB

            # 处理 4MB 大页 (Huge Page)
            if pde & PTE_PS:
                pa = pde & ~0xFFF  # Mask out flag bits
                # 4MB 大页直接添加
                pagerange.add(pde_virt_base, pa, pde & 0xFFF, 4 * 1024 * 1024)
                continue

            # 处理普通页表 (指向 PTE 表)
            pt_phys = pde & ~0xFFF

            # 遍历 Page Table
            for ptx in range(1024):
                pte_addr = phys_to_virt(pt_phys + (ptx * 4))
                pte = read_mem_u32(pte_addr)

                if not pte or not (pte & PTE_P):
                    # 如果遇到空洞，一定要 flush，因为连续性中断了
                    pagerange.flush()
                    continue

                va = pde_virt_base + (ptx << 12)  # + PTX * 4KB
                pa = pte & ~0xFFF
                perms = pte & 0xFFF

                pagerange.add(va, pa, perms, 4 * 1024)

        # 循环结束后，别忘了打印最后一个缓存块
        pagerange.flush()
        print("Scan complete.")


InfoPgCommand()
print("Custom command 'info-pg' loaded.")
