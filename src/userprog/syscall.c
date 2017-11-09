#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/syscall_impl.h"

void *USER_CODE_SEGMENT=(void *)(0x08048000);

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&filesystem_lock);
}

static void syscall_handler (struct intr_frame *f ) {
  /* Cast interrupt frame's stack pointer to an integer pointer */

  int32_t *esp = f->esp;

  if(0 == validate_ptr (esp)) {
    sys_exit (-1);
  } else if ((void *) esp < USER_CODE_SEGMENT) {
    sys_exit (-1);
  } else if (0 == validate_ptr (esp + 1)) {
    sys_exit (-1);
  } else if ((void *) (esp + 1) < USER_CODE_SEGMENT) {
    sys_exit (-1);
  } else {
    if (SYS_HALT == *esp) {
      shutdown_power_off ();
    } else if (SYS_EXIT == *esp) {
      sys_exit (*(esp + 1));
    } else if (SYS_EXEC == *esp) {
      f->eax = sys_exec ((char *) *(esp + 1));
    } else if (SYS_WAIT == *esp) {
      f->eax = process_wait (*(esp + 1));
    } else if (SYS_CREATE == *esp) {
      f->eax = sys_create ((char *) *(esp + 1), *(esp + 2));
    } else if (SYS_REMOVE == *esp) {
      // TODO
    } else if (SYS_OPEN == *esp) {
      f->eax = sys_open ((char *) *(esp + 1));
    } else if (SYS_FILESIZE == *esp) {
      f->eax = sys_filesize (*(esp + 1));
    } else if (SYS_READ == *esp) {
      f->eax = sys_read (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
    } else if (SYS_WRITE == *esp) {
      f->eax = sys_write (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
    } else if (SYS_SEEK == *esp) {
      sys_seek (*(esp + 1),*(esp + 2));
    } else if (SYS_TELL == *esp) {
      f->eax = sys_tell(*(esp + 1));
    } else if (SYS_CLOSE == *esp) {
      // TODO
    } else {
      sys_exit (-1);
    }
  }
}

int sys_exec (char *args) {
  int thread_id;
  if (validate_ptr (args)) {
    thread_id = process_execute (args);
  } else {
    sys_exit (-1);
  }
  return thread_id;
}

int sys_tell (int fd){
  struct thread *t = thread_current ();
  struct list_elem *e;
  int status = -1;

  lock_acquire (&filesystem_lock);
  for (e = list_begin (&t->files); e != list_end (&t->files); e = list_next (e)) {
    struct file_descriptor *f = list_entry (e, struct file_descriptor, file_elem);
     if (f->fid == fd) {
     status= file_tell(f->file_ref);
     break;
    }
  }
  lock_release(&filesystem_lock);
  return status;

}

void sys_seek (int fd, int loc) {
  struct thread *t = thread_current ();
  struct list_elem *e;

  lock_acquire (&filesystem_lock);
  for (e = list_begin (&t->files); e != list_end (&t->files); e = list_next (e)) {
    struct file_descriptor *f = list_entry (e, struct file_descriptor, file_elem);
     if (f->fid == fd) {
      file_seek(f->file_ref, loc);
      break;
    }

  }
  lock_release (&filesystem_lock);
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
      c->exit_code = status;
      printf ("%s: exit(%d)\n", t->name, status);
      sema_up (&c->wait_sema);
      break;
    }
  }

  thread_exit ();
}

int sys_write (int fd, void* buffer, int buffer_size) {
  int status = -1;

  if (validate_ptr (buffer) && validate_ptr (buffer + buffer_size)) {
    lock_acquire (&filesystem_lock);

    /* We ignore the case where fd = STDIN as we can't write to it */
    if (fd == STDOUT_FILENO) {
      putbuf (buffer, buffer_size);
      status = buffer_size;
    } else if (fd > STDOUT_FILENO) {
      struct thread *t = thread_current ();
      struct list_elem *e;

      for (e = list_begin (&t->files); e != list_end (&t->files); e = list_next (e)) {
        struct file_descriptor *f = list_entry (e, struct file_descriptor, file_elem);

        /* If child's ID is equal to current thread's ID */
        if (f->fid == fd) {
          // printf ("\nFound file with FID %d!\n", fd);
          status = file_write (f->file_ref, buffer, buffer_size);
          // printf ("\Wrote file successfully with status %d!\n", status);
          break;
        }
      }
    }
    lock_release (&filesystem_lock);
  } else {
    sys_exit (status);
  }

  return status;
}

