# 一 v1.1规范解读
virtio-mmio 是 virtio 规范中定义的 ​内存映射 I/O（Memory-Mapped I/O）传输层，专为无 PCI 总线的系统设计（如嵌入式设备或 ARM 虚拟化平台）。它通过直接读写内存地址与 virtio 设备交互，实现轻量级、低延迟的虚拟化 I/O 通信。以下是其核心实现细节.

（基于virtio1.1规范分析，1.2和1.3改动似乎不是特别大）


## 1.1  设计背景与适用场景
目标场景:
	• 无 PCI 总线的系统：如 ARM 虚拟机（QEMU/KVM 的 virt 机器类型）、RISC-V 嵌入式设备。
	• 轻量级虚拟化：避免 PCI 协议的复杂性，简化设备初始化流程。
	• 快速原型开发：通过静态内存地址直接操作设备，便于调试。​

典型应用:
	• QEMU 虚拟机：使用 virtio-net-mmio 或 virtio-blk-mmio 设备。
	• 嵌入式 Linux：通过设备树绑定内存地址，驱动硬件加速器或虚拟设备。

## 1.2 设备发现与初始化
设备发现：
	• 通过 ​设备树（Device Tree） 定义设备的内存地址和中断号。

设备树示例：
/ {
    virtio_net@a000000 {
        compatible = "virtio,mmio";
        reg = <0x0 0x0a000000 0x0 0x200>;
        interrupts = <0x0 0x10 0x1>;
        interrupt-parent = <&intc>;
    };

    virtio_blk@a000200 {
        compatible = "virtio,mmio";
        reg = <0x0 0x0a000200 0x0 0x200>;
        interrupts = <0x0 0x11 0x1>;
        interrupt-parent = <&intc>;
    };
};
设备地址 0x0a000000 和长度 0x200 对应 virtio-mmio 设备的寄存器区域。

初始化流程：
	• 驱动探测：内核匹配设备树中的 compatible 字段，调用 virtio_mmio_probe。
	• 映射内存：将设备寄存器区域映射到内核虚拟地址空间。
	• ​版本校验：读取 VIRTIO_MMIO_MAGIC_VALUE 和 VIRTIO_MMIO_VERSION 确认设备兼容性。
	• ​特性协商：驱动和设备协商支持的 virtio 特性（如 VIRTIO_F_RING_INDIRECT_DESC）。



## 1.3 寄存器布局（后端设备的）
MMIO virtio设备 要提供一系列的内存映射控制寄存器以及一个Device-Specific的配置空间。
以此来供前端驱动的读写。
具体如下所示：

寄存器名称	偏移量	方向	功能描述	值类型/示例	重要注意事项
​MagicValue	0x000	RO	设备身份验证	固定值0x74726976 ("virt" LE编码)	驱动初始化时首个校验项
​Version	0x004	RO	协议版本号	0x2 (VirtIO 1.1+), 传统设备返回0x1	版本不匹配必须放弃初始化
​DeviceID	0x008	RO	设备类型标识	1=网络设备, 2=块设备等	0x0表示占位设备
​VendorID	0x00c	RO	厂商标识	0x1AF4 (OASIS标准厂商ID)	用于设备溯源
​DeviceFeatures	0x010	RO	设备支持的功能位图	32位位图，由DeviceFeaturesSel选择功能块	必须先写DeviceFeaturesSel
​DeviceFeaturesSel	0x014	WO	功能块选择器	0-选择0-31位，1-选择32-63位	多块功能需分次读取
​DriverFeatures	0x020	WO	驱动启用的功能位图	32位位图，由DriverFeaturesSel选择功能块	必须先写DriverFeaturesSel
​DriverFeaturesSel	0x024	WO	驱动功能块选择器	同DeviceFeaturesSel	功能协商的核心控制寄存器
​QueueSel	0x030	WO	队列选择器	0-based队列索引	操作队列前必须设置
​QueueNumMax	0x034	RO	队列最大容量	元素数量，0表示不可用	配置队列前必须检查
​QueueNum	0x038	WO	队列实际大小	≤ QueueNumMax	激活队列前必须设置
​QueueReady	0x044	RW	队列激活状态	0=停用，1=激活	修改后需读取确认状态
​QueueNotify	0x050	WO	队列通知寄存器	传统模式：16位队列索引	需协商VIRTIO_F_NOTIFICATION_DATA
				扩展模式：32位{ vqn:16, next_off:15, next_wrap:1 }
