#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <asm/mach/map.h>
#include <asm/irq.h>
#include <linux/interrupt.h>

#define  GPIO_MAJOR  242
#define  GET_VALUE  0xa00


static struct class *di_class;
static struct device *di_device;
//wait_queue_head_t io_queue;
static DECLARE_WAIT_QUEUE_HEAD(gpio_waitq);

unsigned char retq;
unsigned char irq_flag=0;

static struct gpio_dev
{
	struct cdev cdev;
//	struct completion gpio_done;
}dev;

struct di_gpio_st{
	//unsigned long gpio;
	int gpio;
	char *name;
	int irq;
};
static struct di_gpio_st di_gpio[] = {
	//{SABRESD_GPIO_DO_1,"do1"},
	{0,"user-di1",0},
	{1,"user-di2",1},
/*	{2,"user-di3",2},
	{3,"user-di4",3},
	{4,"user-di5",4},*/
};

static long gpio_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	//int id;
	int i = 0;
	int ret = 0;
//	printk(KERN_INFO"ioctl  DI !\n");
	if ( GET_VALUE != cmd ) 
		return -1;
	
//	wait_for_completion(&dev.gpio_done);
	wait_event_interruptible(gpio_waitq,irq_flag);
//	for(i=0;i<5;i++)
//		disable_irq(gpio_to_irq(retq));
    //mdelay(5);
    //v = gpio_get_value(swmb_gpio[0].gpio);
      //          enable_irq(gpio_to_irq(swmb_gpio[0].gpio));
    //            if ( v == 0 ) break;
	for ( i = 0; i < sizeof(di_gpio)/sizeof(di_gpio[0]);++i)
		ret |= (gpio_get_value(di_gpio[i].gpio)<<i);
//		ret |= (gpio_get_value(di_gpio[0].gpio)>>5);
//		printk(KERN_INFO" DI ret=%x\n",ret);
//		ret |= (gpio_get_value(di_gpio[1].gpio)>>11);
  //              printk(KERN_INFO" DI ret=%x\n",ret);
//		ret |= (gpio_get_value(di_gpio[2].gpio)>>5);
	irq_flag = 0;
//        printk(KERN_INFO" DI ret=%x\n",ret);
//	}
	//for(i=0;i<5;i++)
//		enable_irq(gpio_to_irq(retq));
	return ret;

}

static ssize_t gpio_read(struct file *file, char __user *user, size_t size,loff_t *ppos)
{
        int i;
	unsigned char val = 0;
        if (size != 1)
        	return -EINVAL;

        for ( i = 0; i < 2;++i){
               val |= (gpio_get_value(di_gpio[i].gpio)<<i);
		//val = gpio_get_value(di_gpio[i].gpio);
	//	printk(KERN_INFO" DI val=%x\n",val);
	}
       	if(!copy_to_user(user, &val, 1))
		return 1;
	else return -1;
}

static const struct file_operations  gpio_fops = 
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = gpio_ioctl,
	.read = gpio_read,
};	

static irqreturn_t gpio_irq(int irq,void *dev_id)
{
	//int i = 0;
	//unsigned char value=1;
    //for(i=0;i<5;i++){
	//	value = gpio_get_value(di_gpio[i].gpio);
        //printk(KERN_INFO"swmb irq %d\n",value);       
	//	if ( (value&&0x1) == 0){
         //       complete(&dev.gpio_done);
	//			break;
       // }
	//}
	struct di_gpio_st *di_gpioqs = (struct di_gpio_st *)dev_id;
	retq = di_gpioqs->gpio;
	irq_flag = 1;
	wake_up_interruptible(&gpio_waitq);
	return IRQ_HANDLED;
}

