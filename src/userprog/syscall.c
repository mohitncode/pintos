#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

/* Prototype for syscall methods */
int sys_write (int fd, void *buf, int size);
void sys_exit (int s);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f ) {
  /* Cast interrupt frame's stack pointer to an integer pointer */
  
    int32_t *es=(int32_t *)f->esp;
    switch (*es) {
    case SYS_HALT:
      shutdown_power_off ();
      break;
    case SYS_EXIT:
      sys_exit (*(es + 1));
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
      f->eax = sys_write ((int32_t)f->esp+1, (void *)f->esp+2,(int32_t)f->esp+3);
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
  //printf ("\nTrying to write to fd %d with buffer address %p buffer size %d", fd, buffer, buffer_size);

   if (fd == STDIN_FILENO) {
    // Cannot write to standard input
     status = -1;
   } else if (fd == STDOUT_FILENO) {
    putbuf (*(int *)buffer, buffer_size);
    status = buffer_size;
   }

  return status;
}