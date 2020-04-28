#include <linux/linkage.h>
#include <linux/kernel.h>


asmlinkage int my_print_msg(char *string){
	printk("%s\n", string);
	return 0;
}
