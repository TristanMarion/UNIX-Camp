#include <linux/kernel.h>
#include <linux/cdev.h>    // char driver, peut utiliser cdev
#include <linux/fs.h>     // open/close --- /read/write
#include <linux/module.h>
#include <linux/semaphore.h>  // syncro
#include <asm/uaccess.h>   //copy to/from user

struct device {
  char data[1000];
  struct semaphore sem; // prevent corruption
} virtual_device;

// On declare les variables en global pour eviter de saturer la statck du kernel

struct cdev *my_cdev;
int major_number; // variable pour garder nombres majeurs extrait
int ret; //variable pour return

dev_t dev_num; // stop les nombres majeurs

#define DEVICE_NAME	 	"samynaceri"

int device_open(struct inode *inode, struct file *filp){
  if (down_interruptible(&virtual_device.sem) != 0){
    printk(KERN_ALERT "test device: could not lock device during open");
    return -1;
  }
  
  printk(KERN_INFO "test device : device opened !");
  return 0;
}

ssize_t	device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
  printk(KERN_INFO "test device : lecture du device");
  ret = copy_to_user(bufStoreData, virtual_device.data,bufCount);
  return ret;
}

ssize_t	device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
  printk(KERN_INFO "test device: ecriture dans le device");
  ret = copy_from_user(virtual_device.data, bufSourceData, bufCount);
  return ret;
}

int	device_close(struct inode *inode, struct file *filp){
  up(&virtual_device.sem);
  printk(KERN_INFO "test device : closed...");
  return 0;
}

struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .write = device_write,
  .read = device_read
};

static int driver_entry(void){ // point d'entree 
  ret = alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME); //allocation dynamique
  if (ret < 0){
    printk(KERN_ALERT "test device : erreur d'allocation");
    return ret;
  }
  major_number = MAJOR(dev_num); //extraction des nombres majeurs avec la macro MAJOR
  printk(KERN_INFO "test device : le nombre majeur est %d", major_number);
  printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file",DEVICE_NAME,major_number);  //dmesg
  my_cdev = cdev_alloc();  // creation de la structure cdev
  my_cdev->ops = &fops; 
  my_cdev->owner = THIS_MODULE;
  ret = cdev_add(my_cdev, dev_num, 1); //on add cdev au kernel
  if (ret < 0){
    printk(KERN_ALERT "test device : impossible d'add cdev au kernel");
    return ret;
  }
  sema_init(&virtual_device.sem,1); // initialisation du semaphore, il permet de synchro l'acces aux ressources
  return 0;
}

static void driver_exit(void){
  cdev_del(my_cdev); // on del cdev du kernel
  unregister_chrdev_region(dev_num, 1); // on free le alloc_chrdev_region
  printk(KERN_ALERT "test device: unloaded module");
}

module_init(driver_entry); //on precise au module ou commencer
module_exit(driver_exit); // et ou finir
