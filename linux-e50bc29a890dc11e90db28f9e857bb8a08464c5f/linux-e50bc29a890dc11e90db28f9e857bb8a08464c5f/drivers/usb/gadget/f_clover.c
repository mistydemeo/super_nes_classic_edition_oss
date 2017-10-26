/*
 * Gadget Function Driver for Clover USB presence
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2016 Nintendo Co. Ltd
 *
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* For CLOVER we need to publish a USB interface that declares itself as
 * a Self Powered device and using a max of 500mA. That way we should
 * be nicer to USB real hosts, requesting those necessary 500mA explicitely.
 *
 * The linux 3.4 from Allwinner kind of... mandates the Android gadget.
 *
 * So here we are, stripping down the accessory module from Android. Keeping
 * just enough for making the Gadget API happy and publish the gadget.
 *
 * With mainline kernels things would have been different. But that's another 
 * story yet to written.
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/usb.h>
#include <linux/usb/ch9.h>

#define CLOVER_BULK_BUFFER_SIZE    16384
#define CLOVER_STRING_SIZE     256

#define CLOVER_PRESENCE_START   0x51
#define CLOVER_GET_PRESENCE     0xc0
#define CLOVER_PRESENCE_VERSION 0x43

/* String IDs */
#define CLOVER_INTERFACE_STRING_INDEX	0

/* number of tx and rx requests to allocate */
#define TX_REQ_MAX 4
#define RX_REQ_MAX 2

struct clover_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

	/* set to 1 when we connect */
	 unsigned int online:1;
	/* Set to 1 when we disconnect.
	 * Not cleared until our file is closed.
	 */
	 unsigned int disconnected:1;

	/* set to 1 if we have a pending start request */
	int start_requested;

	/* synchronize access to our device file */
	atomic_t open_excl;

	struct list_head tx_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	struct usb_request *rx_req[RX_REQ_MAX];
	int rx_done;

	/* delayed work for handling ACCESSORY_START */
	struct delayed_work start_work;
};

static struct usb_interface_descriptor clover_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass     = USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol     = 0,
};

