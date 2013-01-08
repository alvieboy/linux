/*
 * Platform device data for ZPUino Framebuffer device
 *
 * Copyright 2013 Alvaro Lopes
 * Copyright 2007 Secret Lab Technologies Ltd.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef __ZPUINOFB_H__
#define __ZPUINOFB_H__

#include <linux/types.h>

struct zpuinofb_platform_data {
	//u32 rotate_screen;	/* Flag to rotate display 180 degrees */
//	u32 screen_height_mm;	/* Physical dimensions of screen in mm */
  //      u32 screen_width_mm;
	u32 xres, yres;		/* resolution of screen in pixels */
	u32 xvirt, yvirt;	/* resolution of memory buffer */

	/* Physical address of framebuffer memory; If non-zero, driver
	 * will use provided memory address instead of allocating one from
	 * the consistent pool. */
	u32 fb_phys;
    //    u32 ioaddress;
};

#endif  /* __XILINXFB_H__ */
