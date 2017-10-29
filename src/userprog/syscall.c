#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

/* Prototype for syscall methods */
int sys_write (uint32_t fd, void *buf, uint32_t size);
void sys_exit (int s);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f UNUSED) {
  /* Cast interrupt frame's stack pointer to an integer pointer */
  uint32_t *esp = f->esp;

  printf ("\nSystem call invoked with syscall code %d!\n", *esp);
  switch (*esp) {
    case SYS_HALT:
      printf ("Invoking SYS_HALT");
      break;
    case SYS_EXIT:
      printf ("Invoking SYS_EXIT");
      sys_exit (*(esp + 1));
      break;
    case SYS_EXEC:
      printf ("Invoking SYS_EXEC");
      break;
    case SYS_WAIT:
      printf ("Invoking SYS_WAIT");
      break;
    case SYS_CREATE:
      printf ("Invoking SYS_CREATE");
      break;
    case SYS_REMOVE:
      printf ("Invoking SYS_REMOVE");
      break;
    case SYS_OPEN:
      printf ("Invoking SYS_OPEN");
      break;
    case SYS_FILESIZE:
      printf ("Invoking SYS_FILESIZE");
      break;
    case SYS_READ:
      printf ("Invoking SYS_READ");
      break;
    case SYS_WRITE:
      printf ("Invoking SYS_WRITE");
      sys_write (*(esp + 1), *(esp + 2), *(esp + 3));
      break;
    case SYS_SEEK:
      printf ("Invoking SYS_SEEK");
      break;
    case SYS_TELL:
      printf ("Invoking SYS_TELL");
      break;
    case SYS_CLOSE:
      printf ("Invoking SYS_CLOSE");
      break;
  }
  thread_exit ();
}

void sys_exit (int status) {
  struct thread *t = thread_current ();
  printf ("%s: exit(%d)\n", t->name, status);
  thread_exit ();
}

int sys_write (uint32_t fd, void* buffer, uint32_t buffer_size) {
  int status = 0;

  switch (fd) {
    case STDIN_FILENO:
      // Cannot write to standard input
      status = -1;
      break;
    case STDOUT_FILENO:
      putbuf (buffer, buffer_size);
      status = buffer_size;
      break;
  }

  return status;
}