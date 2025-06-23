# 1 virtio device与virtio_mmio_device定义
struct virtio_device {
    int index;
    bool failed;
    bool config_enabled;
    bool config_change_pending;
    spinlock_t config_lock;
    struct device dev;
    struct virtio_device_id id;
    const struct virtio_config_ops *config;
    const struct vringh_config_ops *vringh_config;
    struct list_head vqs;
    u64 features;
    void *priv;
};
struct virtio_mmio_device {
    struct virtio_device vdev;// 包含一个virtio_device 
    struct platform_device *pdev;
    void __iomem *base;
    unsigned long version;
    /* a list of queues so we can dispatch IRQs */
    spinlock_t lock;
    struct list_head virtqueues;
};
两个数据结构中都有一个virtqueue的链表。
理解这两条数组的初始化时时这篇文章的关键。

# 2 理解virtio的分层架构
核心框架层与传输层提供跨设备和传输后端的通用逻辑，是 virtio 的“大脑”。
​传输层：处理硬件相关细节，是 virtio 的“四肢”。


理解分层，只需理解virtio.ko, virtio-mmio.ko,virtio-console.ko三者的关系即可。

	• virtio.o提供总线, match机制以及probe机制。
	• virtio-mmio.o提供device，注册到bus上
	• virtio-console.o提供driver，注册到bus上


# 3 virtio.c 解析
virtio.c主要是定义一个virtio bus，并定义相关的probe函数和match函数。

## 3.1 框架
这个文件最终编出一个kernel module，下面是入口和出口函数：
static int virtio_init(void)
{
    if (bus_register(&virtio_bus) != 0)
        panic("virtio bus registration failed");
    return 0;
}
static void __exit virtio_exit(void)
{
    bus_unregister(&virtio_bus);
    ida_destroy(&virtio_index_ida);
}
core_initcall(virtio_init);
module_exit(virtio_exit);
MODULE_LICENSE("GPL");
在 Linux 内核中，bus_register() 是设备驱动模型（Device Driver Model）的核心函数之一，用于向内核注册一个新的总线类型（如 PCI、USB、virtio 等）。不做详细分析，只列出最主要的作用：
	• 初始化总线的 sysfs 接口：在 /sys/bus/ 下创建总线的目录（如 /sys/bus/virtio）。
	• ​注册总线属性​（如 drivers_autoprobe、drivers_probe 等）。
	• ​创建设备和驱动的子目录​（如 /sys/bus/virtio/devices 和 /sys/bus/virtio/drivers）。
	• ​将总线加入全局总线列表：使后续设备和驱动能够通过该总线进行注册。

## 3.2 virtio_bus
static struct bus_type virtio_bus = {
    .name  = "virtio",
    .match = virtio_dev_match,
    .dev_groups = virtio_dev_groups,
    .uevent = virtio_uevent,
    .probe = virtio_dev_probe,
    .remove = virtio_dev_remove,
};

当设备或驱动注册到该总线时，内核会调用virtio_dev_match 检查二者是否兼容。
当设备与驱动匹配成功后，调用如virtio_dev_probe初始化设备。

### 3.2.1 virtio_dev_match

这个函数触发时机：
	• 有新的设备添加到bus，也就是register_virtio_device被调用（一般是在virtio_mmio中被调用）。会对每个驱动调用这个函数。
	• 有新的驱动添加到bus，也就是register_virtio_driver被调用。（一般是在virtio-console等驱动中被调用）。这时会对每个设备调用这个函数。

static int virtio_dev_match(struct device *_dv, struct device_driver *_dr)
{
    unsigned int i;
    struct virtio_device *dev = dev_to_virtio(_dv);
    const struct virtio_device_id *ids;
          // 每一virtio driver都有一个virtio_device_id数组
         // 存放这个driver所支持的设备
    ids = drv_to_virtio(_dr)->id_table;
    for (i = 0; ids[i].device; i++)
        if (virtio_id_match(dev, &ids[i]))
            return 1;
    return 0;
}

static inline int virtio_id_match(const struct virtio_device *dev,
    const struct virtio_device_id *id)
{
        if (id->device != dev->id.device && id->device != VIRTIO_DEV_ANY_ID)
                return 0;
        return id->vendor == VIRTIO_DEV_ANY_ID || id->vendor == dev->id.vendor;
}

//每个驱动都会由一个 virtio_device_id 数组（id_table），表明它支持哪些设备和厂商组合
struct virtio_device_id {
    __u32 device;  // 设备类型标识（如网络、块设备）----由virtio规范确定，完整列表在virtio_ids.h
    __u32 vendor;  // 厂商标识（如 QEMU、特定硬件厂商）
};



