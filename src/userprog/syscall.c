#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

bool FILE_LOCK_IS_ACTIVE = false;

static void syscall_handler (struct intr_frame *);

void halt (void); // Halt the operating system
void exit (int status); // Terminate this process
pid_t exec (const char* cmd_line); // Start another process
int wait (pid_t pid); // Wait for a child process to die
bool create (const char* file, unsigned initial_size); // Create a file
bool remove (const char* file); // Delete a file
int open (const char * file); // Open a file
void close (int fd); // Close a file
int filesize (int fd); // Obtain a file's size (in bytes)
int read (int fd, void *buffer, unsigned size); // Read from a file
int write (int fd, const void * buffer, unsigned size); // Write to a file

void parse_args (struct intr_frame *f, int *arg, int num_of_args);
void is_ptr_valid (const void* vaddr);
void is_str_valid (const void* str);
void is_buffer_valid (const void* buf, unsigned byte_size);
int register_file (struct file *file_name);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  // printf ("system call!\n");
  // thread_exit ();
  if (!FILE_LOCK_IS_ACTIVE)
  {
    lock_init(&filesys_lock);
    FILE_LOCK_IS_ACTIVE = true;
  }
  
  int arg[6]; // Architecture limitation
  int esp = get_page_pointer((const void *) f->esp);
  
  switch (* (int *) esp)
  {
    case SYS_HALT:
      halt();
      break;
      
    case SYS_EXIT:
      parse_args(f, &arg[0], 1);
      exit(arg[0]);
      break;
      
    case SYS_EXEC:
      parse_args(f, &arg[0], 1);
      
      is_str_valid((const void*)arg[0]);
      
      arg[0] = get_page_pointer((const void *)arg[0]);
      f->eax = exec((const char*)arg[0]); // Execute the command line
      break;
      
    case SYS_WAIT:
      parse_args(f, &arg[0], 1);
      f->eax = wait(arg[0]);
      break;
      
    case SYS_CREATE:
      parse_args(f, &arg[0], 2);
      
      is_str_valid((const void *)arg[0]);
      
      arg[0] = get_page_pointer((const void *) arg[0]);
      
      f->eax = create((const char *)arg[0], (unsigned)arg[1]);
      break;

    case SYS_REMOVE:
      parse_args(f, &arg[0], 1);
      
      is_str_valid((const void*)arg[0]);
      
      arg[0] = get_page_pointer((const void *) arg[0]);
      
      f->eax = remove((const char *)arg[0]);
      break;

    case SYS_OPEN:
      parse_args(f, &arg[0], 1);
      
      is_str_valid((const void*)arg[0]);
     
      arg[0] = get_page_pointer((const void *)arg[0]);
      
      f->eax = open((const char *)arg[0]);  // open this file
      break;
    
    case SYS_CLOSE:
      parse_args (f, &arg[0], 1);

      close(arg[0]);
      break;
    
    case SYS_FILESIZE:
      parse_args(f, &arg[0], 1);
      
      f->eax = filesize(arg[0]);
      break;
      
    case SYS_READ:
      parse_args(f, &arg[0], 3);
      
      is_buffer_valid((const void*)arg[1], (unsigned)arg[2]);
       
      arg[1] = get_page_pointer((const void *)arg[1]); 
      
      f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
      break;

    case SYS_WRITE:
      parse_args(f, &arg[0], 3);
      
      is_buffer_valid((const void*)arg[1], (unsigned)arg[2]);
       
      arg[1] = get_page_pointer((const void *)arg[1]); 
      
      f->eax = write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
      break;
      
    default:
      break;
  }
}

/* syscall_halt */
void
halt (void)
{
  shutdown_power_off(); // From shutdown.h
}

