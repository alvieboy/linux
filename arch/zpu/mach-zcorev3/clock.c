#include <linux/clk.h>
#include <linux/export.h>


/* This stuff should be set up using bootloader */

struct clk *clk_get(struct device *dev, const char *id)
{
    return NULL;
}
EXPORT_SYMBOL(clk_get);

unsigned long clk_get_rate(struct clk *clk)
{
	return 96000000;
}
EXPORT_SYMBOL(clk_get_rate);

struct clk *clk_get_parent(struct clk *clk)
{
    return NULL;
}

EXPORT_SYMBOL(clk_get_parent);

int clk_enable(struct clk *clk)
{
    return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);