​InterruptStatus	0x060	RO	中断状态	Bit0: 已用缓冲通知	需配合InterruptACK使用
				Bit1: 配置变更通知
​InterruptACK	0x064	WO	中断确认	写入处理完成的中断位	必须严格对应InterruptStatus的值
​Status	0x070	RW	设备状态机	状态位：ACKNOWLEDGE(1)→DRIVER(2)→FEATURES_OK(8)→DRIVER_OK(4)	写0触发设备复位
​QueueDescLow	0x080	WO	描述符表物理地址低32位	64位地址的低半部分	需与QueueDescHigh配合使用
​QueueDescHigh	0x084	WO	描述符表物理地址高32位	64位地址的高半部分	现代系统通常为0
​QueueDriverLow	0x090	WO	可用环(Driver Area)地址低32位	同QueueDesc	必须保证物理连续
​QueueDriverHigh	0x094	WO	可用环地址高32位	同QueueDesc	
​QueueDeviceLow	0x0a0	WO	已用环(Device Area)地址低32位	同QueueDesc	需4字节对齐
​QueueDeviceHigh	0x0a4	WO	已用环地址高32位	同QueueDesc	
​ConfigGeneration	0x0fc	RO	配置原子性版本号	任意非零值	读取配置前需记录，修改后校验
​Config	0x100+	RW	设备特定配置空间	设备定义结构	



## 1.4 规范对设备的要求
virtio 规范对我们配置后端设备时的一些额外要求：
1）身份验证
	• 设备的MagicValue必须是0x74726976，设备的Version必须是0x2（1.1+都是2）。设备发现时驱动会首先校验这两个值，失败则放弃初始化。
2）中断管理
	• 事件发生时InterruptStatus对应bit立即置位，保持置位直到驱动写InterruptACK确认。未触发事件的bit必须保持0
	• 复位时所有中断状态bit和队列就绪标志清零
3）配置同步
	• 配置空间变化时递增ConfigGeneration
	• 驱动需通过轮询该值检测配置变更（防ABA问题）
4）队列访问控制
	• QueueReady=0时禁止访问队列内存
	• 确保队列未就绪时设备不触发DMA操作

## 1.5 规范对驱动的要求
1）驱动程序不得访问1.3中未描述的内存位置
2）驱动程序必须只使用32位宽和对齐的读写来访问1.3描述的设备的控制寄存器。对于特定于设备的配置空间，驱动程序必须对8位宽字段使用8位宽访问，对16位宽字段采用16位宽和对齐访问，对32和64位宽字段则采用32位宽和匹配访问。
3）驱动程序必须忽略MagicValue不是0x74726976的设备，尽管它可能会报告错误。
4）驱动程序必须忽略Version不是0x2的设备，尽管它可能会报告错误。
5）驱动程序必须忽略Device ID为0x0的设备，但不得报告任何错误。
6）在读取DeviceFeatures之前，驱动程序必须向DeviceFeaturesSel写入一个值。
7）在写入DriverFeatures寄存器之前，驱动程序必须向DriverFeaturesSel寄存器写入一个值。
8）驱动程序必须向QueueNum写入一个值，该值小于或等于设备在QueueNumMax中显示的值。
9）当QueueReady不为零时，驱动程序不得访问QueueNum、 QueueDescLow、 QueueDescHigh、 QueueAvailLow、 QueueAvailHigh、 QueueUsedLow、 QueueUsedHigh。
10）要停止使用队列，驱动程序必须将零（0x0）写入此QueueReady，并必须读回该值以确保同步。
11）驱动程序必须忽略中断状态中的未定义位。
12）驱动程序必须在处理完中断后，将一个带有描述其处理的事件的位掩码的值写入InterruptACK，并且不得在值中设置任何未定义的位。

