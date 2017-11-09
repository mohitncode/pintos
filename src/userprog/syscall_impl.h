struct lock filesystem_lock;

/* File descriptor structure */
struct file_descriptor {
  int fid;
  struct file *file_ref;
  struct list_elem file_elem;
};

/* Prototype for syscall methods */
int sys_write (int fd, void *buf, int size);
void sys_exit (int s);
int validate_ptr (void *uptr);
int sys_create (char *name, int initial_size);
int sys_open (char *name);
int sys_read (int fd, void *buffer, int size);
int sys_filesize (int fd);
void sys_seek (int fd, int location);
int sys_tell (int fd);
int sys_exec (char *file);
void sys_close (int fd);

void close_process_files (struct thread *process);
