#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include <stdlib.h>
#include "threads/vaddr.h"
static void syscall_handler (struct intr_frame *);
static int syscall_write(int fd, void *buffer, int size	);
static struct list list_files;
struct file_handler
{
    struct file *file;
    int fd;
    struct list_elem elem;
    struct list_elem l_elem;
};

static struct file_handler* fh_search(int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init(&list_files);
}

static struct file_handler* fh_search(int fd)
{
  struct file_handler * ret_val;
  struct list_elem *l;
  struct thread *t;
  t = thread_current();
  for(l= list_begin(&t->open_files); l!=list_end(&t->open_files);l=list_next(l))  {
    ret_val=list_entry(l, struct file_handler, l_elem);
    if(ret_val->fd == fd)
      return ret_val;
  }
  return -1;
}

static int syscall_read(int fd, char*buffer, unsigned size){
  unsigned i;
 /* if(!fd)
  {
  for (i = 0; i < size; ++i)
    *(buffer++) = input_getc ();
  return size;
  }*/
  struct file *f;
  int ret_val=-1;
  if(fd == STDIN_FILENO)
  {
    for(i=0;i!=size;++i)
    {
      *(uint8_t *)(buffer+i) = input_getc();
    }
    ret_val=size;
    return ret_val;
  }
  else if(fd == STDOUT_FILENO)
    return ret_val;
  else if(!is_user_vaddr(buffer) || !is_user_vaddr(buffer+size))
  {
    syscall_exit(-1);
  }
  return ret_val;
}

static int syscall_write(int fd, void *buffer, int size){
//  printf("In sys_write %d 0x%x\n", fd,buffer);
	if(fd==1){//1 is stdout
    //printf("inside write: %s %d\n", buffer, size);
		putbuf((char *)buffer,size);
	}
	return size;
}
int syscall_wait(int tid)
{
//  printf("%s waits for tid == %d", thread_current()->name, tid);
  int ret = process_wait(tid);
  return ret;
}
int syscall_exit(int status)
{
  struct thread *t = thread_current();
  struct task_struct *cur_info = t->info;
  printf("%s: exit(%d)\n", t->name, status);
  if(cur_info->isParentAlive)
  {
    cur_info->exit_status = status;
    sema_up(&(cur_info->parent_block));
  }
  else
  {
    list_remove(&(cur_info->elem));
    free(cur_info);
  }
  /*
   * Notify child processes
   * */
  thread_exit();
}

/*
translate_vaddr (const void *vaddr)
{
    if (!is_user_vaddr (vaddr))
      return NULL;
    return pagedir_get_page (thread_current ()->pagedir, vaddr);
}

static int
validate_str (const char *vstr, int max_length)
{
    int i;
    for (i = 0; i <= max_length; ++i)
    {
      const char *c = translate_vaddr (vstr++);
      if (c == NULL)
        return -1;
      if (*c == '\0')
        return i;
    }
                          
    return max_length + 1;
}*/

static int
syscall_create(const char *file, unsigned size)
{
  if(!file)
    return syscall_exit(-1);
  return filesys_create(file,size);
}

syscall_open(const char *file)
{
  struct file *f;
  struct file_handler *fdh;
  int ret_val=-1;
  if(!file)
    return -1;
  if(!is_user_vaddr(file))
    syscall_exit(-1);
  f=filesys_open(file);
  if(!f)
    return ret_val;
  fdh = (struct file_handler *)malloc(sizeof (struct file_handler));
  if(!fdh)
  {
    file_close(f);
    return ret_val;
  }
  fdh->file=f;
  fdh->fd=2;
  list_push_back(&list_files, &fdh->elem);
  list_push_back(&thread_current()->open_files,&fdh->l_elem);
  ret_val=fdh->fd;
  return ret_val;
}

static int
syscall_close(int fd)
{
  struct file_handler *f;
  int ret_val;
  f=fh_search(fd);
  if(f == -1)
    return 0;
  file_close(f->file);
  list_remove(&f->elem);
  list_remove(&f->l_elem);
  free(f);
  return 0;
}

static int
syscall_remove(const char* file)
{
  if(!file)
    return false;
  if(!is_user_vaddr(file))
    syscall_exit(-1);
  return filesys_remove(file);
}

static tid_t
syscall_exec (const char *cmd_line)
{
    int cmd_line_size;
    /*
    if ((cmd_line_size = validate_str (cmd_line, PGSIZE)) == -1)
            syscall_exit (-1);

    if (cmd_line_size > PGSIZE)
      return TID_ERROR;*/

    tid_t tid = process_execute (cmd_line);
     // return tid == TID_ERROR ? -1 : tid;
    return tid;
}
static void
syscall_handler (struct intr_frame *f ) 
{
//  printf ("system call!\n");
  int *esp = f->esp;
  bool success=false;
  int val=-1;
  uint8_t syscall_number = *((uint8_t*)esp);
  //printf("%d <--syscall\n", syscall_number);
//  hex_dump(0, esp, PHYS_BASE - (void *)esp, true);
  esp++;
  switch(syscall_number)
  {
    case SYS_WRITE:
      success=true;
      int fd = *(esp++);
      void *buffer = (void*)(*(esp++));
      int size = (int)(*(esp));
      val=syscall_write(fd, buffer, size);
      break;
    case SYS_EXIT : 
      success =true;
      syscall_exit((int)(*esp));
      break;
    case SYS_READ:
      success=true;
      val=syscall_read((int)(*esp), (void*)*(esp+1),
          (unsigned)*(esp+2));
      break;
    case SYS_WAIT:
      f->eax = syscall_wait((int)*esp);
      break;
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXEC:
          success = true;
          val = syscall_exec ((const char*)(*esp));
          break;
    case SYS_CREATE:
          success=true;
          val=syscall_create((const char*)(*esp),(unsigned)(*(esp+1)));
          break;
    case SYS_OPEN:
          success = true;
          val=syscall_open((const char*)(*esp));
          break;
    case SYS_CLOSE:
          success=true;
          syscall_close((int)(*esp));
          break;
    case SYS_REMOVE:
      success=true;
      val=syscall_remove((const char*)(*esp));
      break;
  }
  if(success){
  	f->eax= val;
//    printf("Success %d\n", syscall_number);
  }
}