match机制很简单，就是对driver中的virtio_device_id做遍历，检查是否与device中的id.device和id.vendor相匹配。



### 3.2.2 virtio_dev_probe

当有device和driver match成功后，就会调用到probe函数。
这个函数会大量用到dev->config中的函数。
dev->config是在向virtio bus注册virtio device之前，由mmio设置的！
~~~cpp
static int virtio_dev_probe(struct device *_d)
{
    int err, i;
    struct virtio_device *dev = dev_to_virtio(_d);
    struct virtio_driver *drv = drv_to_virtio(dev->dev.driver);
    u64 device_features;
    u64 driver_features;
    u64 driver_features_legacy;

    /* We have a driver! */
          // 调用 dev->config->set_status向设备的status寄存器写状态
    virtio_add_status(dev, VIRTIO_CONFIG_S_DRIVER);

    /* 调用dev->config->get_features读取设备的feature寄存器 */
    device_features = dev->config->get_features(dev);
    /* Figure out what features the driver supports. */
    driver_features = 0;
    for (i = 0; i < drv->feature_table_size; i++) {
        unsigned int f = drv->feature_table[i];
        BUG_ON(f >= 64);
        driver_features |= (1ULL << f);
    }
    /* Some drivers have a separate feature table for virtio v1.0 */
    if (drv->feature_table_legacy) {
        driver_features_legacy = 0;
        for (i = 0; i < drv->feature_table_size_legacy; i++) {
            unsigned int f = drv->feature_table_legacy[i];
            BUG_ON(f >= 64);
            driver_features_legacy |= (1ULL << f);
        }
    } else {
        driver_features_legacy = driver_features;
    }
          // VIRTIO_F_VERSION_1 是features中的第32位
         // 协商驱动和设备的特性位，并准备写入设备的feature寄存器中
    if (device_features & (1ULL << VIRTIO_F_VERSION_1))
        dev->features = driver_features & device_features;
    else
        dev->features = driver_features_legacy & device_features;

    /* Transport features always preserved to pass to finalize_features. */
        //  feature是一个64位的值，28~38位的特性是为传输层（mmio, pci）保留的。
        // 这里确保不被driver所修改
    for (i = VIRTIO_TRANSPORT_F_START; i < VIRTIO_TRANSPORT_F_END; i++)
        if (device_features & (1ULL << i))
            __virtio_set_bit(dev, i);
    if (drv->validate) {
        err = drv->validate(dev);
        if (err)
            goto err;
    }

        // 写入features的寄存器中
    err = virtio_finalize_features(dev);
    if (err)
        goto err;
         // 调用driver自己的probe函数
    err = drv->probe(dev);
    if (err)
        goto err;
    /* If probe didn't do it, mark device DRIVER_OK ourselves. */
    if (!(dev->config->get_status(dev) & VIRTIO_CONFIG_S_DRIVER_OK))
        virtio_device_ready(dev);
    if (drv->scan)
        drv->scan(dev);
    virtio_config_enable(dev);
    return 0;
err:
    virtio_add_status(dev, VIRTIO_CONFIG_S_FAILED);
    return err;
}
~~~

### 3.2.3 virtio_uevent
virtio_uevent 函数在 Linux 内核中用于为 virtio 设备生成 ​uevent 事件 的 MODALIAS 属性，帮助用户空间（如 udev）自动加载匹配的驱动模块。

也就是说，这个函数是在有设备添加进bus的时候调用的。
也就是在mmio调用的register_virtio_device里调用的。
（我们知道register_virtio_device在系统启动的枚举设备时会调用，但是在有设备热插拔时，似乎也会调用到。）

static int virtio_uevent(struct device *_dv, struct kobj_uevent_env *env)
{
    struct virtio_device *dev = dev_to_virtio(_dv);
         // 若设备 ID 为 0x1（virtio 网络设备），厂商 ID 为 0x1AF4（QEMU）
        // 则生成：MODALIAS=virtio:d00000001v00001AF4
    return add_uevent_var(env, "MODALIAS=virtio:d%08Xv%08X",
                  dev->id.device, dev->id.vendor);
}


补充关于MODALIAS的相关知识
1）MODALIAS 的作用
	• 模块自动加载：用户空间工具（如 udev 或 modprobe）通过 MODALIAS 匹配驱动的 MODULE_ALIAS，自动加载对应驱动模块。
	• ​驱动匹配规则：驱动需在 id_table 中声明支持的设备 ID 和厂商 ID，并通过 MODULE_DEVICE_TABLE 生成别名。