## 1.6 MMIO特有的初始化和设备配置（对在Driver中初始化Device时的要求）
### 1.6.1 设备初始化
驱动 ​必须 首先读取并校验MagicValue和Version寄存器，若任一校验失败，驱动 ​必须 中止初始化，且 ​不得 访问其他寄存器。
若校验通过，驱动读取Device ID，若 DeviceID = 0x0，表示设备为占位符，驱动 ​必须 静默中止初始化。
若设备通过初始校验，驱动需遵循Virtual I/O Device (VIRTIO) Version 1.1的3.1节通用设备初始化流程。

### 1.6.2 Virtqueue配置
驱动需按以下步骤初始化虚拟队列：
1）选择队列
	• 向 ​QueueSel (0x030) 写入队列索引（首个队列为 0）
2）检查队列状态
	• 读取 ​QueueReady (0x044)，预期值为 0x0（队列未激活）。若值非零，说明队列已被占用，需中止配置。
3）获取队列容量
	• 读取 ​QueueNumMax (0x034)，获取设备支持的队列最大元素数。若值为 0x0，表示队列不可用。
4）​分配队列内存
	• 内存必须 ​物理连续​，内存区域需初始化为零。
5）设置队列参数
	• 向 ​QueueNum (0x038) 写入实际队列大小（需 ≤ QueueNumMax）
	• 向以下寄存器对写入物理地址：
	​QueueDescLow/High (0x080/0x084)：描述符表地址。
	​QueueDriverLow/High (0x090/0x094)：可用环（Driver Area）地址。
	​QueueDeviceLow/High (0x0a0/0x0a4)：已用环（Device Area）地址。
	• 对齐要求
	描述符表：16 字节对齐
	可用环：2 字节对齐
	已用环：4 字节对齐
6）激活队列
	• 向 ​QueueReady (0x044) 写入 0x1，驱动需读取该寄存器确认激活成功。
	
1.6.3 可用的Buffer通知

1.6.4 来自设备端的通知
Device通过中断来通知Driver有事件发生（如缓冲区已用、配置变更）
1）中断触发条件
	• ​Used Buffer Notification​（位 0）：至少一个活跃队列有已用缓冲区。
	• ​Configuration Change​（位 1）：设备配置变更（如网络设备链接状态变化）。

2）​驱动处理流程
	• 读取 ​InterruptStatus (0x060) 获取中断原因。
	• 处理事件
		if (status & VIRTIO_MMIO_INT_VRING) {
		    // 处理所有活跃队列的已用缓冲区
		    for_each_active_vq(vq) {
		        virtqueue_get_buf(vq);
		    }
		}
		if (status & VIRTIO_MMIO_INT_CONFIG) {
		    // 处理配置变更
		    read_config();
		}
	• 确认中断：向 ​InterruptACK (0x064) 写入处理过的事件位掩码。

2 virtio-mmio驱动源码分析
2.1 驱动架构分析
整体架构看如下代码即可：

static const struct of_device_id virtio_mmio_match[] = {
    { .compatible = "virtio,mmio", },
    {},
};


static struct platform_driver virtio_mmio_driver = {
    .probe      = virtio_mmio_probe,
    .remove     = virtio_mmio_remove,
    .driver     = {
        .name   = "virtio-mmio",
        .of_match_table = virtio_mmio_match,
        .acpi_match_table = ACPI_PTR(virtio_mmio_acpi_match),
    },
};
static int __init virtio_mmio_init(void)
{
    return platform_driver_register(&virtio_mmio_driver);
}


module_init(virtio_mmio_init);


也就是完成如下的工作：
	• 定义一个platform_driver，并将其注册到platform的bus