static void  gpio_setup_cdev(void)
{
	int err,devno = MKDEV(GPIO_MAJOR,0);

	cdev_init(&dev.cdev,&gpio_fops);
	dev.cdev.owner = THIS_MODULE;
	dev.cdev.ops = &gpio_fops;
	err = cdev_add(&dev.cdev,devno,1);
	if(err)  
		printk(KERN_INFO"setup  error!");
}
#if defined(CONFIG_OF)
static const struct of_device_id twst_di_dt_ids[] = {
        { .compatible = "fsl,twst-di", },
        { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, twst_di_dt_ids);
#endif
static int gemotech_gpio_probe(struct platform_device *ppdev)
{
	struct device *pdev = &ppdev->dev;
	struct device_node *of_node = pdev->of_node;
	int result;
	dev_t devno;
	int i,irq=0;
	
	devno = MKDEV(GPIO_MAJOR,0);

	di_class = class_create(THIS_MODULE,"di_class");

        if (IS_ERR(di_class)) {
                printk(KERN_INFO"create di class fail\n");
                return PTR_ERR(di_class);
        }
#if 1
	di_gpio[0].gpio =  of_get_named_gpio(of_node, di_gpio[0].name, 0);
	if (!gpio_is_valid(di_gpio[0].gpio)) {
                 dev_err(pdev, "pin0 gemotech gpio: invalid gpio %d\n", di_gpio[0].gpio);
                 return -1;
        }

	di_gpio[1].gpio = of_get_named_gpio(of_node, di_gpio[1].name, 0);
	if (!gpio_is_valid(di_gpio[1].gpio)) {
                 dev_err(pdev, "pin1 gemotech gpio: invalid gpio %d\n", di_gpio[1].gpio);
                 return -1;
        }
/*
	di_gpio[2].gpio = of_get_named_gpio(of_node, di_gpio[2].name, 0);
	if (!gpio_is_valid(di_gpio[2].gpio)) {
                 dev_err(pdev, "pin2 gemotech gpio: invalid gpio %d\n", di_gpio[2].gpio);
                 return -1;
        }
	di_gpio[3].gpio = of_get_named_gpio(of_node, di_gpio[3].name, 0);
	if (!gpio_is_valid(di_gpio[3].gpio)) {
                 dev_err(pdev, "pin3 gemotech gpio: invalid gpio %d\n", di_gpio[3].gpio);
                 return -1;
        }

	di_gpio[4].gpio = of_get_named_gpio(of_node, di_gpio[4].name, 0);
        if (!gpio_is_valid(di_gpio[4].gpio)) {
                 dev_err(pdev, "pin4 gemotech gpio: invalid gpio %d\n", di_gpio[4].gpio);
                 return -1;
        }
*/
	gpio_request(di_gpio[0].gpio,di_gpio[0].name);
        gpio_direction_input(di_gpio[0].gpio);

	gpio_request(di_gpio[1].gpio,di_gpio[1].name);
        gpio_direction_input(di_gpio[1].gpio);
/*	gpio_request(di_gpio[2].gpio,di_gpio[2].name);
        gpio_direction_input(di_gpio[2].gpio);
	gpio_request(di_gpio[3].gpio,di_gpio[3].name);
        gpio_direction_input(di_gpio[3].gpio);
	gpio_request(di_gpio[4].gpio,di_gpio[4].name);
        gpio_direction_input(di_gpio[4].gpio);
*/
#endif
	result = register_chrdev_region(devno,1,"di");
        if(result < 0)
		return result;

	di_device = device_create(di_class,NULL, devno,NULL, "di");

        if (IS_ERR(di_device)) {
                printk(KERN_INFO"create DI deivce fail\n");
                class_destroy(di_class);
                return -1;
        }

	gpio_setup_cdev();
	//xhf add change to 2lu  2019-3-13
	for(i=0;i<2;i++){
		irq = gpio_to_irq(di_gpio[i].gpio);
		if(irq < 0){
			dev_err(pdev,"Unable to get irq number for GPIO %d, error %d\n",di_gpio[i].gpio, irq);
            		return -1;
		}
		di_gpio[i].irq = irq;
		
/*		result = request_irq(gpio_to_irq(swmb_gpio[0].gpio), swmb_irq, IRQF_TRIGGER_FALLING|IRQF_DISABLED,
                          "swmb", &dev);//for swmb
*/
	/*	if(i==2)
			result = devm_request_any_context_irq(pdev, irq, gpio_irq, IRQF_TRIGGER_RISING, di_gpio[i].name, di_gpio+i);
		else if(i==3)
			result = devm_request_any_context_irq(pdev, irq, gpio_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, di_gpio[i].name, di_gpio+i);
			else	*/
		result = devm_request_any_context_irq(pdev, irq, gpio_irq, IRQF_TRIGGER_FALLING, di_gpio[i].name, di_gpio+i);
        
  	      if ( result < 0 ){
                	printk(KERN_INFO"di request irq failed irq=%d,error=%d\n",irq,result);
               		cdev_del(&dev.cdev);
                	device_destroy(di_class,devno);
                	class_destroy(di_class);
                	unregister_chrdev_region(devno,1);
                	return -1;
        	}
	}
   // init_completion(&dev.gpio_done);
	//xhf add 2018-10-23
	return 0;
}

static int gemotech_gpio_remove(struct platform_device *pdev)
{
	dev_t devno;
	int i = 0;
	devno = MKDEV(GPIO_MAJOR,0);
	cdev_del(&dev.cdev);
	device_destroy(di_class,devno);
        class_destroy(di_class);

	unregister_chrdev_region(devno,1);
	for ( i = 0; i < sizeof(di_gpio)/sizeof(di_gpio[0]); ++i){
		gpio_free(di_gpio[i].gpio);
	}
	return 0;
}


static struct platform_driver twst_di_driver = {
        .probe  = gemotech_gpio_probe,
        .remove = gemotech_gpio_remove,
        .driver = {
                .name   = "twst-di",
                .owner  = THIS_MODULE,
                .of_match_table = twst_di_dt_ids,
        },
};

module_platform_driver(twst_di_driver);
MODULE_LICENSE("GPL v2");

