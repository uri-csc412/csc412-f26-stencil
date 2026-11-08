from collections import namedtuple, defaultdict
import argparse
import sys
import os.path

parser = argparse.ArgumentParser(description='Grade lab')
parser.add_argument(
    '--grade_up_to',
    type=int,
    default=0,
    help='Will grade stages up to this one (included)',
)
parser.add_argument(
    '--grade_from', type=int, default=0, help='Will grade stages from this one'
)
parser.add_argument(
    '--output', type=str, default='', help='Will write the score to this file'
)
parser.add_argument(
    '--tmpfile', type=str, default='/tmp/log.txt', help='Will use this temporary file for execution log'
)
parser.add_argument(
    '--bad_alloc_tmpfile', type=str, default='', help='Log of the run that makes bad sys_page_alloc requests'
)
parser.add_argument(
    '--misaligned_tmpfile', type=str, default='', help='Log of the run that makes a misaligned sys_page_alloc request'
)
args = parser.parse_args()

PM_PREFIX = 'PM_DUMP'
VM_PREFIX = 'VM_DUMP'
PANIC_PREFIX = 'PANIC: Unexpected exception 52!'

# Per-page state for a page in physical and virtual memory.
PMEntry = namedtuple('PMEntry', ['owner', 'refcount'])
VMEntry = namedtuple('VMEntry', ['owner', 'refcount', 'ua', 'perm'])

# A bunch of constants.
PAGE_SIZE = 4096
KERNEL_LIMIT = 0x100000 // PAGE_SIZE
CGA_BLOCK = 0xB8000 // PAGE_SIZE
PTE_P = 0x1
PTE_W = 0x2
PTE_U = 0x4

# Offset into the start of kernel memory that should not be initially
# owned by processes.
PHYSICAL_UNALLOCATED_OFFSET = 17

# A process's page table tree has one page at each of the top three
# levels, plus one bottom-level page per 2MB region it uses. Its 3MB
# address space spans at most two such regions.
MIN_PAGE_TABLES = 4
MAX_PAGE_TABLES = 5

# The extra credit runs in p-allocator.c claim the marker page at
# BAD_REQUEST_MARKERS + i after request i is rejected.
BAD_REQUEST_MARKERS = 0x1F0000 // PAGE_SIZE
BAD_REQUESTS = [
    'a kernel address',
    'an address past the end of virtual memory',
    'an already-mapped address',
    'a misaligned address',
]
PAST_END_REQUEST = 1
ALREADY_MAPPED_REQUEST = 2
MISALIGNED_REQUEST = 3


class BColors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def print_result(passed, message, detail=''):
    if passed:
        print('  {}PASS{}  {}'.format(BColors.OKGREEN, BColors.ENDC, message))
    else:
        suffix = ' ({})'.format(detail) if detail else ''
        print('  {}FAIL{}  {}{}'.format(BColors.FAIL, BColors.ENDC, message, suffix))


