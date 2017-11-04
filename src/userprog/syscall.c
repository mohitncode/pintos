#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

struct lock filesystem_lock;

/* File descriptor structure */
struct file_descriptor {
  int fid;
  struct file *file_ref;
  struct list_elem file_elem;
};

static void syscall_handler (struct intr_frame *);

/* Prototype for syscall methods */
int sys_write (int fd, void *buf, int size);
void sys_exit (int s);
int validate_ptr (void* uptr);
int sys_create(const char *name, int initial_size);
int sys_open (const char *name);
int sys_read (int fd, void *buffer, off_t size);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&filesystem_lock);
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
       f->eax = sys_create ((char *) *(esp + 1), *(esp + 2));
       break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
       f->eax = sys_open ((char *) *(esp + 1));
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      f->eax = sys_read (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
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
  int status = -1;

  if (validate_ptr (buffer) && validate_ptr (buffer + buffer_size)) {
    if (fd == STDIN_FILENO) {
      // Cannot write to standard input
      status = -1;
    } else if (fd == STDOUT_FILENO) {
      lock_acquire (&filesystem_lock);
      putbuf (buffer, buffer_size);
      status = buffer_size;
      lock_release (&filesystem_lock);
    }
  }

  return status;
}

int validate_ptr (void* uptr) {
  if (is_user_vaddr (uptr)
    && pagedir_get_page (thread_current () ->pagedir, uptr) != NULL) {
    return 1;
  } else {
    return 0;
  }
}

int sys_create (const char *name, int initial_size) {
  int status = -1;
  lock_acquire (&filesystem_lock);
  status = filesys_create (name, initial_size);
  lock_release (&filesystem_lock);
  return status;
}

int sys_open (const char *name){
  int status = -1;
  lock_acquire (&filesystem_lock);
  struct file *f = filesys_open (name);
  lock_release (&filesystem_lock);

  if (f != NULL) {
    struct thread *t = thread_current ();
    struct file_descriptor *fd;
    fd = calloc (1, sizeof *fd);

    fd->fid = t->next_fd;
    t->next_fd++;
    fd->file_ref = f;
    list_push_back (&t->files , &fd->file_elem);
    status = fd->fid;
  }

  return status;
}


int sys_read (int fd, void *buffer, off_t size) {
  int status = -1;
  struct list open_files = thread_current ()->files;
  struct list_elem *e;

  if (validate_ptr (buffer) && validate_ptr (buffer + size)) {
    // printf ("\nTrying to open file with FD %d\n", fd);
    if (fd == STDIN_FILENO) {
      int *temp;
      int counter = size;

      lock_acquire (&filesystem_lock);
      while (counter-- && (*temp = input_getc()) != 0) {
        temp++;
      }
      lock_release (&filesystem_lock);

      status = size;
    } else if (fd == STDOUT_FILENO) {
      status = -1;
    } else {
      for (e = list_begin (&open_files); e != list_end (&open_files); e = list_next (e)) {
        struct file_descriptor *f = list_entry (e, struct file_descriptor, file_elem);

        /* If child's ID is equal to current thread's ID */
        if (f->fid == fd) {
          // printf ("\nFound file with FID %d!\n", fd);
          lock_acquire (&filesystem_lock);
          status = file_read (f->file_ref, buffer, size);
          // printf ("\nRead file successfully with status %d!\n", status);
          lock_release (&filesystem_lock);
          break;
        }
      }
    }
  }

  return status;
}