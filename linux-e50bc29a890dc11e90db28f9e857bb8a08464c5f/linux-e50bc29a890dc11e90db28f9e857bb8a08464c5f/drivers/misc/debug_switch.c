#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/module.h>

#define log_attr(_name) \
static struct kobj_attribute _name##_attr = {   \
        .attr   = {                             \
                .name = __stringify(_name),     \
                .mode = 0644,                   \
        },                                      \
        .show   = _name##_show,                 \
        .store  = _name##_store,                \
}


int log_switch = 0;
EXPORT_SYMBOL_GPL(log_switch);

static ssize_t switch_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
     buf[0] = '0' + log_switch;
     buf[1] = '\n';

     return 2;
}

static ssize_t switch_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
    if (!strncmp(buf, "1\n", 2))
        log_switch = 1;
    else if (!strncmp(buf, "0\n", 2))
        log_switch = 0; 
    else
        return -EINVAL;

    return n;
}

log_attr(switch);

struct kobject *log_kobj;

static struct attribute * g[] = {
    &switch_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
        .attrs = g,
};

static int __init log_switch_init(void)
{
    log_kobj = kobject_create_and_add("log", NULL);
    if (!log_kobj)
        return -ENOMEM;
    return sysfs_create_group(log_kobj, &attr_group);
}

module_init(log_switch_init);