static struct usb_endpoint_descriptor clover_superspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};
static struct usb_ss_ep_comp_descriptor clover_superspeed_in_comp_desc = {
	.bLength =		sizeof(clover_superspeed_in_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	/*.bMaxBurst =		DYNAMIC, */
};

static struct usb_endpoint_descriptor clover_superspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor clover_superspeed_out_comp_desc = {
	.bLength =		sizeof(clover_superspeed_out_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	/*.bMaxBurst =		DYNAMIC, */
};

static struct usb_endpoint_descriptor clover_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor clover_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor clover_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor clover_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_clover_descs[] = {
	(struct usb_descriptor_header *) &clover_interface_desc,
	(struct usb_descriptor_header *) &clover_fullspeed_in_desc,
	(struct usb_descriptor_header *) &clover_fullspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *hs_clover_descs[] = {
	(struct usb_descriptor_header *) &clover_interface_desc,
	(struct usb_descriptor_header *) &clover_highspeed_in_desc,
	(struct usb_descriptor_header *) &clover_highspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *ss_clover_descs[] = {
	(struct usb_descriptor_header *) &clover_interface_desc,
	(struct usb_descriptor_header *) &clover_superspeed_in_desc,
	(struct usb_descriptor_header *) &clover_superspeed_in_comp_desc,
	(struct usb_descriptor_header *) &clover_superspeed_out_desc,
	(struct usb_descriptor_header *) &clover_superspeed_out_comp_desc,
	NULL,
};

static const char* clover_model_interface[] =
{
	"CLV-S-HVCY",
	"CLV-S-NESY",
	"CLV-S-SHVY",
	"CLV-S-SNSY",
	"CLV-S-SNPY",
};

static struct usb_string clover_string_defs[] = {
	[CLOVER_INTERFACE_STRING_INDEX].s = NULL, /* replaced at runtime*/
	{  },	/* end of list */
};

static struct usb_gadget_strings clover_string_table = {
	.language		= 0x0409,	/* en-US */
	.strings		= clover_string_defs,
};

static struct usb_gadget_strings *clover_strings[] = {
	&clover_string_table,
	NULL,
};

/* temporary variable used between clover_open() and clover_gadget_bind() */
static struct clover_dev *_clover_dev;

static inline struct clover_dev *func_to_clover_dev(struct usb_function *f)
{
	return container_of(f, struct clover_dev, function);
}

static struct usb_request *clover_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void clover_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

/* add a request to the tail of a list */
static void clover_req_put(struct clover_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request *clover_req_get(struct clover_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void clover_set_disconnected(struct clover_dev *dev)
{
	dev->online = 0;
	dev->disconnected = 1;
}

static void clover_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct clover_dev *dev = _clover_dev;

	if (req->status != 0)
		clover_set_disconnected(dev);

	clover_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void clover_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct clover_dev *dev = _clover_dev;

	dev->rx_done = 1;
	if (req->status != 0)
		clover_set_disconnected(dev);

	wake_up(&dev->read_wq);
}

static void clover_complete_set_string(struct usb_ep *ep, struct usb_request *req)
{
	struct clover_dev	*dev = ep->driver_data;
	char *string_dest = NULL;
	int length = req->actual;

	if (req->status != 0) {
		pr_err("clover_complete_set_string, err %d\n", req->status);
		return;
	}

	/* Noop */
}

static int clover_create_bulk_endpoints(struct clover_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	DBG(cdev, "clover_create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

#ifndef CONFIG_USB_SUNXI_USB
	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;
#endif

	/* now allocate requests for our endpoints */
	for (i = 0; i < TX_REQ_MAX; i++) {
		req = clover_request_new(dev->ep_in, CLOVER_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = clover_complete_in;
		clover_req_put(dev, &dev->tx_idle, req);
	}
	for (i = 0; i < RX_REQ_MAX; i++) {
		req = clover_request_new(dev->ep_out, CLOVER_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = clover_complete_out;
		dev->rx_req[i] = req;
	}

	return 0;

fail:
	pr_err("clover_bind() could not allocate requests\n");
	while ((req = clover_req_get(dev, &dev->tx_idle)))
		clover_request_free(req, dev->ep_in);
	for (i = 0; i < RX_REQ_MAX; i++)
		clover_request_free(dev->rx_req[i], dev->ep_out);
	return -1;
}

static ssize_t clover_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	struct clover_dev *dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret = 0;

	pr_debug("clover_read(%d)\n", count);

	if (dev->disconnected)
		return -ENODEV;

	if (count > CLOVER_BULK_BUFFER_SIZE)
		count = CLOVER_BULK_BUFFER_SIZE;

	/* we will block until we're online */
	pr_debug("clover_read: waiting for online\n");
	ret = wait_event_interruptible(dev->read_wq, dev->online);
	if (ret < 0) {
		r = ret;
		goto done;
	}

requeue_req:
	/* queue a request */
	req = dev->rx_req[0];
#ifndef CONFIG_ARCH_SUN9I
	req->length = count;
#else
	req->length = count + dev->ep_out->maxpacket - 1;
	req->length -= req->length % dev->ep_out->maxpacket;
#endif
	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_KERNEL);
	if (ret < 0) {
		r = -EIO;
		goto done;
	} else {
		pr_debug("rx %p queue\n", req);
	}

	/* wait for a request to complete */
	ret = wait_event_interruptible(dev->read_wq, dev->rx_done);
	if (ret < 0) {
		r = ret;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}
	if (dev->online) {
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

		pr_debug("rx %p %d\n", req, req->actual);
		xfer = (req->actual < count) ? req->actual : count;
		r = xfer;
		if (copy_to_user(buf, req->buf, xfer))
			r = -EFAULT;
	} else
		r = -EIO;

done:
	pr_debug("clover_read returning %d\n", r);
	return r;
}

static ssize_t clover_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct clover_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	pr_debug("clover_write(%d)\n", count);

	if (!dev->online || dev->disconnected)
		return -ENODEV;

	while (count > 0) {
		if (!dev->online) {
			pr_debug("clover_write dev->error\n");
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			((req = clover_req_get(dev, &dev->tx_idle)) || !dev->online));
		if (!req) {
			r = ret;
			break;
		}

		if (count > CLOVER_BULK_BUFFER_SIZE)
			xfer = CLOVER_BULK_BUFFER_SIZE;
		else
			xfer = count;
		if (copy_from_user(req->buf, buf, xfer)) {
			r = -EFAULT;
			break;
		}

		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
			pr_debug("clover_write: xfer error %d\n", ret);
			r = -EIO;
			break;
		}

		buf += xfer;
		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

	if (req)
		clover_req_put(dev, &dev->tx_idle, req);

	pr_debug("clover_write returning %d\n", r);
	return r;
}

static long clover_ioctl(struct file *fp, unsigned code, unsigned long value)
{
	struct clover_dev *dev = fp->private_data;
	char *src = NULL;
	int ret;

	switch (code) {
	case CLOVER_GET_PRESENCE:
		ret = CLOVER_PRESENCE_VERSION;
		break;
        default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int clover_open(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "clover_open\n");
	if (atomic_xchg(&_clover_dev->open_excl, 1))
		return -EBUSY;

	_clover_dev->disconnected = 0;
	fp->private_data = _clover_dev;
	return 0;
}

static int clover_release(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "clover_release\n");

	WARN_ON(!atomic_xchg(&_clover_dev->open_excl, 0));
	_clover_dev->disconnected = 0;
	return 0;
}

/* file operations for /dev/usb_clover */
static const struct file_operations clover_fops = {
	.owner = THIS_MODULE,
	.read = clover_read,
	.write = clover_write,
	.unlocked_ioctl = clover_ioctl,
	.open = clover_open,
	.release = clover_release,
};

static struct miscdevice clover_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "usb_clover",
	.fops = &clover_fops,
};

static int clover_ctrlrequest(struct usb_composite_dev *cdev,
				const struct usb_ctrlrequest *ctrl)
{
	struct clover_dev	*dev = _clover_dev;
	int	value = -EOPNOTSUPP;
	int offset;
	u8 b_requestType = ctrl->bRequestType;
	u8 b_request = ctrl->bRequest;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	unsigned long flags;

/*
	printk(KERN_INFO "clover_ctrlrequest "
			"%02x.%02x v%04x i%04x l%u\n",
			b_requestType, b_request,
			w_value, w_index, w_length);
*/

	if (b_requestType == (USB_DIR_OUT | USB_TYPE_VENDOR)) {
		if (b_request == CLOVER_PRESENCE_START) {
			dev->start_requested = 1;
			schedule_delayed_work(
				&dev->start_work, msecs_to_jiffies(10));
			value = 0;
		}
	} else if (b_requestType == (USB_DIR_IN | USB_TYPE_VENDOR)) {
		if (b_request == CLOVER_GET_PRESENCE) {
			*((u16 *)cdev->req->buf) = CLOVER_PRESENCE_VERSION;
			value = sizeof(u16);

			/* clear any string left over from a previous session */
			dev->start_requested = 0;
		}
	}

	if (value >= 0) {
		cdev->req->zero = 0;
		cdev->req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "%s setup response queue error\n",
				__func__);
	}

err:
	if (value == -EOPNOTSUPP)
		VDBG(cdev,
			"unknown class-specific control req "
			"%02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	return value;
}

static int
clover_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct clover_dev	*dev = func_to_clover_dev(f);
	int			id;
	int			ret;

	DBG(cdev, "clover_function_bind dev: %p\n", dev);

	dev->start_requested = 0;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	clover_interface_desc.bInterfaceNumber = id;

	/* allocate endpoints */
	ret = clover_create_bulk_endpoints(dev, &clover_fullspeed_in_desc,
			&clover_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		clover_highspeed_in_desc.bEndpointAddress =
			clover_fullspeed_in_desc.bEndpointAddress;
		clover_highspeed_out_desc.bEndpointAddress =
			clover_fullspeed_out_desc.bEndpointAddress;
	}

	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
		clover_superspeed_in_desc.bEndpointAddress =
			clover_fullspeed_in_desc.bEndpointAddress;
		clover_superspeed_out_desc.bEndpointAddress =
			clover_fullspeed_out_desc.bEndpointAddress;
	}

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
clover_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct clover_dev	*dev = func_to_clover_dev(f);
	struct usb_request *req;
	int i;

