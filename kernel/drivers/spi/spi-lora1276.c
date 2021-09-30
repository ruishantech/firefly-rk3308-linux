#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/console.h>
#include <linux/serial_core.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/freezer.h>
#include <linux/spi/spi.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/serial.h>
#include <linux/io.h>
#include <linux/miscdevice.h>


//SX1276 register map
#define LR_RegFifo                       0x00
#define LR_RegOpMode                     0x01
#define LR_RegBitrateMsb                 0x02
#define LR_RegBitrateLsb                 0x03
#define LR_RegFdevMsb                    0x04
#define LR_RegFdMsb                      0x05
#define LR_RegFrMsb                      0x06
#define LR_RegFrMid                      0x07
#define LR_RegFrLsb                      0x08
#define LR_RegPaConfig                   0x09
#define LR_RegPaRamp                     0x0A
#define LR_RegOcp                        0x0B
#define LR_RegLna                        0x0C
#define LR_RegFifoAddrPtr                0x0D
#define LR_RegFifoTxBaseAddr             0x0E
#define LR_RegFifoRxBaseAddr             0x0F
#define LR_RegFifoRxCurrentaddr          0x10
#define LR_RegIrqFlagsMask               0x11
#define LR_RegIrqFlags                   0x12
#define LR_RegRxNbBytes                  0x13
#define LR_RegRxHeaderCntValueMsb        0x14
#define LR_RegRxHeaderCntValueLsb        0x15
#define LR_RegRxPacketCntValueMsb        0x16
#define LR_RegRxPacketCntValueLsb        0x17
#define LR_RegModemStat                  0x18
#define LR_RegPktSnrValue                0x19
#define LR_RegPktRssiValue               0x1A
#define LR_RegRssiValue                  0x1B
#define LR_RegHopChannel                 0x1C
#define LR_RegModemConfig1               0x1D
#define LR_RegModemConfig2               0x1E
#define LR_RegSymbTimeoutLsb             0x1F
#define LR_RegPreambleMsb                0x20
#define LR_RegPreambleLsb                0x21
#define LR_RegPayloadLength              0x22
#define LR_RegMaxPayloadLength           0x23
#define LR_RegHopPeriod                  0x24
#define LR_RegFifoRxByteAddr             0x25
#define LR_RegModemConfig3               0x26
#define REG_LR_DIOMAPPING1               0x40
#define REG_LR_DIOMAPPING2               0x41
#define REG_LR_VERSION                   0x42
#define REG_LR_PLLHOP                    0x44
#define REG_LR_TCXO                      0x4B
#define REG_LR_PADAC                     0x4D
#define REG_LR_FORMERTEMP                0x5B
#define REG_LR_AGCREF                    0x61
#define REG_LR_AGCTHRESH1                0x62
#define REG_LR_AGCTHRESH2                0x63
#define REG_LR_AGCTHRESH3                0x64

#define TX_DONE_MASK 0x41
#define RX_DONE_MASK 0x01

#define DEVICE_NAME "lora1276c1"

struct lora_device
{
    /*for linux framework*/
    struct spi_device *spi_lora;
    struct workqueue_struct *workqueue;
    struct work_struct work;
    struct mutex lora_lock_reg;
    struct mutex lora_lock_dev;

    int	pwr_gpio;
    int	rst_gpio;
    int sel_gpio;
    int	cs_gpio;
    int	irq_gpio;
    int irq;

    /*for Lora*/
    int tx_done_flag;
    int rx_done_flag;
    int cad_done_flag;
    int dio0_irq_flag;
};

static struct lora_device lora1276_device;

void lora1276_cs_gpio(int on)
{
    gpio_set_value(lora1276_device.cs_gpio, !!on );
    //printk("lora cs gpio is set to %d\n",!!on);
}

static void send_uevent(struct device *dev ,char * strings)
{
    int ret;
    char event_string[128];
    char *envp[] = { event_string, NULL };
    sprintf(event_string, "LORA1276=%s", strings);
    ret = kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
    if(ret)
        printk("uevent env failed \n");
}