int validate_ptr (void *uptr) {
  if (NULL == uptr) {
    return 0;
  } else if (0 == is_user_vaddr (uptr)) {
    return 0;
  } else if (uptr < USER_CODE_SEGMENT) {
    return 0;
  } else if(NULL == pagedir_get_page (thread_current ()->pagedir, uptr)) {
    return 0;
  } else {
    return 1;
  }
}

int sys_create (char* name, int initial_size) {
  int status = -1;
  if (validate_ptr (name)) {
    lock_acquire (&filesystem_lock);
    status = filesys_create (name, initial_size);
    lock_release (&filesystem_lock);
  } else {
    sys_exit (status);
  }
  return status;
}

int sys_open (char* name){
  int status = -1;

  if (validate_ptr (name)) {
    lock_acquire (&filesystem_lock);
    struct file *f = filesys_open (name);

    lock_release (&filesystem_lock);
    if (f != NULL) {
      struct thread *t = thread_current ();

      // If the file requested for opening is the current thread's executable
      // deny writes to it
      if (0 == strcmp (name, t->name) || 0 == (strcmp (name, t->parent->name))){
        file_deny_write (f);
      }

      struct file_descriptor *fd;
      fd = malloc (sizeof *fd);

      fd->fid = t->next_fd;
      t->next_fd++;
      fd->file_ref = f;
      list_push_back (&t->files , &fd->file_elem);
      status = fd->fid;
    }
  } else {
    sys_exit (status);
  }

  return status;
}

int sys_filesize (int fd) {
  int status = -1;
  struct thread *t = thread_current ();
  struct list_elem *e;

  for (e = list_begin (&t->files); e != list_end (&t->files); e = list_next (e)) {
    struct file_descriptor *f = list_entry (e, struct file_descriptor, file_elem);

    /* If child's ID is equal to current thread's ID */
    if (f->fid == fd) {
      // printf ("\nFound file with FID %d!\n", fd);
      lock_acquire (&filesystem_lock);
      status = file_length (f->file_ref);
      // printf ("\nRead file successfully with status %d!\n", status);
      lock_release (&filesystem_lock);
      break;
    }
  }

  return status;
}

int sys_read (int fd, void *buffer, int size) {
  int status = -1;

  if (validate_ptr (buffer) && validate_ptr (buffer + size)) {
    // printf ("\nTrying to open file with FD %d\n", fd);
    lock_acquire (&filesystem_lock);

    /* We ignore the case where fd = STDOUT as we can't read from it */
    if (fd == STDIN_FILENO) {
      int read_bytes = 0;
      while (input_getc() != 0) {
        read_bytes++;
      }
      // printf ("Read bytes = %d and size = %d\n", read_bytes, size);

      status = read_bytes;
    } else if (fd > 1) {
      struct thread *t = thread_current ();
      struct list_elem *e;
      for (e = list_begin (&t->files); e != list_end (&t->files); e = list_next (e)) {
        struct file_descriptor *f = list_entry (e, struct file_descriptor, file_elem);

        /* If child's ID is equal to current thread's ID */
        if (f->fid == fd) {
          // printf ("\nFound file with FID %d!\n", fd);
          status = file_read (f->file_ref, buffer, size);
          // printf ("\nRead file successfully with status %d!\n", status);
          break;
        }
      }
    }
    lock_release (&filesystem_lock);
  } else {
    sys_exit (-1);
  }

  return status;
}