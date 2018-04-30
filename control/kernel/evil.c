#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/vmalloc.h>

char *filename;
module_param(filename, charp, 0644);


/**
 * d_path - return the path of a dentry
 * @path: path to report
 * @buf: buffer to return value in
 * @buflen: buffer length
 *
 * Convert a dentry into an ASCII path name. If the entry has been deleted
 * the string " (deleted)" is appended. Note that this is ambiguous.
 *
 * Returns a pointer into the buffer or an error code if the path was
 * too long. Note: Callers should use the returned pointer, not the passed
 * in buffer, to use the name! The implementation often starts at an offset
 * into the buffer, and may leave 0 bytes at the start.
 *
 * "buflen" should be positive.
 */
//char *d_path(const struct path *path, char *buf, int buflen)

static int
file_getattr(char *name)
{
	int ccode = 0;
	struct file *f;
	struct kstat k_stat;
	u32 mask = STATX_ALL;
	unsigned int flags = KSTAT_QUERY_FLAGS;

	memset(&k_stat, 0x00, sizeof(struct kstat));
	f = filp_open(name, O_RDONLY, 0);
	if (f) {
		ccode = vfs_getattr(&f->f_path, &k_stat, mask, flags);
		printk("evil-vfs_getattr returned %d\n", ccode);
		printk("%s size: %lld\n", name, k_stat.size);
		filp_close(f, 0);
	}

	return ccode;

}


static int read_file(char *name)
{
  static char __data[0x1000];
  loff_t size = 0L;
  int ccode;
  struct file *f;

  f = filp_open(name, O_RDONLY, 0);
  if (f) {
	  printk("we have a file pointer %p\n", f);
	  printk("evil-flags %x\n", f->f_flags);
	  ccode = kernel_read(f, &__data[0], 0x100, &size);
	  printk("evil-ccode is %d, size is %llx\n", ccode, size);
	  if (ccode < 0) {
		   pr_err("Unable to open file: %s (%d)", name, ccode);
		   filp_close(f, 0);
		   return ccode;
	  }
	  printk("%s\n", (char *)__data);
	  filp_close(f, 0);
  }
  return ccode;
}

static int __init init(void)
{
  read_file(filename);
  file_getattr(filename);

  return 0;
}

static void __exit exit(void)
{ }

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
