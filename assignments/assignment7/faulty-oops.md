
## Analysis - Executing "echo \"hello_world\" > /dev/faulty" in runqemu script

```sh
# echo "hello_world" > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b64000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: faulty(O) scull(O) hello(O)
CPU: 0 PID: 169 Comm: sh Tainted: G           O       6.1.44 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc8/0x390
sp : ffffffc008dfbd20
x29: ffffffc008dfbd80 x28: ffffff8001b99a80 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 000000000000000c x22: 000000000000000c x21: ffffffc008dfbdc0
x20: 0000005556c45990 x19: ffffff8001ba1400 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc00078c000 x3 : ffffffc008dfbdc0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f)
---[ end trace 0000000000000000 ]---
```



## 1️⃣ Call Trace Confirmation
The Program Counter (PC) was at an offset of **+0x10** in `faulty_write`:

```
pc : faulty_write+0x10/0x20 [faulty]
```
This means the crash happened at instruction **0x10** in the `faulty_write` function.

## 2️⃣ Disassembly of `faulty_write`

```
0000000000000000 <faulty_write>:
   0:   d2800001        mov     x1, #0x0          // x1 = 0
   4:   d2800000        mov     x0, #0x0          // x0 = 0
   8:   d503233f        paciasp                   // Pointer Authentication Code (PAC)
   c:   d50323bf        autiasp                   // Authenticate PAC
  10:   b900003f        str     wzr, [x1]         // Store 0 (wzr) at address in x1 (which is 0)
  14:   d65f03c0        ret                       // Return
  18:   d503201f        nop
  1c:   d503201f        nop
```

At instruction **0x10**:

```
b900003f        str     wzr, [x1]
```
- `wzr` is the **zero register** (it holds the constant value `0`).
- The instruction is trying to **store `0` into the memory address pointed by `x1`**.
- But **`x1 = 0`**, meaning **it attempts to write to address `0x0`**, which is an **invalid memory location**.
- This triggers a **segmentation fault** and the **kernel oops**.

## Summary
- The crash happened due to a **deliberate NULL pointer dereference** in `faulty_write`.
- The **offending instruction** (`str wzr, [x1]` at `0x10`) attempted to **write to memory address `0x0`**.
- The kernel **triggered a segmentation fault**, resulting in a **kernel oops**.

This confirms that the **faulty module is working as expected**.