static int lora1276_read_reg(struct spi_device *spi, uint8_t reg, uint8_t *dat)
{
    struct spi_message msg;
    uint8_t buf_wdat[2];
    uint8_t buf_rdat[2];
    int status = 0;
    struct spi_transfer index_xfer = {
        .len            = 2,
        .cs_change      = 1,
    };

    mutex_lock(&lora1276_device.lora_lock_reg);
    lora1276_cs_gpio(0);
    spi_message_init(&msg);
    buf_wdat[0] = reg;
    buf_wdat[1] = 0x00;
    buf_rdat[0] = 0x00;
    buf_rdat[1] = 0x00;
    index_xfer.tx_buf = buf_wdat;
    index_xfer.rx_buf =(void *) buf_rdat;
    spi_message_add_tail(&index_xfer, &msg);
    status = spi_sync(spi, &msg);
    udelay(3);
    lora1276_cs_gpio(1);
    mutex_unlock(&lora1276_device.lora_lock_reg);

    if (status)
    {
        printk("read spi reg failed\n");
        return status;
    }
    *dat = buf_rdat[1];

    return 0;
}

static int lora1276_write_reg(struct spi_device *spi, uint8_t reg, uint8_t dat)
{
    struct spi_message msg;
    uint8_t buf_reg[2];
    int status;
    struct spi_transfer index_xfer = {
        .len            = 2,
        .cs_change      = 1,
    };

    mutex_lock(&lora1276_device.lora_lock_reg);
    lora1276_cs_gpio(0);
    spi_message_init(&msg);
    /* register index */
    buf_reg[0] = reg | 0x80;
    buf_reg[1] = dat;
    index_xfer.tx_buf = buf_reg;
    spi_message_add_tail(&index_xfer, &msg);
    status = spi_sync(spi, &msg);
    udelay(3);
    lora1276_cs_gpio(1);
    mutex_unlock(&lora1276_device.lora_lock_reg);

    if(status){
        printk("write spi reg failed\n");
    }
    return status;
}

#define MAX_RFCOUNT_SIZE 256
static int lora_burst_read_fifo(struct spi_device *spi,uint8_t reg, uint8_t fifolen, uint8_t *dat)
{
    struct spi_message msg;
    int status,i;
    uint8_t recive_fifo_data[MAX_RFCOUNT_SIZE+1]={0};
    uint8_t transmit_fifo_data[MAX_RFCOUNT_SIZE+1]={0};
    struct spi_transfer index_xfer = {
        .len            = fifolen+1,
        .cs_change      = 1,
    };
    if(!(fifolen>0))
    {
        printk(KERN_ERR "%s,burst read error,fifolen:%d!!\n", __func__,fifolen);
        return 1;
    }
    mutex_lock(&lora1276_device.lora_lock_reg);
    lora1276_cs_gpio(0);
    spi_message_init(&msg);
    /* register index */
    transmit_fifo_data[0] = reg;
    index_xfer.tx_buf = transmit_fifo_data;
    index_xfer.rx_buf =(void *) recive_fifo_data;
    spi_message_add_tail(&index_xfer, &msg);

    status = spi_sync(spi, &msg);
    udelay(3);

    for(i=0;i<fifolen;i++)
    {
        *(dat+i)=recive_fifo_data[i+1];
    }
    //printk("read fifo %d\n",fifolen);
    lora1276_cs_gpio(1);
    mutex_unlock(&lora1276_device.lora_lock_reg);
    return status;
}

static int lora_burst_write_fifo(struct spi_device *spi, uint8_t reg, uint8_t fifolen, uint8_t *dat)
{
    struct spi_message msg;
    int status,i;
    uint8_t recive_fifo_data[MAX_RFCOUNT_SIZE+1]={0};
    uint8_t transmit_fifo_data[MAX_RFCOUNT_SIZE+1]={0};
    struct spi_transfer index_xfer = {
        .len            = fifolen+1,
        .cs_change      = 1,
    };
    if(!(fifolen>0))
    {
        printk(KERN_ERR "%s,burst write error,fifolen:%d!!\n", __func__,fifolen);
        return 1;
    }
    mutex_lock(&lora1276_device.lora_lock_reg);
    lora1276_cs_gpio(0);
    spi_message_init(&msg);
    /* register index */
    transmit_fifo_data[0] = reg | 0x80;

    for(i=0;i<fifolen;i++)
    {
        transmit_fifo_data[i+1]=*(dat+i);
    }
    //printk("burst write fifo %d\n",fifolen);
    index_xfer.tx_buf = transmit_fifo_data;
    index_xfer.rx_buf =(void *) recive_fifo_data;
    spi_message_add_tail(&index_xfer, &msg);

    status = spi_sync(spi, &msg);
    udelay(3);
    lora1276_cs_gpio(1);
    mutex_unlock(&lora1276_device.lora_lock_reg);

    return status;
}


