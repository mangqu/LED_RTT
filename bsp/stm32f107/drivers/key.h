#ifndef __KEY_H__
#define __KEY_H__

#include <rtthread.h>

struct rt_device_key
{
	struct rt_device parent;

	rt_timer_t poll_timer;
	const struct rt_key_ops* ops;
};

struct rt_key_ops
{
	unsigned char (*key_scan)();
};

void rt_hw_key_init(void);

#endif
