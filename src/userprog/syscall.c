#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
static int fd_count = 2;

/* Prototype for syscall methods */
int sys_write (int fd, void *buf, int size);
void sys_exit (int s);
bool is_valid_ptr (void* uptr);
int create(const char *name, int initial_size);
int open (const char *name);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f ) {
  /* Cast interrupt frame's stack pointer to an integer pointer */

  int32_t *esp = f->esp;

  switch (*esp) {
    case SYS_HALT:
      shutdown_power_off ();
      break;
    case SYS_EXIT:
      sys_exit (*(esp + 1));
      break;
    case SYS_EXEC:
      break;
    case SYS_WAIT:
      break;
    case SYS_CREATE:
       f->eax = create((char *) *(esp + 1), *(esp + 2));
       break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
       f->eax = open ((char *) *(esp + 1));
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      break;
    case SYS_WRITE:
      f->eax = sys_write (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
  }
}

void sys_exit (int status) {
  struct thread *t = thread_current ();
  struct list children = t->parent->child_threads;
  struct list_elem *e;
  /*
    Look for current ID in parent's list of child threads and set it's
    exit status in the list element's child structure
  */
  for (e = list_begin (&children); e != list_end (&children); e = list_next (e)) {
    struct child_thread_status *c = list_entry (e, struct child_thread_status, child_elem);

    /* If child's ID is equal to current thread's ID */
    if (c->tid == t->tid) {
      c->has_exited = true;
      c->exit_code = status;
      sema_up (&c->wait_sema);
      break;
    }
  }

  printf ("%s: exit(%d)\n", t->name, status);
  thread_exit ();
}

int sys_write (int fd, void* buffer, int buffer_size) {
  int status = 0;

  if (is_valid_ptr (buffer) && is_valid_ptr (buffer + buffer_size)) {
    if (fd == STDIN_FILENO) {
      // Cannot write to standard input
      status = -1;
    } else if (fd == STDOUT_FILENO) {
     putbuf (buffer, buffer_size);
     status = buffer_size;
    }
  } else {
    printf ("Calling sys_exit with -1");
    sys_exit (-1);
  }

  return status;
}

bool is_valid_ptr (void* uptr) {
  return (is_user_vaddr (uptr)
    && pagedir_get_page (thread_current () ->pagedir, uptr) != NULL);
}

int create(const char *name, int initial_size){
  return  filesys_create(name,initial_size);
}

int open (const char *name){
  int status = -1;
  struct file *f = filesys_open(name);
  if(f != NULL){
    status = fd_count;
    fd_count++;
  }
  return status;
}