uint8_t g_rbuf[256]={0};
static void lora127x_work(struct work_struct *w)
{
    uint8_t  temp,packet_size;
    lora1276_read_reg(lora1276_device.spi_lora, LR_RegIrqFlags, &temp); //make sure what interrupt type is happening 
    if(temp & 0x08){
        lora1276_device.tx_done_flag = 1;
        lora1276_write_reg(lora1276_device.spi_lora, LR_RegIrqFlags, 0xff);			// clear lora irq
        lora1276_write_reg(lora1276_device.spi_lora,LR_RegOpMode,0x09);         // enter standby
        send_uevent(&lora1276_device.spi_lora->dev,"txdone");
    }else if(temp & 0x40){
        lora1276_device.rx_done_flag = 1;
        lora1276_write_reg(lora1276_device.spi_lora, LR_RegIrqFlags, 0xff);			// clear lora irq
        lora1276_read_reg(lora1276_device.spi_lora, LR_RegFifoRxCurrentaddr, &temp);    // read current receive cache address 
        lora1276_write_reg(lora1276_device.spi_lora, LR_RegFifoAddrPtr,temp);           // set the received data from the current receive cache address 
        lora1276_read_reg(lora1276_device.spi_lora,LR_RegRxNbBytes, &packet_size);     // read received data
        lora_burst_read_fifo(lora1276_device.spi_lora,0x00, packet_size, g_rbuf);
        lora1276_write_reg(lora1276_device.spi_lora,LR_RegOpMode,0x09);         // enter standby
        send_uevent(&lora1276_device.spi_lora->dev ,"rxdone");
        //printk("%s: rec %s\n",__func__,g_rbuf);

    }else if(temp & 0x04){
        lora1276_device.cad_done_flag = 1;
        lora1276_write_reg(lora1276_device.spi_lora, LR_RegIrqFlags, 0xff);			// clear lora irq
        send_uevent(&lora1276_device.spi_lora->dev ,"caddone");
        lora1276_write_reg(lora1276_device.spi_lora,LR_RegOpMode,0x09);  
    }else{
        send_uevent(&lora1276_device.spi_lora->dev ,"err");
        //printk("irq workqueue err\n");
    }
    lora1276_device.dio0_irq_flag =0;
    enable_irq(lora1276_device.irq);
}

static irqreturn_t lora127x_irq(int irq, void *dev_id)
{
    struct lora_device* d = dev_id;
    disable_irq_nosync(d->irq);
    d->dio0_irq_flag = 1;
    queue_work(d->workqueue, &d->work);

    return IRQ_HANDLED;
}

static int lora1276_open(struct inode *inode, struct file *filp)
{
    //printk("lora1276c1 open\n");
    return 0;
}


#define MAX_BUFF_CDEV 256
static ssize_t lora1276_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    char rxbuf[MAX_BUFF_CDEV] = { 0 };
    int ret;
    uint8_t reg;

    if(size > MAX_BUFF_CDEV||size<1){
        printk("%s, buffer size error, size len: %d!!\n", __func__,(int)size);
        return -1;
    }

    mutex_lock(&lora1276_device.lora_lock_dev);

    if(copy_from_user(rxbuf, buf , 1)){
        printk("%s: read reg from user failed\n",__func__);
        mutex_unlock(&lora1276_device.lora_lock_dev);
        return -EFAULT;
    }else{
        ret = size;
    }
    reg = rxbuf[0];

    if(reg > 0x64 ||reg < 0x00){
        printk("%s: read reg out of range\n",__func__);
        mutex_unlock(&lora1276_device.lora_lock_dev);
        return -EFAULT;
    }

    if(size == 1){
        ret=lora1276_read_reg(lora1276_device.spi_lora,reg, &rxbuf[0]);
        if (copy_to_user(buf, &rxbuf[0],1)){
            printk("%s: copy to user failed\n",__func__);
            ret = -EFAULT;
        }
    }else{
        //ret=lora_burst_read_fifo(lora1276_device.spi_lora,reg, size, rxbuf);
        if (copy_to_user(buf, &g_rbuf,size)){
            printk("%s: copy to user failed\n",__func__);
            ret = -EFAULT;
        }
        lora1276_device.rx_done_flag = 0;
        //printk("%s: kernel get %s\n",__func__,g_rbuf);
    }
    mutex_unlock(&lora1276_device.lora_lock_dev);

    return ret;
}

