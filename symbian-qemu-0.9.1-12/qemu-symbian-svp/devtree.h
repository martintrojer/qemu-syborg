#include "hw/hw.h"

typedef struct QEMUDeviceClass QEMUDeviceClass;
typedef struct QEMUDevice QEMUDevice;

typedef void (*QDEVCreateFn)(QEMUDevice *);

extern QEMUDeviceClass *cpu_device_class;

extern const void *machine_devtree;
extern int machine_devtree_size;

typedef struct {
    target_phys_addr_t base;
    ram_addr_t size;
} devtree_ram_region;

extern devtree_ram_region *devtree_ram_map;
extern int devtree_ram_map_size;

void register_devices(void);
void cpu_device_register(void);
void cpu_bootstrap(const char *kernel_filename, const char *kernel_cmdline,
                   const char *initrd_filename);

int devtree_get_config_int(const char * name, int def);

QEMUDeviceClass *qdev_new(const char *name, QDEVCreateFn create, int nirq);
void qdev_add_chardev(QEMUDeviceClass *dc);
void qdev_add_registers(QEMUDeviceClass *dc, CPUReadMemoryFunc **mem_read,
                        CPUWriteMemoryFunc **mem_write,
                        target_phys_addr_t mem_size);
void qdev_add_property_string(QEMUDeviceClass *dc, const char *name,
                              const char *def);
void qdev_add_property_int(QEMUDeviceClass *dc, const char *name, int def);
void qdev_add_savevm(QEMUDeviceClass *dc, int ver,
                     SaveStateHandler *save_state,
                     LoadStateHandler *load_state);
void qdev_add_class_opaque(QEMUDeviceClass *dc, void *opaque);

void qdev_set_irq_level(QEMUDevice *dev, int n, int level);
void qdev_get_irq(QEMUDevice *dev, int n, qemu_irq *p);
CharDriverState *qdev_get_chardev(QEMUDevice *dev);
void qdev_create_interrupts(QEMUDevice *dev, qemu_irq_handler handler, 
                            void *opaque, int n);
int qdev_get_property_int(QEMUDevice *dev, const char *name);
const char *qdev_get_property_string(QEMUDevice *dev, const char *name);
const char *qdev_get_name(QEMUDevice *dev);
void *qdev_get_class_opaque(QEMUDevice *dev);
void qdev_set_opaque(QEMUDevice *dev, void *opaque);
void qdev_set_region_opaque(QEMUDevice *dev, int n, void *opaque);

#ifdef DEVICE_NAME
void glue(DEVICE_NAME, _register)(void);
#endif
