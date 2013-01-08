/*
 */
#include <linux/unistd.h>


int sys_cacheflush()
{
	return -1;
}

#if defined(CONFIG_FB) || defined(CONFIG_FB_MODULE)

#include <linux/fb.h>
#include <linux/export.h>

unsigned long get_fb_unmapped_area(struct file *filp, unsigned long orig_addr,
				   unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct fb_info *info = filp->private_data;
	return (unsigned long)info->screen_base;
}

EXPORT_SYMBOL(get_fb_unmapped_area);

#endif
