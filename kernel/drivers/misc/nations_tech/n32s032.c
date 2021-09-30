#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/gpio.h>
#include <asm/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/wakelock.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "n32s032.h"

#define DEBUG_INFO              0

#define N32S032_I2C_NAME        "n32s032"
#define N32S032_I2C_ADDR        0x50

struct n32s032 {
    struct i2c_client *client;
};
struct n32s032 *g_n32s032 = NULL;

static void Delay_MS(uint32_t ms)
{
    mdelay(ms);
}

static int32_t IIC_Write_Callback(void *pvInBuffer, uint32_t ui32NumBytes)
{
    int ret;
    struct i2c_msg msg;
    struct i2c_client *client;
    int i;
    
    if (!g_n32s032) {
        return -ENOMEM;
    }
    client = g_n32s032->client; 

    msg.flags = !I2C_M_RD;  // wirte
    msg.addr  = client->addr;
    msg.len   = ui32NumBytes;
    msg.buf   = pvInBuffer;
    ret = i2c_transfer(client->adapter, &msg, 1);                                                                                                                                                                                        
    if (ret < 0) {
        printk("%s:i2c_transfer fail, ret=%d\n", __FUNCTION__,ret);
        return BPOI_CMD_EXEC_ERR;
    } 
#if DEBUG_INFO 
    printk("%s: ", __FUNCTION__);
    for (i = 0; i < ui32NumBytes; i++) {
        printk("%02x ", ((char*)pvInBuffer)[i]);
    }
    printk("\n");
#endif

	return BPOI_ERR_NONE;
}

static int32_t IIC_Read_Callback(void *pvOutBuffer, uint32_t ui32NumBytes)
{
    int ret;
    struct i2c_msg msg;
    struct i2c_client *client;
    int i;
    
    if (!g_n32s032) {
        return -ENOMEM;
    }
    client = g_n32s032->client;
    
    msg.flags = I2C_M_RD; // read
    msg.addr  = client->addr;
    msg.len   = ui32NumBytes;
    msg.buf   = pvOutBuffer;
    ret = i2c_transfer(client->adapter, &msg, 1);                                                                                                                                                                                        
    if (ret < 0) {
        printk("%s:i2c_transfer fail, ret=%d\n", __FUNCTION__,ret);
        return BPOI_CMD_EXEC_ERR;
    }
#if DEBUG_INFO
    printk("%s : ", __FUNCTION__);
    for (i = 0; i < ui32NumBytes; i++) {
        printk("%02x ", ((char*)pvOutBuffer)[i]);
    }
    printk("\n");
#endif

	return BPOI_ERR_NONE;
}

static int n32s032_open(struct inode *inode, struct file *file)                                                                                                                                                                             
{
    return 0;
}

static ssize_t n32s032_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
    ssize_t ret = -1;
    char *buf;

    buf = kmalloc(count, GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }
    
    if (BPOI_ERR_NONE == IIC_Read_Callback(buf, count)) {
        ret = copy_to_user(buff, buf, count);                                                                                                                                                                                     
        if (ret) {
            ret = count - ret;
            goto free_buf;
        }
        ret = count;
    }
    
free_buf:
    kfree(buf);

    return ret;
}

static ssize_t n32s032_write(struct file *filp, const char __user *buff, size_t count, loff_t *off)
{
    ssize_t ret = -1;
    char *buf;

    buf = kmalloc(count, GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }
    
    ret = copy_from_user(buf, buff, count);                                                                                                                                                                                         
    if (ret) {
        ret = count - ret;
        goto free_buf;
    }

    if (BPOI_ERR_NONE == IIC_Write_Callback(buf, count)) {
        ret = count;
    }
    
free_buf:
    kfree(buf);

    return ret;
}

static int n32s032_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations n32s032_fops = {
    .owner = THIS_MODULE,
    .open = n32s032_open,
    .read = n32s032_read,
    .write = n32s032_write,
    .release = n32s032_release,                                                                                                                                                                                                   
};

static struct miscdevice n32s032_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "nations",
    .fops = &n32s032_fops,
};

static const struct of_device_id n32s032_ids[] = {
    { .compatible = "nations,n32s032", },
    {},
};

static int n32s032_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    const struct of_device_id *match;
    int ret;

    printk("Enter %s\n", __FUNCTION__);
    match = of_match_device(of_match_ptr(n32s032_ids), &client->dev);
    if (!match)
        return -EINVAL;
        
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "gsl I2C functionality not supported\n");
        return -ENODEV;
    }

    g_n32s032 = devm_kzalloc(&client->dev, sizeof(*g_n32s032), GFP_KERNEL);
    if (!g_n32s032)
        return -ENOMEM;
        
    g_n32s032->client = client;
    i2c_set_clientdata(client, g_n32s032);
    
    ret = misc_register(&n32s032_device);
    if (ret < 0) {
        printk("n32s032_device misc register failed\n");
        goto exit_misc_device_register_failed;
    }
    
    printk("%s End\n", __FUNCTION__);
    return 0;
    
exit_misc_device_register_failed:
    misc_deregister(&n32s032_device);
    
    return -1;
}

static int n32s032_remove(struct i2c_client *client)
{
    printk("%s\n", __FUNCTION__);
    misc_deregister(&n32s032_device);
    return 0;
}

static const struct i2c_device_id n32s032_id[] = {
    {N32S032_I2C_NAME, 0},
    {},
};

MODULE_DEVICE_TABLE(i2c, n32s032_id);

static struct i2c_driver n32s032_driver = {
    .driver = {
               .name = N32S032_I2C_NAME,
               .owner = THIS_MODULE,
               .of_match_table = of_match_ptr(n32s032_ids),
               },

    .probe = n32s032_probe,
    .remove = n32s032_remove,
    .id_table = n32s032_id,
};

static int __init n32s032_init(void)
{       
    int ret;
    ret = i2c_add_driver(&n32s032_driver);

    return ret;
}

static void __exit n32s032_exit(void)
{
    i2c_del_driver(&n32s032_driver);

    return;
}

late_initcall(n32s032_init);
module_exit(n32s032_exit);

MODULE_AUTHOR("zhongw");
MODULE_DESCRIPTION("firefly");
MODULE_LICENSE("GPL");