class MemMapInstance:
    def __init__(self):
        self.pm_state = None
        self.vm_state = {}

    def set_pm_state(self, pm_state):
        assert self.pm_state == None
        self.pm_state = pm_state

    def add_vm_state(self, process_id, vm_state):
        assert not process_id in self.vm_state
        self.vm_state[process_id] = vm_state

    def IsKernelIsolated(self):
        # Returns true if all of kernel's addresses are not accessible
        # from any of the processes (except for the CGA block).
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i >= KERNEL_LIMIT:
                    break

                if i != CGA_BLOCK and entry.ua:
                    return False
        return True

    def AreProcessesIsolated(self):
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i < KERNEL_LIMIT:
                    # In kernel mem, checked by IsKernelIsolated
                    continue

                if entry.owner and entry.owner != process_id and entry.ua:
                    return False
        return True

    def IsVPAUsed(self):
        # Will check if virtual page allocation is used. Will simply
        # look at the owners of physical pages and make sure that at
        # least one physical page outside the address range of any
        # process is owned by a process.
        offset = PHYSICAL_UNALLOCATED_OFFSET
        for i, entry in enumerate(self.pm_state):
            if i < offset:
                continue

            if i >= KERNEL_LIMIT:
                return False
            if entry.owner and entry.owner <= 4:
                return True

        # Unreachable
        return False

    def AreProcessAddressesOverlapping(self):
        # Will check if there is at least one process that occupies
        # more than 64 pages of virtual memory.
        occupancy = defaultdict(int)
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i < KERNEL_LIMIT:
                    # In kernel mem.
                    continue

                if entry.owner and entry.owner == process_id and entry.ua:
                    occupancy[process_id] += 1
        return any(v > 64 for _, v in list(occupancy.items()))

    def CheckOwnershipInvariant(self):
        # Checks that all pages in the virtual address space of the
        # process are accessible by the process if they are owned by
        # it.
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i < KERNEL_LIMIT:
                    # In kernel mem.
                    continue

                if entry.owner and entry.owner == process_id and not entry.ua:
                    return False
        return True

    def CheckCGAAccessInvariant(self):
        # The CGA block should be accessible by all processes.
        for process_id, entries in list(self.vm_state.items()):
            if not entries[CGA_BLOCK].ua:
                return False
        return True
    
    def IsForkOK(self):
        # Checks to see if all processes' virtual address space starts
        # at the same place and, if the page is writable, that it's
        # owned by the given process. (Handles shared, read-only pages.)
        for process_id, entries in list(self.vm_state.items()):
            entry = entries[KERNEL_LIMIT]
            if not entry.owner or \
                   (entry.owner != process_id and entry.perm & PTE_W):
                return False
        return True

    def AreRefcountsConsistent(self):
        # A free page has no references and a page in use has at least
        # one. Before fork, a process-owned page has exactly one.
        for entry in self.pm_state:
            if (entry.owner == 0) != (entry.refcount == 0):
                return False
            if entry.owner in self.vm_state and entry.refcount != 1:
                return False
        return True

    def ArePermissionsCorrect(self):
        # Mapped pages below PROC_START_ADDR are kernel-only, except the
        # CGA block. Mapped pages from PROC_START_ADDR up are user pages.
        kernel_perm = PTE_P | PTE_W
        user_perm = PTE_P | PTE_W | PTE_U
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if not entry.perm & PTE_P:
                    continue
                is_user_page = i >= KERNEL_LIMIT or i == CGA_BLOCK
                if entry.perm != (user_perm if is_user_page else kernel_perm):
                    return False
        return True

    def AreKernelMappingsConsistent(self):
        # Every process should see the same permissions for addresses
        # below PROC_START_ADDR.
        views = [[entry.perm for entry in entries[:KERNEL_LIMIT]]
                 for entries in list(self.vm_state.values())]
        return all(view == views[0] for view in views)

    def AreOwnedPagesAccounted(self):
        # Every physical page a process owns should either be mapped
        # once with user access or be one of its page tables. Too few
        # leftover pages means a page table is user-accessible or a
        # page is mapped twice; too many means a leak. Kernel-only
        # identity mappings are not counted.
        for process_id, entries in list(self.vm_state.items()):
            owned = sum(1 for entry in self.pm_state
                        if entry.owner == process_id)
            mapped = sum(1 for entry in entries
                         if entry.owner == process_id and entry.ua)
            if not MIN_PAGE_TABLES <= owned - mapped <= MAX_PAGE_TABLES:
                return False
        return True

    def IsStackAtTop(self):
        # The last page of virtual memory holds the process's stack.
        for process_id, entries in list(self.vm_state.items()):
            top = entries[-1]
            if top.owner != process_id or top.perm != PTE_P | PTE_W | PTE_U:
                return False
        return True


