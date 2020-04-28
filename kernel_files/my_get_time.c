#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>

asmlinkage int my_get_time(unsigned long *sec, unsigned long *nsec){
	struct timespec t;
	getnstimeofday(&t);
	*sec = t.tv_sec;
	*nsec = t.tv_nsec;
	return 0;
}
