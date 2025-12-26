#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PROCESS 10
#define MEMORY_SIZE 1024
#define PAGE_SIZE 16
#define FRAMES (MEMORY_SIZE / PAGE_SIZE)
#define MAX_FILES 20
#define MAX_RES 3
#define TIME_QUANTUM 2
#define STRESS_PROCESSES 8
#define MAX_BURST 10
#define MAX_MEM 256

typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } STATE;

// Page Table Entry
typedef struct {
    int frame;
    int valid;
} PageTableEntry;

// Process Control Block
typedef struct {
    int pid;
    STATE state;
    int burst;
    int remaining;
    int pages;
    PageTableEntry page_table[32];
} PCB;

// File System
typedef struct {
    char name[20];
    char data[128];
    int used;
} File;

// Global Variables
PCB pcb[MAX_PROCESS];
File fs[MAX_FILES];
int process_count = 0;
int current = -1;
int frame_used[FRAMES];
int available[MAX_RES] = {3, 3, 2};
int max_need[MAX_PROCESS][MAX_RES];
int allocation[MAX_PROCESS][MAX_RES];
int need[MAX_PROCESS][MAX_RES];

// Utility Functions
void print_state(STATE s) {
    char *states[] = {"NEW","READY","RUNNING","WAITING","TERMINATED"};
    printf("%s", states[s]);
}

// File Management
int find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++)
        if (fs[i].used && !strcmp(fs[i].name, name))
            return i;
    return -1;
}

void store_file(const char *name, const char *data) {
    int idx = find_file(name);
    if (idx == -1) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (!fs[i].used) {
                idx = i;
                break;
            }
        }
    }
    if (idx == -1) { printf("File system full!\n"); return; }
    strcpy(fs[idx].name, name);
    strcpy(fs[idx].data, data);
    fs[idx].used = 1;
    printf("File '%s' stored.\n", name);
}

void cat_file(const char *name) {
    int idx = find_file(name);
    if (idx == -1) { printf("File not found!\n"); return; }
    printf("%s\n", fs[idx].data);
}

// Paging
int allocate_frame() {
    for (int i = 0; i < FRAMES; i++)
        if (!frame_used[i]) { frame_used[i] = 1; return i; }
    return -1;
}

void free_frames(PCB *p) {
    for (int i = 0; i < p->pages; i++)
        if (p->page_table[i].valid) frame_used[p->page_table[i].frame] = 0;
}

void setup_paging(PCB *p, int mem_size) {
    p->pages = (mem_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < p->pages; i++) {
        int frame = allocate_frame();
        if (frame == -1) {
            printf("Page fault! Out of memory\n");
            p->page_table[i].valid = 0;
        } else {
            p->page_table[i].frame = frame;
            p->page_table[i].valid = 1;
        }
    }
}

void translate_address(PCB *p, int logical) {
    if (!p) { printf("Invalid PID!\n"); return; }
    if (logical < 0) { printf("Invalid logical address!\n"); return; }

    int page = logical / PAGE_SIZE;
    int offset = logical % PAGE_SIZE;

    if (page >= p->pages || !p->page_table[page].valid) {
        printf("PAGE FAULT at logical address %d\n", logical);
        return;
    }

    int physical = p->page_table[page].frame * PAGE_SIZE + offset;
    printf("Logical %d → Physical %d\n", logical, physical);
}

// Banker's Algorithm
int is_safe() {
    int work[MAX_RES];
    int finish[MAX_PROCESS] = {0};
    for (int i = 0; i < MAX_RES; i++) work[i] = available[i];

    int done_flag;
    do {
        done_flag = 0;
        for (int i = 0; i < process_count; i++) {
            if (!finish[i]) {
                int ok = 1;
                for (int r = 0; r < MAX_RES; r++)
                    if (need[i][r] > work[r]) { ok = 0; break; }
                if (ok) {
                    for (int r = 0; r < MAX_RES; r++) work[r] += allocation[i][r];
                    finish[i] = 1;
                    done_flag = 1;
                }
            }
        }
    } while (done_flag);

    for (int i = 0; i < process_count; i++)
        if (!finish[i]) return 0;
    return 1;
}

