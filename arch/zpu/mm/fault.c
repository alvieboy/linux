/*
 * Copyright (C) 2004-2006 Atmel Corporation
 *
 * Based on linux/arch/sh/mm/fault.c:
 *   Copyright (C) 1999  Niibe Yutaka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/kdebug.h>
#include <linux/kprobes.h>