platform bus 的match函数会从设备树中寻找与virtio_mmio_match相匹配的节点，match成功后，先调用platform bus中的probe函数。创建platform device，然后再调用virtio_mmio中的probe函数。

virtio_mmio中的probe函数实际上就是将通过设备树识别到的platform device转换成virtio device，并注册到virtio bus上去。

也就是说，virtio_mmio实际上完成的是向virtio bus上注册设备，以便virtio blk、console等设备的driver来探测。

## 2.2 virtio_mmio 的probe函数
函数实现为：
static int virtio_mmio_probe(struct platform_device *pdev)
{
    struct virtio_mmio_device *vm_dev;
    struct resource *mem;
    unsigned long magic;
    int rc;
    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem)
        return -EINVAL;

        // 请求占用该内存区域，若区域已被占用，返回 -EBUSY
    if (!devm_request_mem_region(&pdev->dev, mem->start,
            resource_size(mem), pdev->name))
        return -EBUSY;
        
        //分配内存
    vm_dev = devm_kzalloc(&pdev->dev, sizeof(*vm_dev), GFP_KERNEL);
    if (!vm_dev)
        return -ENOMEM;

        //成员赋值
    vm_dev->vdev.dev.parent = &pdev->dev;
    vm_dev->vdev.dev.release = virtio_mmio_release_dev;
    vm_dev->vdev.config = &virtio_mmio_config_ops;// 重要的！！
    vm_dev->pdev = pdev;
    INIT_LIST_HEAD(&vm_dev->virtqueues);
    spin_lock_init(&vm_dev->lock);
        //寄存器映射与验证--mmio的核心就是这里的地址映射
    vm_dev->base = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
    if (vm_dev->base == NULL)
        return -EFAULT;
        
        // 地址映射完，可以读写设备的寄存器了，也就设备的物理地址了
        // 设备的基地址映射到vm_dev->base
        
        // 协议相关，见上
    /* Check magic value */
    magic = readl(vm_dev->base + VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != ('v' | 'i' << 8 | 'r' << 16 | 't' << 24)) {
        dev_warn(&pdev->dev, "Wrong magic value 0x%08lx!\n", magic);
        return -ENODEV;
    }
    /* Check device version，必须要是1或2 */
    vm_dev->version = readl(vm_dev->base + VIRTIO_MMIO_VERSION);
    if (vm_dev->version < 1 || vm_dev->version > 2) {
        dev_err(&pdev->dev, "Version %ld not supported!\n",
                vm_dev->version);
        return -ENXIO;
    }
    vm_dev->vdev.id.device = readl(vm_dev->base + VIRTIO_MMIO_DEVICE_ID);
    if (vm_dev->vdev.id.device == 0) {
        /*
         * virtio-mmio device with an ID 0 is a (dummy) placeholder
         * with no function. End probing now with no error reported.
         */
        return -ENODEV;
    }
    vm_dev->vdev.id.vendor = readl(vm_dev->base + VIRTIO_MMIO_VENDOR_ID);
        // version是1，代表是传统设备，要有特殊处理，一般1.1+的标准都是2
    if (vm_dev->version == 1) {
        writel(PAGE_SIZE, vm_dev->base + VIRTIO_MMIO_GUEST_PAGE_SIZE);
        rc = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
        /*
         * In the legacy case, ensure our coherently-allocated virtio
         * ring will be at an address expressable as a 32-bit PFN.
         */
        if (!rc)
            dma_set_coherent_mask(&pdev->dev,
                          DMA_BIT_MASK(32 + PAGE_SHIFT));
    } else {
        rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
    }
    if (rc)
        rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
    if (rc)
        dev_warn(&pdev->dev, "Failed to enable 64-bit or 32-bit DMA.  Trying to continue, but this might not work.\n");

        // 将设备数据绑定到平台设备，并注册 VirtIO 设备。
    platform_set_drvdata(pdev, vm_dev);
    rc = register_virtio_device(&vm_dev->vdev);
    if (rc)
        put_device(&vm_dev->vdev.dev);
    return rc;
}
### 2.2.1 platform_device的定义：
struct platform_device {
        const char	*name;
        int		id;
        bool		id_auto;
        struct device	dev; //  通过包含device，实现C++中的继承
        u64		platform_dma_mask;
        struct device_dma_parameters dma_parms;
        u32		num_resources;
        struct resource	*resource;