void request_resources(int pid) {
    if (pid < 0 || pid >= process_count) { printf("Invalid PID!\n"); return; }
    int req[MAX_RES];
    printf("Enter resource request (A B C): ");
    for (int i = 0; i < MAX_RES; i++) scanf("%d", &req[i]);

    for (int i = 0; i < MAX_RES; i++) {
        if (req[i] > need[pid][i]) { printf("Error: Exceeds maximum need\n"); return; }
        if (req[i] > available[i]) { printf("Resources not available\n"); return; }
    }

    for (int i = 0; i < MAX_RES; i++) {
        available[i] -= req[i];
        allocation[pid][i] += req[i];
        need[pid][i] -= req[i];
    }

    if (!is_safe()) {
        printf("Deadlock detected! Request denied\n");
        for (int i = 0; i < MAX_RES; i++) {
            available[i] += req[i];
            allocation[pid][i] -= req[i];
            need[pid][i] += req[i];
        }
    } else {
        printf("Request granted (Safe state)\n");
    }
}

// Process Management
void create_process() {
    if (process_count >= MAX_PROCESS) { printf("Process limit reached\n"); return; }

    int burst, mem;
    printf("Burst time: ");
    scanf("%d", &burst);
    printf("Memory size: ");
    scanf("%d", &mem);

    PCB *p = &pcb[process_count];
    p->pid = process_count;
    p->state = READY;
    p->burst = burst;
    p->remaining = burst;

    setup_paging(p, mem);

    for (int r = 0; r < MAX_RES; r++) {
        max_need[process_count][r] = rand() % 4;
        allocation[process_count][r] = 0;
        need[process_count][r] = max_need[process_count][r];
    }

    printf("Process %d created with %d pages\n", p->pid, p->pages);
    process_count++;
}

// Scheduler
void schedule() {
    for (int i = 0; i < process_count; i++) {
        current = (current + 1) % process_count;
        if (pcb[current].state == READY) {
            pcb[current].state = RUNNING;
            pcb[current].remaining -= TIME_QUANTUM;

            if (pcb[current].remaining <= 0) {
                pcb[current].state = TERMINATED;
                printf("Process %d finished\n", pcb[current].pid);

                // Free memory frames
                free_frames(&pcb[current]);

                // Release allocated resources
                for (int r = 0; r < MAX_RES; r++) {
                    available[r] += allocation[current][r];
                    allocation[current][r] = 0;
                    need[current][r] = 0;
                }

            } else {
                pcb[current].state = READY;
                printf("Running PID %d\n", pcb[current].pid);
            }
            return;
        }
    }
}

void ps() {
    printf("\nPID\tSTATE\tPAGES\n");
    for (int i = 0; i < process_count; i++) {
        printf("%d\t", pcb[i].pid);
        print_state(pcb[i].state);
        printf("\t%d\n", pcb[i].pages);
    }
}

void dump_mem() {
    printf("\nMemory Map (Frames %d):\n", FRAMES);
    for (int i = 0; i < FRAMES; i++) {
        printf("Frame %2d: ", i);
        if (!frame_used[i]) printf("Free\n");
        else {
            // find which PID owns this frame
            int pid = -1;
            for (int p = 0; p < process_count; p++)
                for (int pg = 0; pg < pcb[p].pages; pg++)
                    if (pcb[p].page_table[pg].valid && pcb[p].page_table[pg].frame == i)
                        pid = pcb[p].pid;
            printf("Allocated to PID %d\n", pid);
        }
    }
}

// Shell
void shell() {
    char cmd[30];
    while (1) {
        printf("\nMiniOS> ");
        scanf("%s", cmd);

        if (!strcmp(cmd, "create")) create_process();
        else if (!strcmp(cmd, "run")) {
            int done;
            do {
                done = 1;
                for (int i = 0; i < process_count; i++)
                    if (pcb[i].state != TERMINATED) done = 0;
                if (!done) schedule();
            } while (!done);
        }
        else if (!strcmp(cmd, "ps")) ps();
        else if (!strcmp(cmd, "addr")) {
            int pid, addr;
            scanf("%d %d", &pid, &addr);
            if (pid < 0 || pid >= process_count) { printf("Invalid PID!\n"); continue; }
            translate_address(&pcb[pid], addr);
        }
        else if (!strcmp(cmd, "req")) {
            int pid;
            scanf("%d", &pid);
            request_resources(pid);
        }
        else if (!strcmp(cmd, "store")) {
            char name[20], data[128];
            scanf("%s %s", name, data);
            store_file(name, data);
        }
        else if (!strcmp(cmd, "cat")) {
            char name[20];
            scanf("%s", name);
            cat_file(name);
        }
        else if (!strcmp(cmd, "dump_mem")) dump_mem();
        else if (!strcmp(cmd, "exit")) break;
        else printf("Unknown command\n");
    }
}