/* syscall_exit */
void
exit (int status)
{
  struct thread *cur = thread_current();
  if (thread_exists(cur->parent) && cur->cp)
  {
    if (status < 0)
    {
      status = -1;
    }
    cur->cp->status = status;
  }
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

/* syscall_exec */
pid_t
exec (const char* cmd_line)
{
    pid_t pid = process_execute(cmd_line);
    struct child_process *child_process_ptr = get_child_process_by_pid(pid);
    if (!child_process_ptr)
    {
      return -1;
    }
    // Wait for the child process to load
    if (child_process_ptr->load_status == NOT_LOADED)
    {
      sema_down(&child_process_ptr->load_sema);
    }
    // If the child process failed to load, remove it
    if (child_process_ptr->load_status == LOAD_FAILED)
    {
      delete_child_process(child_process_ptr);
      return -1;
    }
    return pid;
}

/* syscall_wait */
int
wait (pid_t pid)
{
  return process_wait(pid);
}

/* syscall_create */
bool
create (const char* file, unsigned initial_size)
{
  lock_acquire(&filesys_lock);
  bool success = filesys_create(file, initial_size); // From filesys.h
  lock_release(&filesys_lock);
  return success;
}

/* syscall_remove */
bool
remove (const char* file)
{
  lock_acquire(&filesys_lock);
  bool success = filesys_remove(file); // From filesys.h
  lock_release(&filesys_lock);
  return success;
}

/* syscall_open */
int
open (const char *file)
{
  lock_acquire(&filesys_lock);
  struct file *file_pointer = filesys_open(file); // From filesys.h
  if (!file_pointer)
  {
    lock_release(&filesys_lock);
    return -1;
  }
  int filedes = register_file(file_pointer);
  lock_release(&filesys_lock);
  return filedes;
}

/* syscall_filesize */
int
filesize (int fd)
{
  lock_acquire(&filesys_lock);
  struct file *file_pointer = get_file_by_desc(fd);
  if (!file_pointer)
  {
    lock_release(&filesys_lock);
    return -1;
  }
  int filesize = file_length(file_pointer); // From file.h
  lock_release(&filesys_lock);
  return filesize;
}

/* syscall_read */
int
read (int fd, void *buffer, unsigned size)
{
  if (size <= 0)
  {
    return size;
  }
  
  if (fd == 0) // STD_INPUT
  {
    uint8_t *local_buf = (uint8_t *) buffer;
    for (unsigned i = 0; i < size; i++)
    {
      // Read key from input buffer
      local_buf[i] = input_getc(); // from input.h
    }
    return size;
  }
  
  // Start reading from file
  lock_acquire(&filesys_lock);
  struct file *file_pointer = get_file_by_desc(fd);
  if (!file_pointer)
  {
    lock_release(&filesys_lock);
    return -1;
  }

  int bytes = file_read(file_pointer, buffer, size); // From file.h
  lock_release (&filesys_lock);
  return bytes;
}

/* syscall_write */
int 
write (int fd, const void * buffer, unsigned size)
{
    if (size <= 0)
    {
      return size;
    }
    if (fd == 1) // STD_OUTPUT
    {
      putbuf (buffer, size); // From stdio.h
      return size;
    }
    
    // Start writing to file
    lock_acquire(&filesys_lock);
    struct file *file_pointer = get_file_by_desc(fd);
    if (!file_pointer)
    {
      lock_release(&filesys_lock);
      return -1;
    }

    int bytes = file_write(file_pointer, buffer, size); // file.h
    lock_release (&filesys_lock);
    return bytes;
}

/* syscall_close */
void
close(int fd)
{
  lock_acquire(&filesys_lock);
  terminate_file_access(fd);
  lock_release(&filesys_lock);
}

/* Get arguments from stack */
void
parse_args (struct intr_frame *f, int *args, int num_of_args)
{
  int i;
  int *ptr;
  for (i = 0; i < num_of_args; i++)
  {
    ptr = (int *) f->esp + i + 1;
    is_ptr_valid((const void *) ptr);
    args[i] = *ptr;
  }
}

void
is_ptr_valid (const void *vaddr)
{
    // 0x08048000 is the least possible user virtual address
    if (vaddr < ((void *) 0x08048000) || !is_user_vaddr(vaddr))
    {
      // Out of bound pointer
      exit(-1);
    }
}

void
is_str_valid (const void* str)
{
    for (; * (char *) get_page_pointer(str) != 0; str = (char *) str + 1);
}

void
is_buffer_valid(const void* buf, unsigned byte_size)
{
  unsigned i = 0;
  char* this_buff = (char *)buf;
  for (; i < byte_size; i++)
  {
    is_ptr_valid((const void*)this_buff);
    this_buff++;
  }
}

/* Get the physical address of a virtual address */
int
get_page_pointer(const void *vaddr)
{
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
  {
    exit(-1);
  }
  return (int)ptr;
}

/* Add file to the file list and return descriptor */
int
register_file (struct file *file_name)
{
  struct thread_file_context *process_file_ptr = malloc(sizeof(struct thread_file_context));
  if (!process_file_ptr)
  {
    return -1;
  }
  process_file_ptr->file = file_name;
  process_file_ptr->fd = thread_current()->fd;
  thread_current()->fd++;
  list_push_back(&thread_current()->file_handle_list, &process_file_ptr->elem);
  return process_file_ptr->fd;
}

/* Get the file pointer from the file descriptor */
struct file*
get_file_by_desc (int fd)
{
  struct thread *t = thread_current();
  struct list_elem* next;
  struct list_elem* e = list_begin(&t->file_handle_list);
  
  for (; e != list_end(&t->file_handle_list); e = next)
  {
    next = list_next(e);
    struct thread_file_context *process_file_ptr = list_entry(e, struct thread_file_context, elem);
    if (fd == process_file_ptr->fd)
    {
      return process_file_ptr->file;
    }
  }
  return NULL; // nothing found
}

/* Close the file */
void
terminate_file_access (int fd)
{
  struct thread *t = thread_current();
  struct list_elem *next;
  struct list_elem *e = list_begin(&t->file_handle_list);
  
  for (;e != list_end(&t->file_handle_list); e = next)
  {
    next = list_next(e);
    struct thread_file_context *process_file_ptr = list_entry (e, struct thread_file_context, elem);
    if (fd == process_file_ptr->fd || fd == -1)
    {
      file_close(process_file_ptr->file);
      list_remove(&process_file_ptr->elem);
      free(process_file_ptr);
      if (fd != -1)
      {
        return;
      }
    }
  }
}

/* Find a specific child process */
struct child_process* get_child_process_by_pid(int pid)
{
  struct thread *t = thread_current();
  struct list_elem *e;
  struct list_elem *next;
  
  for (e = list_begin(&t->child_list); e != list_end(&t->child_list); e = next)
  {
    next = list_next(e);
    struct child_process *cp = list_entry(e, struct child_process, elem);
    if (pid == cp->pid)
    {
      return cp;
    }
  }
  return NULL;
}

/* Remove a specific child process */
void
delete_child_process (struct child_process *cp)
{
  list_remove(&cp->elem);
  free(cp);
}

/* Remove all child processes of the current thread */
void cleanup_child_processes (void) 
{
  struct thread *t = thread_current();
  struct list_elem *next;
  struct list_elem *e = list_begin(&t->child_list);
  
  for (;e != list_end(&t->child_list); e = next)
  {
    next = list_next(e);
    struct child_process *cp = list_entry(e, struct child_process, elem);
    list_remove(&cp->elem); // remove child process
    free(cp);
  }
}