	while ((req = clover_req_get(dev, &dev->tx_idle)))
		clover_request_free(req, dev->ep_in);
	for (i = 0; i < RX_REQ_MAX; i++)
		clover_request_free(dev->rx_req[i], dev->ep_out);
}

static void clover_start_work(struct work_struct *data)
{
	char *envp[2] = { "ACCESSORY=START", NULL };
	kobject_uevent_env(&clover_device.this_device->kobj, KOBJ_CHANGE, envp);
}

static int clover_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct clover_dev	*dev = func_to_clover_dev(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	DBG(cdev, "clover_function_set_alt intf: %d alt: %d\n", intf, alt);

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_in);
	if (ret)
		return ret;

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}

	dev->online = 1;

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	return 0;
}

static void clover_function_disable(struct usb_function *f)
{
	struct clover_dev	*dev = func_to_clover_dev(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	DBG(cdev, "clover_function_disable\n");
	clover_set_disconnected(dev);
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	VDBG(cdev, "%s disabled\n", dev->function.name);
}

static int clover_bind_config(struct usb_configuration *c, int model)
{
	struct clover_dev *dev = _clover_dev;
	int ret;

	printk(KERN_INFO "clover_bind_config\n");

	if (model < 0 || model >= sizeof(clover_model_interface) / sizeof(clover_model_interface[0]))
		model = 0;
	clover_string_defs[CLOVER_INTERFACE_STRING_INDEX].s = clover_model_interface[model];

	/* allocate a string ID for our interface */
	if (clover_string_defs[CLOVER_INTERFACE_STRING_INDEX].id == 0) {
		ret = usb_string_id(c->cdev);
		if (ret < 0)
			return ret;
		clover_string_defs[CLOVER_INTERFACE_STRING_INDEX].id = ret;
		clover_interface_desc.iInterface = ret;
	}

	dev->cdev = c->cdev;
	dev->function.name = "clover";
	dev->function.strings = clover_strings,
	dev->function.descriptors = fs_clover_descs;
	dev->function.hs_descriptors = hs_clover_descs;
	dev->function.ss_descriptors = ss_clover_descs;
	dev->function.bind = clover_function_bind;
	dev->function.unbind = clover_function_unbind;
	dev->function.set_alt = clover_function_set_alt;
	dev->function.disable = clover_function_disable;

	return usb_add_function(c, &dev->function);
}

static int clover_setup(void)
{
	struct clover_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);
	atomic_set(&dev->open_excl, 0);
	INIT_LIST_HEAD(&dev->tx_idle);
	INIT_DELAYED_WORK(&dev->start_work, clover_start_work);

	/* _clover_dev must be set before calling usb_gadget_register_driver */
	_clover_dev = dev;

	ret = misc_register(&clover_device);
	if (ret)
		goto err;

	return 0;

err:
	kfree(dev);
	pr_err("USB clover gadget driver failed to initialize\n");
	return ret;
}

static void clover_disconnect(void)
{
}

static void clover_cleanup(void)
{
	misc_deregister(&clover_device);
	kfree(_clover_dev);
	_clover_dev = NULL;
}
