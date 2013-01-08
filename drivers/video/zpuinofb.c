/*
 * ZPUino VGA frame buffer driver
 *
 * (C) Alvaro Lopes 2013
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

/*
 * Based on Xilinx TFT driver, (C) Montavista Software.
 *
 * This driver was based on au1100fb.c by MontaVista rewritten for 2.6
 * by Embedded Alley Solutions <source@embeddedalley.com>, which in turn
 * was based on skeletonfb.c, Skeleton for a frame buffer device by
 * Geert Uytterhoeven.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/zpuinofb.h>
#include <linux/slab.h>
#include <asm/sysreg.h>


#define DRIVER_NAME		"zpuino_fb"

#define BYTES_PER_PIXEL	 2
#define BITS_PER_PIXEL	 (BYTES_PER_PIXEL * 8)

#define RED_SHIFT   10
#define GREEN_SHIFT 5
#define BLUE_SHIFT	0

#define PALETTE_ENTRIES_NO	16	/* passed to fb_alloc_cmap() */

/*
 * Default zpuino VGA configuration
 */
static struct zpuinofb_platform_data zpuino_fb_default_pdata = {
	.xres = 640,
	.yres = 480,
	.xvirt = 1024,  /* This allows scrolling */
	.yvirt = 480,
};

/*
 * Here are the default fb_fix_screeninfo and fb_var_screeninfo structures
 */
static struct fb_fix_screeninfo zpuino_fb_fix = {
	.id =		"ZPUino",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_TRUECOLOR,
	.accel =	FB_ACCEL_NONE
};

static struct fb_var_screeninfo zpuino_fb_var = {
	.bits_per_pixel =	BITS_PER_PIXEL,

	.red =		{ RED_SHIFT, 8, 0 },
	.green =	{ GREEN_SHIFT, 8, 0 },
	.blue =		{ BLUE_SHIFT, 8, 0 },
	.transp =	{ 0, 0, 0 },

	.activate =	FB_ACTIVATE_NOW
};


struct zpuinofb_drvdata {

	struct fb_info	info;		/* FB driver info record */

	phys_addr_t	regs_phys;	/* phys. address of the control registers */
	void		*fb_virt;	/* virt. address of the frame buffer */
	dma_addr_t	fb_phys;
	int		fb_alloced;	/* Flag, was the fb memory alloced? */

	u8 		flags;		/* features of the driver */

	u32		reg_ctrl_default;

	//u32		pseudo_palette[PALETTE_ENTRIES_NO];
					/* Fake palette of 16 colors */
};

#define to_zpuinofb_drvdata(_info) \
	container_of(_info, struct xilinxfb_drvdata, info)

static int
zpuino_fb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
	unsigned transp, struct fb_info *fbi)
{
#if 0
	u32 *palette = fbi->pseudo_palette;

	if (regno >= PALETTE_ENTRIES_NO)
		return -EINVAL;

	if (fbi->var.grayscale) {
		/* Convert color to grayscale.
		 * grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue =
			(red * 77 + green * 151 + blue * 28 + 127) >> 8;
	}

	/* fbi->fix.visual is always FB_VISUAL_TRUECOLOR */

	/* We only handle 8 bits of each color. */
	red >>= 8;
	green >>= 8;
	blue >>= 8;
	palette[regno] = (red << RED_SHIFT) | (green << GREEN_SHIFT) |
			 (blue << BLUE_SHIFT);
#endif
	return 0;
}

static int
zpuino_fb_blank(int blank_mode, struct fb_info *fbi)
{
#if 0
	struct xilinxfb_drvdata *drvdata = to_xilinxfb_drvdata(fbi);

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		/* turn on panel */
		xilinx_fb_out_be32(drvdata, REG_CTRL, drvdata->reg_ctrl_default);
		break;

	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_POWERDOWN:
		/* turn off panel */
		xilinx_fb_out_be32(drvdata, REG_CTRL, 0);
	default:
		break;

	}
#endif
	return 0; /* success */
}


static int zpuino_fb_mmap(struct fb_info*info, struct vm_area_struct * vma)
{
        return -ENOMEM;
}

static struct fb_ops zpuinofb_ops =
{
	.owner			= THIS_MODULE,
	.fb_setcolreg		= zpuino_fb_setcolreg,
	.fb_blank		= zpuino_fb_blank,
//	.fb_mmap		= zpuino_fb_mmap,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
};

/* ---------------------------------------------------------------------
 * Bus independent setup/teardown
 */

static int zpuinofb_assign(struct device *dev,
			   struct zpuinofb_drvdata *drvdata,
			   unsigned long physaddr,
			   struct zpuinofb_platform_data *pdata)
{
	int rc;
	int fbsize = pdata->xvirt * pdata->yvirt * BYTES_PER_PIXEL;
	struct resource *r;

	if (!request_mem_region(physaddr, 16,
				DRIVER_NAME)) {
		dev_err(dev,"Cannot allocate mem region\n");
		return -ENODEV;
	}
        
	drvdata->regs_phys = physaddr;

	/* Allocate the framebuffer memory */
	if (pdata->fb_phys) {
		drvdata->fb_phys = pdata->fb_phys;
	} else {
		drvdata->fb_alloced = 1;
                /*
		dma_alloc_coherent(dev, fbsize,
		&drvdata->fb_phys, GFP_KERNEL);
		*/
                drvdata->fb_phys=(dma_addr_t)kmalloc(fbsize,GFP_KERNEL);
	}

