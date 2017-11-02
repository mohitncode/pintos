#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

/* Prototype for syscall methods */
int sys_write (int fd, void *buf, int size);
void sys_exit (int s);
bool is_valid_ptr (void* uptr);

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
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
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
     putbuf ((char *) buffer, buffer_size);
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