#include <linux/linkage.h>
#include <asm/page.h>

	.text
	.global clear_page

clear_page:
	breakpoint
	poppc
