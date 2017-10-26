#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f UNUSED) {
  /* Cast interrupt frame's stack pointer to an integer pointer */
  int *syscall_code = (int *) (f->esp);
  printf ("System call invoked with syscall code %d!\n", *syscall_code);
  switch (*syscall_code) {
    case SYS_HALT:
      printf ("Invoking SYS_HALT");
      break;
    case SYS_EXIT:
      printf ("Invoking SYS_EXIT");
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