        const struct platform_device_id	*id_entry;
        /*
         * Driver name to force a match.  Do not set directly, because core
         * frees it.  Use driver_set_override() to set or clear it.
         */
        const char *driver_override;

        /* MFD cell pointer */
        struct mfd_cell *mfd_cell;

        /* arch specific additions */
        struct pdev_archdata	archdata;
};
这里的resource是设备树所描述的内存的信息，是指向设备资源的数组。
每个资源由一个 struct resource 结构体表示，描述了资源的类型、起始地址、大小等信息。

### 2.2.2 virtio_mmio_device 定义
struct virtio_mmio_device {
    struct virtio_device vdev;
    struct platform_device *pdev;
    void __iomem *base; // 内存映射后的设备的基地址
    unsigned long version;
    /* a list of queues so we can dispatch IRQs */
    spinlock_t lock;
    struct list_head virtqueues;
};

### 2.2.3 关键函数解析
1）platform_get_resource(pdev, IORESOURCE_MEM, 0);
这个函数通过读取platform_device的resources成员，返回其IORESOURCE_MEM类型的resource。
实际上，再设备树中我们只配置了 reg = <0x0 0x0a000200 0x0 0x200>，也就是地址信息，这恰好是IORESOURCE_MEM类型的。

2）devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
进行寄存器映射，将设备的物理内存区域映射到内核虚拟地址空间。
物理内存地址是通过设备树描述，并通过platform_get_resource函数获取的。

3）platform_set_drvdata(pdev, vm_dev)
将vm_dev赋值给pdev->dev->driver_data. 
将驱动私有数据与平台设备关联起来，方便在驱动的其他部分通过 platform_get_drvdata 访问这些数据。这是 Linux 设备驱动中管理设备状态和配置信息的常用方法


## 2.3 virtio_mmio_config_ops
再virtio_mmio_probe函数中，还有一个核心的地方就是virtio_mmio_config_ops，这里面定义了一些驱动与设备交互的一些实现。
这些实现才是真正提供给virtio_console、virtio_blk等driver使用到的函数。也是与pci的不同之处。
static const struct virtio_config_ops virtio_mmio_config_ops = {
    .get        = vm_get,
    .set        = vm_set,
    .generation = vm_generation,
    .get_status = vm_get_status,
    .set_status = vm_set_status,
    .reset      = vm_reset,
    .find_vqs   = vm_find_vqs,
    .del_vqs    = vm_del_vqs,
    .get_features   = vm_get_features,
    .finalize_features = vm_finalize_features,
    .bus_name   = vm_bus_name,
};


### 2.3.1 vm_get
该函数用于从virtio设备的配置空间的指定偏移处取信息。
取多少信息取决于len，结果放到buf中。
这里面的VIRTIO_MMIO_CONFIG值是0x100，从协议中查找这是设备的配置空间处。
static void vm_get(struct virtio_device *vdev, unsigned offset,
           void *buf, unsigned len)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    void __iomem *base = vm_dev->base + VIRTIO_MMIO_CONFIG;
    u8 b;
    __le16 w;
    __le32 l;
    if (vm_dev->version == 1) {
        u8 *ptr = buf;
        int i;
        for (i = 0; i < len; i++)
            ptr[i] = readb(base + offset + i);
        return;
    }
    switch (len) {
    case 1:
        b = readb(base + offset);
        memcpy(buf, &b, sizeof b);
        break;
    case 2:
        w = cpu_to_le16(readw(base + offset));
        memcpy(buf, &w, sizeof w);
        break;
    case 4:
        l = cpu_to_le32(readl(base + offset));
        memcpy(buf, &l, sizeof l);
        break;
    case 8:
        l = cpu_to_le32(readl(base + offset));
        memcpy(buf, &l, sizeof l);
        l = cpu_to_le32(ioread32(base + offset + sizeof l));
        memcpy(buf + sizeof l, &l, sizeof l);
        break;
    default:
        BUG();
    }
}

