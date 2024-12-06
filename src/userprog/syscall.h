#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "threads/thread.h"

enum load_status
{
  NOT_LOADED,
  LOADED,
  LOAD_FAILED
};

void syscall_init (void);

struct lock filesys_lock; // Lock for file system
int get_page_pointer (const void *vaddr); // Get the page pointer
struct child_process* get_child_process_by_pid (int pid); // Find a specific child process
void delete_child_process (struct child_process *child); // Remove a child process
void cleanup_child_processes (void); // Remove all child processes of the current thread
struct file* get_file_by_desc(int fd); // Get the file pointer from the file descriptor
void terminate_file_access (int fd); // Close the file

struct child_process {
  int pid; // Process ID
  enum load_status load_status; // Load status
  int is_waiting; // Wait status
  int is_exit; // Exit status
  int status; // Status
  struct semaphore load_sema; // Load semaphore
  struct semaphore exit_sema; // Exit semaphore
  struct list_elem elem; // List element
};

struct thread_file_context {
  struct file *file; // File pointer
  int fd; // File descriptor
  struct list_elem elem; // List element
};

#endif /**< userprog/syscall.h */