// 驱动代码中会声明支持的设备
static const struct virtio_device_id virtio_net_id_table[] = {
    { VIRTIO_ID_NET, VIRTIO_DEV_ANY_ID },
    { 0 },
};

// 每一个驱动都会有一个这样的语句！
//  编译的时候，depmod会将这个解析，并写入lib/modules/$(uname -r)/modules.alias
MODULE_DEVICE_TABLE(virtio, virtio_net_id_table);



2）​uevent 事件流程
	• 设备注册时，内核触发 uevent 事件。
	• MODALIAS 被传递到用户空间（会根据设备ID和厂商ID自动计算）。
	• systemd-udevd 根据 MODALIAS 查询模块别名数据库（modules.alias）。
	• 匹配后加载驱动模块（如 virtio_net）。


现在回到virtio_uevent的调用路径：

register_virtio_device
→device_add()
    → kobject_uevent(KOBJ_ADD)
       → bus->uevent()  // virtio_bus.uevent = virtio_uevent
          → 生成 MODALIAS 并发送到用户空间
               → udev 监听 uevent 事件
                    →根据 MODALIAS 查询modules.alias来加载相应驱动模块
                       →insmode xxx.ko
                           → 调用到register_virtio_driver
                              → 注册driver到bus


也就是说，在 Linux 内核中，virtio 设备的驱动加载和探测流程分为 ​模块加载（由用户空间触发）​ 和 ​总线匹配与探测（由内核总线子系统触发）​ 两个阶段。
1）设备注册与 uevent 事件
当调用 register_virtio_device 注册一个 virtio 设备时：
	• ​设备注册到总线：设备被添加到 virtio_bus（virtio 总线）的设备列表中。
	• ​触发 uevent 事件：内核调用 bus->uevent()（即 virtio_bus.uevent），生成 MODALIAS 环境变量，格式 为 virtio:d<device_id>（例如 virtio:d000004）。
	• ​用户空间响应：uevent 事件被用户空间的 udev 或 systemd-udevd 捕获。
	• 用户空间工具（如 modprobe）根据 MODALIAS 查询 /lib/modules/$(uname -r)/modules.alias，找到匹配的驱动模块名。
	• 驱动模块（如 virtio_net.ko）被动态加载到内核。

uevent 的作用是通知用户空间加载驱动模块，并不直接触发驱动的 probe。
如果驱动模块已经内置在内核中（而非模块形式），则无需此步骤。

2）驱动注册与总线匹配
当驱动模块被加载后，其驱动注册流程如下：
	• 驱动注册到总线：驱动通过 virtio_driver_register 注册到 virtio_bus，此时驱动的 id_table（设备 ID 列表）被记录到总线。
	• ​总线匹配（Match）​：virtio 总线遍历所有已注册的设备，检查设备的 device_id 是否与驱动的 id_table 匹配。匹配通过设备 ID（如 VIRTIO_ID_NET）完成
	• ​触发驱动的 probe 函数：如果匹配成功，总线调用驱动的 probe 方法（如 virtnet_probe）。
	• 在 probe 中完成设备初始化（分配资源、注册网络设备等）。


时序与依赖关系：
​(1) 设备注册先于驱动加载
uevent 触发模块加载 → 驱动注册到总线 → 总线匹配设备 → 调用 probe。此时 probe 发生在驱动加载之后。
​(2) 驱动已加载后注册设备
设备注册到总线 → 总线直接匹配到驱动 → 立即调用 probe。无需 uevent 触发模块加载。


常见问题解答：
​Q1：如果手动加载驱动模块（如 insmod virtio_net.ko），是否还需要 uevent？
不需要。驱动模块加载后，总线会立即尝试匹配已注册的设备。如果设备已存在，直接触发 probe。
​
Q2：如何验证驱动是否匹配成功？
通过 /sys/bus/virtio/devices/<device>/driver 符号链接，确认设备是否绑定到驱动。
使用 dmesg 查看内核日志，观察驱动的 probe 函数是否被调用。


总结：在 Linux 内核的设备驱动模型中，设备插入时触发的 uevent 和驱动加载后的设备匹配会涉及 ​两次设备与驱动的匹配验证（uevent和probe的时候）。不过，这两次验证是内核总线机制的正常设计，目的是确保无论设备或驱动哪个先注册，都能正确匹配并触发初始化。



![image](https://github.com/user-attachments/assets/e78f0465-f412-4281-ade0-f7dd75ddd0f1)
