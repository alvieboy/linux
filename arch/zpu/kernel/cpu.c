/*
 */
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/param.h>
#include <linux/errno.h>

static DEFINE_PER_CPU(struct cpu, cpu_devices);

static int __init topology_init(void)
{
	int cpu;
	for_each_possible_cpu(cpu) {
		struct cpu *c = &per_cpu(cpu_devices, cpu);
		register_cpu(c, cpu);
	}
	return 0;
}

subsys_initcall(topology_init);

void __init setup_processor(void)
{
}

#ifdef CONFIG_PROC_FS
static int c_show(struct seq_file *m, void *v)
{
    return -EIO;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void c_stop(struct seq_file *m, void *v)
{

}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show
};
#endif /* CONFIG_PROC_FS */