### 2.3.2 vm_set
与vm_get类似，向virtio设备的配置空间的指定偏移处写消息，内容为buf，长度为len。

static void vm_set(struct virtio_device *vdev, unsigned offset,
           const void *buf, unsigned len)


### 2.3.3 vm_generation
VIRTIO_MMIO_CONFIG_GENERATION是0x0fc，查询协议为ConfigGeneration，见最上面的协议内容。
这个字段用来：读取配置前需记录，修改后校验。
static u32 vm_generation(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    if (vm_dev->version == 1)
        return 0;
    else
        return readl(vm_dev->base + VIRTIO_MMIO_CONFIG_GENERATION);
}


### 2.3.4 vm_get_status
VIRTIO_MMIO_STATUS是0x070，见上面协议部分.
读取设备的Status字段，也就是设备的状态机
static u8 vm_get_status(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    return readl(vm_dev->base + VIRTIO_MMIO_STATUS) & 0xff;
}



### 2.3.5 vm_set_status
向0x070处写，设置设备的Status字段。
static void vm_set_status(struct virtio_device *vdev, u8 status)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    /* We should never be setting status to 0. */
    BUG_ON(status == 0);
    writel(status, vm_dev->base + VIRTIO_MMIO_STATUS);
}

### 2.3.6 vm_reset
向Status字段写入0，查看协议可知，这是复位设备。也就是0是设备状态机的起始状态。

static void vm_reset(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    /* 0 status means a reset. */
    writel(0, vm_dev->base + VIRTIO_MMIO_STATUS);
}

### 2.3.7 vm_find_vqs
用于创建虚拟队列。创建几条队列，是传入的参数决定的。
比如说virtio-blk和virtio-net的队列数量不一样，传入的这个参数就不一样

static int vm_find_vqs(struct virtio_device *vdev, unsigned nvqs,
               struct virtqueue *vqs[],
               vq_callback_t *callbacks[],
               const char * const names[],
               const bool *ctx,
               struct irq_affinity *desc)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    unsigned int irq = platform_get_irq(vm_dev->pdev, 0);
    int i, err, queue_idx = 0;
    err = request_irq(irq, vm_interrupt, IRQF_SHARED,
            dev_name(&vdev->dev), vm_dev);
    if (err)
        return err;
    for (i = 0; i < nvqs; ++i) {
        if (!names[i]) {
            vqs[i] = NULL;
            continue;
        }
                   //创建virtqueue
        vqs[i] = vm_setup_vq(vdev, queue_idx++, callbacks[i], names[i],
                     ctx ? ctx[i] : false);
                //任意队列创建失败，清理所有队列
        if (IS_ERR(vqs[i])) {
            vm_del_vqs(vdev);
            return PTR_ERR(vqs[i]);
        }
    }
    return 0;
}

1）platform_get_irq(vm_dev->pdev, 0)
从platform设备中获取第0个中断号。
platform设备的中断信息是在设备树中描述的。
设备树中，设备节点的 interrupts 属性定义了设备的中断信息。
例如：
my_device {
    compatible = "vendor,my-device";
    reg = <0x1000 0x100>;
    interrupts = <0 5 4>; // 第一个中断描述符
};
interrupts 属性的格式为 <中断控制器索引 中断号 触发方式>


2）request_irq(irq, vm_interrupt, IRQF_SHARED, dev_name(&vdev->dev), vm_dev);
将中断号与中断处理函数绑定：当设备触发中断时，内核调用注册的处理函数。

函数原型为：
int request_irq(unsigned int irq, irq_handler_t handler,
               unsigned long flags, const char *name, void *dev);