class MemMapTimeline:
    def __init__(self, log_file):
        # Parsed data. For each tick will store an instance of
        # MemMapInstance.
        self.mem_data = defaultdict(MemMapInstance)
        self.panic = False

        with open(log_file) as f:
            lines = f.readlines()

        for line in lines:
            if line.startswith(PM_PREFIX):
                line_split = list(filter(bool, line.split(' ')))

                # The label, the tick 'timestamp' and the newline at
                # the end, 512 pages in physical memory.
                assert len(line_split) - 3 == 512 * 2
                tick = int(line_split[1])

                data = []
                for i in range(512):
                    owner, refcount = line_split[2 * i + 2 : 2 * i + 4]
                    data.append(PMEntry(int(owner), int(refcount)))
                self.mem_data[tick].set_pm_state(data)
            elif line.startswith(VM_PREFIX):
                line_split = list(filter(bool, line.split(' ')))

                # The label, the id, the tick 'timestamp' and the
                # newline at the end, 768 pages in virtual memory.
                assert len(line_split) - 4 == 768 * 3
                process_id = int(line_split[1])
                tick = int(line_split[2])

                data = []
                for i in range(768):
                    owner, refcount, perm = line_split[3 * i + 3 : 3 * i + 6]
                    data.append(VMEntry(int(owner), int(refcount),
                                        int(perm) & PTE_U,
                                        int(perm)))
                self.mem_data[tick].add_vm_state(process_id, data)
            elif line.startswith(PANIC_PREFIX):
                self.panic = True

    def PerformCheck(self, check_function, message, start_at_tick):
        res = 0
        tick = 0
        for tick, instance in sorted(self.mem_data.items()):
            if tick < start_at_tick:
                continue

            func = getattr(instance, check_function)
            if func():
                res = 1
            else:
                return (0, tick)

        return (res, tick)


    def PerformChecks(self, grade_from, up_to):
        """Runs the tests for each step - returns the total number of test cases passed"""
        all_functions = [
            ('IsKernelIsolated', 'Step 1A: Kernel isolation', 0),
            ('AreProcessesIsolated', 'Step 2A: Process isolation', 0),
            ('IsVPAUsed', 'Step 3: Virtual page allocation', 500),
            ('AreProcessAddressesOverlapping', 'Step 4A: Overlapping address spaces', 800),
            ('IsForkOK', 'Extra credit: Fork', 800),
        ]
        # Part B of a step, shown right after part A.
        part_b_checks = {
            'IsKernelIsolated': ('ArePermissionsCorrect',
                                 'Step 1B: Page permissions',
                                 'wrong PTE flags on a mapped page'),
            'AreProcessesIsolated': ('AreOwnedPagesAccounted',
                                     'Step 2B: Process page accounting',
                                     'leaked pages, or page tables visible to the process'),
            'AreProcessAddressesOverlapping': ('IsStackAtTop',
                                               'Step 4B: Stack at top of virtual memory',
                                               'the last virtual page is not the process stack'),
        }
        to_perform = all_functions[grade_from:up_to]
        passed = 0
        part_b_passed = 0
        for function_name, message, start_at_tick in to_perform:
            res, tick = self.PerformCheck(function_name, message, start_at_tick)
            print_result(res, message, 'tick {}'.format(tick))
            passed += res
            if function_name in part_b_checks:
                part_b_name, part_b_message, hint = part_b_checks[function_name]
                part_b_res, part_b_tick = self.PerformCheck(part_b_name, part_b_message, 0)
                print_result(part_b_res, part_b_message,
                             'tick {}: {}'.format(part_b_tick, hint))
                part_b_passed += part_b_res
        # make grade-five's "/5" total counts steps only, so leave out part B.
        if args.output != '':
            with open(args.output, 'w') as f:
                f.write(str(passed))
        return passed + part_b_passed

    def PerformAlwaysChecks(self):
        """Checks that should pass from the stencil onward. Returns the number passed."""
        always_checks = [
            ('AreRefcountsConsistent', 'Physical page refcounts',
             'a free page has references, or a process page has more than one'),
            ('AreKernelMappingsConsistent', 'Kernel mappings match across processes',
             'processes disagree on permissions below PROC_START_ADDR'),
        ]
        passed = 0
        for function_name, message, hint in always_checks:
            res, tick = self.PerformCheck(function_name, message, 0)
            print_result(res, message, 'tick {}: {}'.format(tick, hint))
            passed += res
        return passed

    def CheckKernelRuns(self, expected_max_tick):
        message = f"Kernel ran for {expected_max_tick} ticks"
        if not self.mem_data:
            print_result(False, message,
                         'no memory dumps logged: run make and check that the kernel compiles and boots')
            return 0
        max_tick = max(self.mem_data.keys())
        if not self.panic and expected_max_tick != max_tick:
            print_result(False, message, f"last recorded tick is {max_tick}")
            return 0
        print_result(True, message)
        return 1

    def CheckInvariants(self):
        message = "Invariants check"
        if not self.mem_data:
            print_result(False, message, 'no memory dumps logged')
            return 0
        for tick, instance in sorted(self.mem_data.items()):
            if not instance.CheckOwnershipInvariant():
                print_result(False, message,
                             f"tick {tick}: a process cannot access a page it owns")
                return 0

            if not instance.CheckCGAAccessInvariant():
                print_result(False, message,
                             f"tick {tick}: a process cannot access the CGA console")
                return 0
        print_result(True, message)
        return 1