static ssize_t lora1276_write(struct file *filp, const char __user * buf, size_t size, loff_t * ppos)
{
    char txbuf[MAX_BUFF_CDEV];
    int ret;
    if(size > MAX_BUFF_CDEV||size < 2){
        printk("%s, buffer size error, size len: %d!!\n", __func__,(int)size);
        return -1;
    }

    if(buf[0]>0x64||buf[0]<0x00){
        printk("%s: read reg out of rang\n",__func__);
        return -1;
    }

    mutex_lock(&lora1276_device.lora_lock_dev);
    if(copy_from_user(txbuf, buf , size)){
        printk("%s: copy from user failed\n",__func__);
        mutex_unlock(&lora1276_device.lora_lock_dev);
        return -EFAULT;
    }else{
        ret = size;
    }

    if(size == 2){
        ret=lora1276_write_reg(lora1276_device.spi_lora,txbuf[0],txbuf[1]);
    }else{
        ret=lora_burst_write_fifo(lora1276_device.spi_lora, 0x00,size, txbuf);
        lora1276_device.tx_done_flag = 0;
        //printk("%s: kernel get %s\n",__func__,txbuf);
    }
    ret = size;
    if(!ret)
        ret = -EFAULT;
    mutex_unlock(&lora1276_device.lora_lock_dev);
    return ret;
}

static int lora1276_release(struct inode *inode, struct file *filp)
{
    lora1276_write_reg(lora1276_device.spi_lora, LR_RegIrqFlags, 0xff);			// clear lora irq
    lora1276_write_reg(lora1276_device.spi_lora,LR_RegOpMode,0x09);         // enter standby
    return 0;
}

static struct file_operations lora1276_fops = {
    .owner = THIS_MODULE,
    .open = lora1276_open,
    .read = lora1276_read,
    .write = lora1276_write,
    .release = lora1276_release,
};

static struct miscdevice lora1276_dev={
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &lora1276_fops,
};