irq: 中断号（由 platform_get_irq 获取）。
​handler: 中断处理函数（如 vm_interrupt）。
​flags 中断标志（如 IRQF_SHARED）。IRQF_SHARED：允许多个设备共享同一中断号。
​name: 中断名称（用于 /proc/interrupts 显示）。
​dev: 传递给中断处理函数的设备指针（用于共享中断时区分设备）。在共享中断场景下，dev 用于唯一标识设备。当中断触发时，内核会遍历所有注册到该中断的处理函数，并通过 dev 区分设备

​返回值：成功返回 0，失败返回错误码（如 -EBUSY）。

3）vm_interrupt
用于处理设备的中断事件（如配置变更或虚拟队列数据到达）
也就是说，设备有事件发生时，就会手动去触发中断。
具体来讲，设备侧有数据就绪、配置变更时，就会往特定的寄存器里写入中断的类型。
例如写入 VIRTIO_MMIO_INTERRUPT_STATUS 寄存器的特定位（如 VIRTIO_MMIO_INT_VRING）来标记中断类型。
然后通过eventfd 或 irqfd 等机制通知Guest驱动（暂时不确实是哪种机制）。

Guest的驱动得到中断通知后，就会调用这里注册的vm_interrupt！
这里的vm_interrupt的实现可以整理为以下步骤：
	• 读取设备的InterruptStatus寄存器（偏移0x60）
	• 将读到的中断状态写入InterruptACK寄存器，表示确认中断，并清除中断标志，避免重复触发
	• 判断中断的类型是什么
	• 如果是配置发生了变更（如 Virtio 设备的特性协商完成），调用更上层驱动的函数来处理设备配置变更事件。
	• 如果是数据就绪，调用更上层驱动的函数来处理设备的数据就绪事件。

static irqreturn_t vm_interrupt(int irq, void *opaque)
{
    struct virtio_mmio_device *vm_dev = opaque;
    struct virtio_mmio_vq_info *info;
    unsigned long status;
    unsigned long flags;
    irqreturn_t ret = IRQ_NONE;
    /* Read and acknowledge interrupts */
    status = readl(vm_dev->base + VIRTIO_MMIO_INTERRUPT_STATUS);
    writel(status, vm_dev->base + VIRTIO_MMIO_INTERRUPT_ACK);
    if (unlikely(status & VIRTIO_MMIO_INT_CONFIG)) {
        virtio_config_changed(&vm_dev->vdev);
        ret = IRQ_HANDLED;
    }
    if (likely(status & VIRTIO_MMIO_INT_VRING)) {
        spin_lock_irqsave(&vm_dev->lock, flags);
        list_for_each_entry(info, &vm_dev->virtqueues, node)
            ret |= vring_interrupt(irq, info->vq);
        spin_unlock_irqrestore(&vm_dev->lock, flags);
    }
    return ret;
}
virtio_config_changed和vring_interrupt是如何实现的，在virtio.c和vring.c中实现的。
这里来说不重要，暂不分析。

4）vm_setup_vq
用于创建虚拟队列。最重要的一个函数。

