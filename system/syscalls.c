/*
 * syscalls.c - System call stubs for CH32V103 microcontroller
 * 
 * This file provides minimal stub implementations of POSIX system calls
 * required by the C standard library (newlib) for embedded systems.
 * These stubs prevent linker errors and provide basic functionality.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/*
 * _close - Close a file descriptor
 * For embedded systems, we don't have a file system, so this always fails
 */
int _close(int file) {
    (void)file;  // Suppress unused parameter warning
    errno = EBADF;
    return -1;
}

/*
 * _fstat - Get file status
 * For embedded systems, we treat all file descriptors as character devices
 */
int _fstat(int file, struct stat *st) {
    (void)file;  // Suppress unused parameter warning
    
    if (st == NULL) {
        errno = EFAULT;
        return -1;
    }
    
    // Set up stat structure for character device
    st->st_mode = S_IFCHR;  // Character device
    st->st_size = 0;
    st->st_blksize = 0;
    st->st_blocks = 0;
    
    return 0;
}

/*
 * _isatty - Check if file descriptor is a terminal
 * For embedded systems, we consider stdin/stdout/stderr as terminals
 */
int _isatty(int file) {
    // Consider stdin (0), stdout (1), stderr (2) as terminals
    if (file >= 0 && file <= 2) {
        return 1;  // Is a terminal
    }
    
    errno = ENOTTY;
    return 0;  // Not a terminal
}

/*
 * _lseek - Seek to position in file
 * For embedded systems without file system, this always fails
 */
off_t _lseek(int file, off_t offset, int whence) {
    (void)file;    // Suppress unused parameter warning
    (void)offset;  // Suppress unused parameter warning
    (void)whence;  // Suppress unused parameter warning
    
    errno = ESPIPE;  // Illegal seek
    return -1;
}

/*
 * _read - Read from file descriptor
 * For embedded systems, this could be implemented to read from UART
 * For now, it's a stub that always fails
 */
ssize_t _read(int file, void *ptr, size_t len) {
    (void)file;  // Suppress unused parameter warning
    (void)ptr;   // Suppress unused parameter warning
    (void)len;   // Suppress unused parameter warning
    
    errno = EBADF;
    return -1;
}

/*
 * _write - Write to file descriptor
 * This could be implemented to write to UART for debugging
 * For now, it's a stub that pretends to succeed
 */
// ssize_t _write(int file, const void *ptr, size_t len) {
//     (void)file;  // Suppress unused parameter warning
//     (void)ptr;   // Suppress unused parameter warning
    
//     // For stdout/stderr, pretend we wrote successfully
//     if (file == 1 || file == 2) {
//         return len;
//     }
    
//     errno = EBADF;
//     return -1;
// }

/*
 * _sbrk - Increase program data space
 * Used by malloc() for dynamic memory allocation
 */
// extern char _end;  // Defined by linker script
// static char *heap_end = NULL;

// void *_sbrk(ptrdiff_t incr) {
//     char *prev_heap_end;
    
//     if (heap_end == NULL) {
//         heap_end = &_end;
//     }
    
//     prev_heap_end = heap_end;
//     heap_end += incr;
    
//     return (void *)prev_heap_end;
// }

/*
 * _getpid - Get process ID
 * For embedded systems, always return 1
 */
pid_t _getpid(void) {
    return 1;
}

/*
 * _kill - Send signal to process
 * For embedded systems, this is not applicable
 */
int _kill(pid_t pid, int sig) {
    (void)pid;  // Suppress unused parameter warning
    (void)sig;  // Suppress unused parameter warning
    
    errno = EINVAL;
    return -1;
}

/*
 * _exit - Terminate program
 * For embedded systems, enter infinite loop
 */
void _exit(int status) {
    (void)status;  // Suppress unused parameter warning
    
    // Disable interrupts and enter infinite loop
    __asm volatile ("csrci mstatus, 8");  // Disable global interrupts
    while (1) {
        __asm volatile ("wfi");  // Wait for interrupt (low power)
    }
}