void stress_test() {
    srand(time(NULL));
    printf("=== MINI OS STRESS TEST START ===\n");

    //Create multiple processes
    printf("\n[Test] Creating %d processes...\n", STRESS_PROCESSES);
    for (int i = 0; i < STRESS_PROCESSES; i++) {
        PCB *p = &pcb[process_count];
        p->pid = process_count;
        p->state = READY;
        p->burst = rand() % MAX_BURST + 1;
        p->remaining = p->burst;

        int mem = rand() % MAX_MEM + 16;
        setup_paging(p, mem);

        // Random resource needs
        for (int r = 0; r < MAX_RES; r++) {
            max_need[process_count][r] = rand() % 4;
            allocation[process_count][r] = 0;
            need[process_count][r] = max_need[process_count][r];
        }

        printf("Process %d: Burst=%d, Pages=%d, MaxRes=[%d %d %d]\n",
               p->pid, p->burst, p->pages,
               max_need[process_count][0],
               max_need[process_count][1],
               max_need[process_count][2]);

        process_count++;
    }

    // File system stress
    printf("\n[Test] File system stress...\n");
    for (int i = 0; i < 5; i++) {
        char fname[20], data[50];
        sprintf(fname, "file%d.txt", i);
        sprintf(data, "Data_of_file_%d", i);
        store_file(fname, data);
    }

    // Random resource requests
    printf("\n[Test] Random resource requests...\n");
    for (int i = 0; i < STRESS_PROCESSES; i++) {
        int pid = i;
        int req[MAX_RES];
        for (int r = 0; r < MAX_RES; r++)
            req[r] = rand() % (need[pid][r]+1);

        printf("PID %d requesting [%d %d %d]... ", pid, req[0], req[1], req[2]);

        for (int r = 0; r < MAX_RES; r++) {
            if (req[r] > need[pid][r]) req[r] = need[pid][r];
            if (req[r] > available[r]) req[r] = available[r];
            available[r] -= req[r];
            allocation[pid][r] += req[r];
            need[pid][r] -= req[r];
        }

        if (is_safe()) printf("Granted (Safe)\n");
        else {
            printf("Denied (Unsafe)\n");
            for (int r = 0; r < MAX_RES; r++) { // rollback
                available[r] += req[r];
                allocation[pid][r] -= req[r];
                need[pid][r] += req[r];
            }
        }
    }

    // Logical address translation with random faults
    printf("\n[Test] Logical address translation...\n");
    for (int i = 0; i < STRESS_PROCESSES; i++) {
        int logical = rand() % (pcb[i].pages*PAGE_SIZE + 20); // +20 for intentional faults
        printf("PID %d, Logical=%d -> ", i, logical);
        translate_address(&pcb[i], logical);
    }

    // Memory dump before scheduling
    printf("\n[Test] Memory dump before scheduler...\n");
    dump_mem();

    // Run scheduler (Round-Robin)
    printf("\n[Test] Running scheduler...\n");
    int done;
    do {
        done = 1;
        for (int i = 0; i < process_count; i++)
            if (pcb[i].state != TERMINATED) done = 0;
        if (!done) schedule();
    } while (!done);

    // Final process table & memory
    printf("\n[Test] Final process table:\n");
    ps();

    printf("\n[Test] Final memory dump:\n");
    dump_mem();

    printf("\n=== MINI OS STRESS TEST END ===\n");
}

int main() {
    //printf("Booting MiniOS Stress Test...\n");
    //stress_test();

    printf("Booting MiniOS...\n");
    printf("Interactive shell started.\n");
    printf("Commands available: create, run, ps, addr, req, store, cat, dump_mem, exit\n");
    // Start interactive shell
    shell();

    printf("MiniOS shutdown.\n");
    return 0;

}
