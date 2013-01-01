#include <linux/linkage.h>
#include <asm/page.h>

	.text
	.global copy_page
copy_page:
	breakpoint
	poppc