	if (!drvdata->fb_phys) {
		dev_err(dev, "Could not allocate frame buffer memory\n");
		rc = -ENOMEM;
		goto err_fbmem;
	}

	iowrite32(drvdata->fb_phys, (void __iomem*)physaddr);

	/* Fill struct fb_info */
	drvdata->info.device = dev;
	drvdata->info.screen_base = (void __iomem *)drvdata->fb_phys;
	drvdata->info.fbops = &zpuinofb_ops;
	drvdata->info.fix = zpuino_fb_fix;
	drvdata->info.fix.smem_start = drvdata->fb_phys;
	drvdata->info.fix.smem_len = fbsize;
	drvdata->info.fix.line_length = pdata->xvirt * BYTES_PER_PIXEL;

	//drvdata->info.pseudo_palette = drvdata->pseudo_palette;
	drvdata->info.flags = FBINFO_DEFAULT;
	drvdata->info.var = zpuino_fb_var;
	//drvdata->info.var.height = pdata->screen_height_mm;
	//drvdata->info.var.width = pdata->screen_width_mm;
	drvdata->info.var.xres = pdata->xres;
	drvdata->info.var.yres = pdata->yres;
	drvdata->info.var.xres_virtual = pdata->xvirt;
	drvdata->info.var.yres_virtual = pdata->yvirt;

	/* Allocate a colour map */
	rc = fb_alloc_cmap(&drvdata->info.cmap, PALETTE_ENTRIES_NO, 0);
	if (rc) {
		dev_err(dev, "Fail to allocate colormap (%d entries)\n",
			PALETTE_ENTRIES_NO);
		goto err_cmap;
	}

	/* Register new frame buffer */
	rc = register_framebuffer(&drvdata->info);
	if (rc) {
		dev_err(dev, "Could not register frame buffer\n");
		goto err_regfb;
	}
        printk(KERN_INFO "ZPUino framebuffer allocated at 0x%08x\n", drvdata->fb_phys);
	return 0;	/* success */

err_regfb:
	fb_dealloc_cmap(&drvdata->info.cmap);

err_cmap:
	if (drvdata->fb_alloced)
		dma_free_coherent(dev, PAGE_ALIGN(fbsize), drvdata->fb_virt,
			drvdata->fb_phys);
	else
		iounmap(drvdata->fb_virt);

	/* Turn off the display */
	//xilinx_fb_out_be32(drvdata, REG_CTRL, 0);

err_fbmem:
	//if (drvdata->flags & PLB_ACCESS_FLAG)
	//iounmap(drvdata->regs);

err_map:
	//if (drvdata->flags & PLB_ACCESS_FLAG)
	release_mem_region(physaddr, 8);

err_region:
	kfree(drvdata);
	dev_set_drvdata(dev, NULL);

	return rc;
}

static int zpuinofb_release(struct device *dev)
{
	struct zpuinofb_drvdata *drvdata = dev_get_drvdata(dev);

#if !defined(CONFIG_FRAMEBUFFER_CONSOLE) && defined(CONFIG_LOGO)
	zpuino_fb_blank(VESA_POWERDOWN, &drvdata->info);
#endif

	unregister_framebuffer(&drvdata->info);

	fb_dealloc_cmap(&drvdata->info.cmap);

	if (drvdata->fb_alloced)
		dma_free_coherent(dev, PAGE_ALIGN(drvdata->info.fix.smem_len),
				  drvdata->fb_virt, drvdata->fb_phys);
	else
		iounmap(drvdata->fb_virt);

	/* Turn off the display */
	//xilinx_fb_out_be32(drvdata, REG_CTRL, 0);

	/* Release the resources, as allocated based on interface */
	//if (drvdata->flags & PLB_ACCESS_FLAG) {
	//iounmap(drvdata->regs);
	release_mem_region(drvdata->regs_phys, 8);
	//}

	kfree(drvdata);
	dev_set_drvdata(dev, NULL);

	return 0;
}

/* ---------------------------------------------------------------------
 * OF bus binding
 */

static int __devinit zpuinofb_probe(struct platform_device *op)
{
	const u32 *prop;
	u32 *p;
	u32 tft_access;
	struct zpuinofb_platform_data pdata;
	struct resource *r;
	int size, rc;
	struct zpuinofb_drvdata *drvdata;

	/* Copy with the default pdata (not a ptr reference!) */
	pdata = zpuino_fb_default_pdata;

	r = platform_get_resource(op, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&op->dev, "Cannot get resource\n");
		return -ENOMEM;
	}

	/* Allocate the driver data region */
	drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata) {
		dev_err(&op->dev, "Couldn't allocate device private record\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&op->dev, drvdata);
	return zpuinofb_assign(&op->dev, drvdata, r->start, &pdata);

 err:
	kfree(drvdata);
	return -ENODEV;
}

static int __devexit zpuinofb_remove(struct platform_device *dev)
{
	int err = zpuinofb_release(platform_get_drvdata(dev));
	platform_set_drvdata(dev, 0);
	return err;
}

static struct platform_driver zpuinofb_of_driver = {
	.probe = zpuinofb_probe,
	.remove = __devexit_p(zpuinofb_remove),
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(zpuinofb_of_driver);

MODULE_AUTHOR("Alvaro Lopes <alvieboy@alvie.com>");
MODULE_DESCRIPTION("ZPUino VGA frame buffer driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);