def IsMarkerClaimed(instance, request):
    # True if every process claimed the marker page for this request.
    if not instance.vm_state:
        return False
    for process_id, entries in list(instance.vm_state.items()):
        marker = entries[BAD_REQUEST_MARKERS + request]
        if marker.owner != process_id or not marker.ua:
            return False
    return True


def IsRequestRejected(timeline, request):
    if not timeline.mem_data:
        return False
    instance = timeline.mem_data[max(timeline.mem_data)]
    if not IsMarkerClaimed(instance, request):
        return False
    # A request past the end of memory can fail only after taking a page.
    # An accepted already-mapped request leaks a page too, so only blame
    # this request for a leak when that one was rejected.
    if (request == PAST_END_REQUEST
            and IsMarkerClaimed(instance, ALREADY_MAPPED_REQUEST)
            and not instance.AreOwnedPagesAccounted()):
        return False
    return True


def CheckBadRequests(log_file, misaligned_log_file):
    """Reports the extra credit runs that make bad sys_page_alloc requests."""
    timeline = MemMapTimeline(log_file)
    misaligned_timeline = MemMapTimeline(misaligned_log_file)
    print()
    print('Optional extra credit (1 point each):')
    for request, description in enumerate(BAD_REQUESTS):
        run = misaligned_timeline if request == MISALIGNED_REQUEST else timeline
        print_result(IsRequestRejected(run, request),
                     'Extra credit: sys_page_alloc rejects {}'.format(description),
                     'wrong return value, kernel crash, or leaked page')


def grade():
    """Runs all the grading for"""
    if not os.path.isfile(args.tmpfile):
        print('No output from run, did code compile?')
        sys.exit(1)

    timeline = MemMapTimeline(args.tmpfile)

    # The fork run (make grade-five's second boot) only reports the fork test.
    if args.grade_from > 0:
        print()
        timeline.PerformChecks(args.grade_from, args.grade_up_to)
        return

    print('Should always pass (if one fails, something is broken):')
    kernel_runs = timeline.CheckKernelRuns(900)
    invariants_passed = timeline.CheckInvariants()
    always_passed = timeline.PerformAlwaysChecks()

    print()
    print('Implement these one step at a time:')
    passed = timeline.PerformChecks(args.grade_from, args.grade_up_to)

    total_score = kernel_runs + invariants_passed + always_passed + passed
    print()
    print(f"Total score {total_score}/11")

    if args.bad_alloc_tmpfile != '':
        CheckBadRequests(args.bad_alloc_tmpfile, args.misaligned_tmpfile)


if __name__ == "__main__":
    grade()