static struct virtqueue *vm_setup_vq(struct virtio_device *vdev, unsigned index,
                  void (*callback)(struct virtqueue *vq),
                  const char *name, bool ctx)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    struct virtio_mmio_vq_info *info;
    struct virtqueue *vq;
    unsigned long flags;
    unsigned int num;
    int err;
    if (!name)
        return NULL;
    /* Select the queue we're interested in */
          //  向VIRTIO_MMIO_QUEUE_SEL寄存器写内容，来选择当前操作的队列索引
          // vm_setup_vq是循环调用的，每次调用都会传入index
    writel(index, vm_dev->base + VIRTIO_MMIO_QUEUE_SEL);
    /* Queue shouldn't already be set up. */
         // 队列不应该是ready的，因为现在还没激活
    if (readl(vm_dev->base + VIRTIO_MMIO_QUEUE_READY)) {
        err = -ENOENT;
        goto error_available;
    }
    /* Allocate and fill out our active queue description */
    info = kmalloc(sizeof(*info), GFP_KERNEL);
    if (!info) {
        err = -ENOMEM;
        goto error_kmalloc;
    }
         // 读取设备的最大queue数量
    num = readl(vm_dev->base + VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (num == 0) {
        err = -ENOENT;
        goto error_new_virtqueue;
    }
    /* Create the vring */
         // 创建并初始化vring（描述符环、可用环、已用环）
    vq = vring_create_virtqueue(index, num, VIRTIO_MMIO_VRING_ALIGN, vdev,
                 true, true, ctx, vm_notify, callback, name);
    if (!vq) {
        err = -ENOMEM;
        goto error_new_virtqueue;
    }
    /* Activate the queue */
         // 激活队列 
    writel(virtqueue_get_vring_size(vq), vm_dev->base + VIRTIO_MMIO_QUEUE_NUM);
    if (vm_dev->version == 1) {
           …
           …        
    } else {
        u64 addr;
                    // 将创建好的virtqueue的地址信息，写到设备的相关寄存器中
                   // 也就是说，virtqueue是在Guest的驱动端维护的
        addr = virtqueue_get_desc_addr(vq);
        writel((u32)addr, vm_dev->base + VIRTIO_MMIO_QUEUE_DESC_LOW);
        writel((u32)(addr >> 32),
                vm_dev->base + VIRTIO_MMIO_QUEUE_DESC_HIGH);
        addr = virtqueue_get_avail_addr(vq);
        writel((u32)addr, vm_dev->base + VIRTIO_MMIO_QUEUE_AVAIL_LOW);
        writel((u32)(addr >> 32),
                vm_dev->base + VIRTIO_MMIO_QUEUE_AVAIL_HIGH);
        addr = virtqueue_get_used_addr(vq);
        writel((u32)addr, vm_dev->base + VIRTIO_MMIO_QUEUE_USED_LOW);
        writel((u32)(addr >> 32),
                vm_dev->base + VIRTIO_MMIO_QUEUE_USED_HIGH);
               // 激活这个virtqueue
        writel(1, vm_dev->base + VIRTIO_MMIO_QUEUE_READY);
    }
    vq->priv = info;
    info->vq = vq;
        // 将创建好的这个virtqueue放在设备的 virtqueues 链表
    spin_lock_irqsave(&vm_dev->lock, flags);
    list_add(&info->node, &vm_dev->virtqueues);
    spin_unlock_irqrestore(&vm_dev->lock, flags);
    return vq;
}

### 2.3.8 vm_get_features
根据协议读取设备寄存器中存放的特性（64位）
features每一位代表的属性，位于include/linux/virtio_config.h中。

static u64 vm_get_features(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    u64 features;
         // 写1 表示要读取高32位特性 
    writel(1, vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES_SEL);
    features = readl(vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES);
    features <<= 32;
         // 写0表示要读取高32位特性 
    writel(0, vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES_SEL);
    features |= readl(vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES);
    return features;
}

### 2.3.8  vm_finalize_features

读取设备所支持的特性，驱动会根据自己所支持的特性，确定两者之间共同支持的一些特性。
这个函数就是将协商好的共同支持的特性，写到设备的VIRTIO_MMIO_DRIVER_FEATURES寄存器中。

static int vm_finalize_features(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    /* Give virtio_ring a chance to accept features. */
    vring_transport_features(vdev);
    /* Make sure there is are no mixed devices */
    if (vm_dev->version == 2 &&
            !__virtio_test_bit(vdev, VIRTIO_F_VERSION_1)) {
        dev_err(&vdev->dev, "New virtio-mmio devices (version 2) must provide VIRTIO_F_VERSION_1 feature!\n");
        return -EINVAL;
    }
    writel(1, vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES_SEL);
    writel((u32)(vdev->features >> 32),
            vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES);
    writel(0, vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES_SEL);
    writel((u32)vdev->features,
            vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES);
    return 0;
}
![image](https://github.com/user-attachments/assets/269e2cd7-da97-40a4-a586-de2dddfe1f12)