static int lora1276_probe(struct spi_device *spi)
{
    struct device_node *node = spi->dev.of_node;
    struct device *dev = &spi->dev;
    int ret;
    int en_value;
    enum of_gpio_flags flags;

    memset(&lora1276_device, 0, sizeof(lora1276_device));

    ret = misc_register(&lora1276_dev);
    if(ret < 0){
        printk("lora1276 misc_register failed\n");
        return ret;
    } 

    lora1276_device.cs_gpio = of_get_named_gpio_flags(node, "cs-gpio", 0, &flags);
    if ( gpio_is_valid( lora1276_device.cs_gpio ) ) {
        en_value = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
        ret = devm_gpio_request(dev, lora1276_device.cs_gpio, "lora cs-gpio");
        if(ret < 0){
            printk("%s() failed to request gpio%d for cs-gpio\n", __FUNCTION__, lora1276_device.cs_gpio);
            goto err;
        }
        gpio_direction_output(lora1276_device.cs_gpio, en_value);
    }

    lora1276_device.pwr_gpio = of_get_named_gpio_flags(node, "power-gpio", 0, &flags);
    if ( gpio_is_valid( lora1276_device.pwr_gpio ) ) {
        en_value = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
        ret = devm_gpio_request(dev, lora1276_device.pwr_gpio, "lora power-gpio");
        if(ret < 0){
            printk("%s() failed to request gpio%d for power-gpio\n", __FUNCTION__, lora1276_device.pwr_gpio);
            goto err;
        }
        gpio_direction_output(lora1276_device.pwr_gpio, en_value);
    }

    lora1276_device.rst_gpio = of_get_named_gpio_flags(node, "reset-gpio", 0, &flags);
    if ( gpio_is_valid( lora1276_device.rst_gpio ) ) {
        en_value = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
        ret = devm_gpio_request(dev, lora1276_device.rst_gpio, "lora reset-gpio");
        if(ret < 0){
            printk("%s() failed to request gpio%d for reset-gpio\n", __FUNCTION__, lora1276_device.rst_gpio);
            goto err;
        }
        gpio_direction_output(lora1276_device.rst_gpio, en_value);
    }

    lora1276_device.sel_gpio = of_get_named_gpio_flags(node, "sel-gpio", 0, &flags);
    if( gpio_is_valid( lora1276_device.sel_gpio ) ){
        en_value = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
        ret = devm_gpio_request(dev, lora1276_device.sel_gpio , "lora sel-gpio");
        if(ret < 0){
            printk("%s() failed to request gpio%d for sel-gpio\n", __FUNCTION__, lora1276_device.sel_gpio);
            goto err;
        }
        gpio_direction_output(lora1276_device.sel_gpio, en_value);
    }

    lora1276_device.irq_gpio = of_get_named_gpio_flags(node, "irq-gpio", 0, &flags);
    if ( gpio_is_valid( lora1276_device.irq_gpio ) ) {
        unsigned int irq_flags = IRQF_SHARED | flags;
        ret = devm_gpio_request(dev, lora1276_device.irq_gpio, "lora irq-gpio");
        if(ret < 0){
            printk("%s() failed to request gpio%d for irq-gpio\n", __FUNCTION__, lora1276_device.irq_gpio);
            goto err;
        }
        gpio_direction_input(lora1276_device.irq_gpio);
        lora1276_device.irq = gpio_to_irq( lora1276_device.irq_gpio );
        if(request_irq(lora1276_device.irq, lora127x_irq, irq_flags, "Lora1276_int", &lora1276_device) < 0) {
            goto err;
        }
    }


#if 0
    spi->mode = SPI_MODE_0;
    ret = spi_setup(spi);
    if (ret)
        goto err;

    printk("lora1276_probe: setup mode %d, %s %s %s %s %u bits/w, %u Hz max\r\n", (int) (spi->mode & (SPI_CPOL | SPI_CPHA)),
           (spi->mode & SPI_CS_HIGH) ? "cs_high, " : "cs_low",
           (spi->mode & SPI_LSB_FIRST) ? "lsb, " : "",
           (spi->mode & SPI_3WIRE) ? "3wire, " : "",
           (spi->mode & SPI_LOOP) ? "loopback, " : "",
           spi->bits_per_word, spi->max_speed_hz);
#endif

    lora1276_device.spi_lora = spi;
    mutex_init(&lora1276_device.lora_lock_reg);
    mutex_init(&lora1276_device.lora_lock_dev);
    lora1276_device.workqueue = alloc_workqueue("workqueue_lora1276", 0, 0);
    INIT_WORK(&lora1276_device.work, lora127x_work);

    printk("%s: lora1276 register success\n",__func__);
    return 0;

err:
    printk("lora1276 probe failed\n");
    if(lora1276_device.workqueue){
        destroy_workqueue(lora1276_device.workqueue);
        lora1276_device.workqueue = NULL;
    }
    misc_deregister(&lora1276_dev);
    return ret;
}

static int lora1276_remove(struct spi_device *spi)
{
    if(lora1276_device.dio0_irq_flag == 1){
        printk("%s:workqueue may not ready\n",__func__);
        return -1;
    }
    if(lora1276_device.workqueue){
        destroy_workqueue(lora1276_device.workqueue);
        lora1276_device.workqueue = NULL;
    }
    free_irq(lora1276_device.irq, &lora1276_device);
    misc_deregister(&lora1276_dev);
    return 0;
}

static const struct of_device_id lora1276_of_match[] = {
    {
        .compatible	= "firefly,lora1276",
    },
    { }
};

static struct spi_driver lora1276_driver = {
    .driver = {
        .name           = "lora1276",
        .owner          = THIS_MODULE,
        .of_match_table = lora1276_of_match,
    },
    .probe          = lora1276_probe,
    .remove         = lora1276_remove,
};

static int __init lora1276_init(void)
{
    int ret;
    ret = spi_register_driver(&lora1276_driver);
    printk("lora1276 register spi return v = :%d\n",ret);
    return ret;
}

static void __exit lora1276_exit(void)
{
    spi_unregister_driver(&lora1276_driver);
    printk("lora1276 exit\n ");
}

module_init(lora1276_init);
module_exit(lora1276_exit);

MODULE_AUTHOR("T-chip firefly"); 
MODULE_DESCRIPTION("LoRa1276 C1 driver"); 
MODULE_LICENSE("GPL");
