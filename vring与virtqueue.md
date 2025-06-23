#1 相关的数据结构
## 1.1 virtqueue
虽为交换数据的队列，但是这个数据结构并没有缓冲区buf来存放数据的成员变量。
而是通过一个priv私有数据指针指向一个缓冲区实现，比如vring_virtqueue。
也就是说vring_virtqueue只是 virtqueue 的一种具体实现方式。
vring_virtqueue的底层是vring。
~~~cpp
struct virtqueue {
    struct list_head list;
    void (*callback)(struct virtqueue *vq);
    const char *name;
    struct virtio_device *vdev;
    unsigned int index;
    unsigned int num_free;
    void *priv;
};
~~~
1）list
struct list_head list是链表节点， 将这个virtqueue 链接到virtio device的队列链表中。
list_head是双向链表，virtio device中的list是链表的头节点.
每当创建一个virtqueue的时候，就把这个virtqueue加到virtio device中的list中。
通过遍历virtqueue的list，就能找到所有的virtqueue。

2） callback
这个函数是具体驱动传入的，当设备处理完队列中的请求（如数据发送完成）时，触发此回调函数，通知驱动进行后续处理（如释放缓冲区）。

3）name 
virtqueue的名字。

4）vdev 
所属的virtio device

5）index
该virtqueue是该device中的第几个队列。

6）num_free
当前virtqueue中可用的空闲描述符数量。
若启用 VIRTIO_RING_F_INDIRECT_DESC，单个描述符可指向多个缓冲区，此时 num_free 表示剩余的直接描述符数量。

7）void *priv
指向具体实现的私有数据（如 struct vring_virtqueue），用于关联底层实现（如 vring）与抽象队列。
~~~cpp
// vring_virtqueue 初始化时设置 priv
struct vring_virtqueue *vvq = kmalloc(sizeof(*vvq), GFP_KERNEL);
vq->priv = vvq;
~~~


也就是说，vring是virtio中用于数据传输的环形缓冲区，而virtqueue是抽象层，提供队列的管理接口。vring_virtqueue则是将两者结合起来的具体实现。

可以理解为，virtqueue是一个基类（直接看成虚基类），vring_virtqueue继承了virtqueue，来作为virtqueue的一种实现，来实现其中的全部接口。只不过C语言中的继承，就是通过在结构体中直接包含来实现的。

## 1.2 vring_virtqueue

实际上，virtqueue 是vring_virtqueue的首个元素，所以，vring_virtqueue可以看成是一个virtqueue 。
这也正是两者的继承关系。
~~~cpp
struct vring_virtqueue {
    struct virtqueue vq; // 继承virtqueue 
    /*基础控制字段*/
    bool packed_ring; //标记是否为 ​Packed Virtqueue​
    bool use_dma_api;// 是否使用 DMA API 管理内存（如 dma_alloc_coherent）
    bool weak_barriers; // 是否使用弱内存屏障（针对特定架构优化性能）
    bool broken; // 标记队列是否处于故障状态（如设备未响应，需停止使用）
    bool indirect; // 是否支持 ​间接描述符​（减少可用环更新次数）
    bool event; // 是否启用 ​事件索引（Event Index）​ 机制（优化通知频率）
    /* 队列状态管理 */
    unsigned int free_head; // 空闲描述符链表的头索引（用于 Split Virtqueue）
    unsigned int num_added; // 自上次通知设备以来新增的描述符数量（用于批处理优化）
    u16 last_used_idx; // 最后观察到的 ​已用环（Used Ring）​ 索引（用于事件抑制）
    
     // 联合体。该vring_virtqueue不是 split的就是packed 的
    union {
        /*  Split Virtqueue 专用字段*/
        struct {
             struct vring vring; //Split Virtqueue 的 vring 结构
            /* Last written value to avail->flags */
            u16 avail_flags_shadow;
            /*
             * Last written value to avail->idx in
             * guest byte order.
             */
            u16 avail_idx_shadow;

            /* 监控每一个vring.desc的状态 */
            struct vring_desc_state_split *desc_state;
            /* DMA address and size information */
            dma_addr_t queue_dma_addr;
            size_t queue_size_in_bytes;
        } split;

        /*  Packed Virtqueue 专用字段 */
        struct {
            /* Actual memory layout for this queue. */
            struct {
                unsigned int num;
                struct vring_packed_desc *desc;
                struct vring_packed_desc_event *driver;
                struct vring_packed_desc_event *device;
            } vring;
            /* Driver ring wrap counter. */
            bool avail_wrap_counter;
            /* Device ring wrap counter. */
            bool used_wrap_counter;
            /* Avail used flags. */
            u16 avail_used_flags;
            /* Index of the next avail descriptor. */
            u16 next_avail_idx;
            /*
             * Last written value to driver->flags in
             * guest byte order.
             */
            u16 event_flags_shadow;
            /* Per-descriptor state. */
            struct vring_desc_state_packed *desc_state;
            struct vring_desc_extra_packed *desc_extra;
            /* DMA address and size information */
            dma_addr_t ring_dma_addr;
            dma_addr_t driver_event_dma_addr;
            dma_addr_t device_event_dma_addr;
            size_t ring_size_in_bytes;
            size_t event_size_in_bytes;
        } packed;
    };

    bool (*notify)(struct virtqueue *vq); // 通知设备的回调函数（如写入 PCI 配置空间或发送中断）
    /* DMA, allocation, and size information */
    bool we_own_ring;
};
~~~

## 1.3 vring 
~~~cpp
struct vring {
    unsigned int num;
    vring_desc_t *desc; 是数组！！！，C中链表用list
    vring_avail_t *avail;
    vring_used_t *used;


};
~~~
### 1.3.1 关于三种类型的对齐要求
~~~cpp
#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE 4
#define VRING_DESC_ALIGN_SIZE 16
typedef struct vring_desc __attribute__((aligned(VRING_DESC_ALIGN_SIZE)))
    vring_desc_t;
typedef struct vring_avail __attribute__((aligned(VRING_AVAIL_ALIGN_SIZE)))
    vring_avail_t;
typedef struct vring_used __attribute__((aligned(VRING_USED_ALIGN_SIZE)))
    vring_used_t;
~~~
通过 typedef + aligned 属性强制统一对齐方式，具体机制如下。

​为何使用 typedef 而非直接修饰结构体？
	• 若直接在结构体定义时使用 aligned 属性（如 struct __attribute__((aligned(N))) vring_desc {...};），GCC 只能增加其对齐，无法降低。若要降低对齐，必须额外使用 packed 属性，但这会移除结构体内存填充（padding），可能破坏布局。
	• 通过 typedef 对结构体重命名时使用 aligned 属性（如示例代码），GCC 允许自由调整对齐​（增或减），且不修改原结构体的内存布局，避免跨组件数据错位。
​

// 原结构体：编译器根据成员自动选择对齐（可能较高）
struct vring_desc {...}; 

// 通过typedef重新定义类型，强制对齐到 VRING_DESC_ALIGN_SIZE
// 无论原对齐高低，均被覆盖为目标值
typedef struct vring_desc __attribute__((aligned(VRING_DESC_ALIGN_SIZE))) vring_desc_t;

struct vring {
    ...
    vring_desc_t *desc;  // 指向强制对齐后的结构体类型
    vring_avail_t *avail;
    vring_used_t *used;
};
当通过 desc、avail、used 指针访问内存时，​内存分配器​（如驱动或DMA）会按 aligned 指定的对齐方式分配内存，确保不同组件按统一对齐访问数据。

关键作用：
	• ​安全跨组件传递：强制对齐确保组件间对结构体地址的理解一致，避免因对齐假设不同导致的非法内存访问。
	• ​保留布局稳定性：不改变原结构体内存布局（不用 packed），仅调整对齐，兼容性更佳。

### 1.3.2 vring_desc, vring_avail, vring_used
~~~cpp
1）vring_desc
struct vring_desc {
    __virtio64 addr;// guest的物理地址
    __virtio32 len;
    __virtio16 flags;
    __virtio16 next;// 下一个未被使用的desc的索引
};
~~~
2）vring_avail
对设备来讲的可用。
驱动将请求放在vring_avail中！！！
设备来处理这里面的请求或数据。
驱动向里面写，代表请求已经准备好，设备可以处理。
驱动通过 vring_avail ​生产请求
设备通过 vring_avail ​消费请求。
struct vring_avail {
    __virtio16 flags;
    __virtio16 idx;  // 下一个可用槽位的索引（驱动维护）
    __virtio16 ring[];// 零长数组，后面可能有n个。表示可用的desc索引！
};

3）vring_used
设备通过 vring_used ​生产结果。驱动通过 vring_used ​消费结果
设备已经将数据放到了这里面，也就是通知驱动来处理后续。
~~~cpp
struct vring_used_elem {
    __virtio32 id; // desc的索引
    /* Total length of the descriptor chain which was used (written to) */
    __virtio32 len;
};
typedef struct vring_used_elem __attribute__((aligned(VRING_USED_ALIGN_SIZE)))
    vring_used_elem_t;

struct vring_used {
    __virtio16 flags;
    __virtio16 idx;// 设备维护的，表示下一个要写入的vring_used.ring[]的位置，循环递增的
    vring_used_elem_t ring[];// 0 长数组，表示vring_desc中的已处理的的desc的索引
};
~~~
综上，vring的数据结构可以参考下图的结构：


注意的是：
1）vring中的desc指针指向的内容，是一个vring_desc数组！！数量似乎是num个，也就是说vring_desc中数组，是连续存放的，这很重要。因为vring_used和vring_avial都要进行使用index在desc中进行索引
2）vring最核心的就是这个vring_desc数组，也叫做desc数组。

2 vring_create_virtqueue
这个函数是驱动调用的最开始的接口。virtio_device的config->virtio_mmio_config_ops中会调用这个函数。
主要作用就是，创建一个virtqueue，并返回。

根据设备是否支持VIRTIO_F_RING_PACKED，来创建split或者是packed的virtqueue。
Packed virqueue是virtio1.1之后引入的，是一中性能更好的实现。
而Split的优势是兼容性好，能支持所有的传统设备。
~~~cpp
struct virtqueue *vring_create_virtqueue(
    unsigned int index,
    unsigned int num,
    unsigned int vring_align,
    struct virtio_device *vdev,
    bool weak_barriers,
    bool may_reduce_num,
    bool context,
    bool (*notify)(struct virtqueue *),
    void (*callback)(struct virtqueue *),
    const char *name)
{
    if (virtio_has_feature(vdev, VIRTIO_F_RING_PACKED))
        return vring_create_virtqueue_packed(index, num, vring_align,
                vdev, weak_barriers, may_reduce_num,
                context, notify, callback, name);
    return vring_create_virtqueue_split(index, num, vring_align,
            vdev, weak_barriers, may_reduce_num,
            context, notify, callback, name);
}
~~~
## 2.1 vring_create_virtqueue_split
~~~cpp
static struct virtqueue *vring_create_virtqueue_split(
    unsigned int index,
    unsigned int num,
    unsigned int vring_align,
    struct virtio_device *vdev,
    bool weak_barriers,
    bool may_reduce_num,
    bool context,
    bool (*notify)(struct virtqueue *),
    void (*callback)(struct virtqueue *),
    const char *name)
{
    struct virtqueue *vq;
    void *queue = NULL;
    dma_addr_t dma_addr;
    size_t queue_size_in_bytes;
    struct vring vring;
    /* We assume num is a power of 2. */
         // num是设备支持的单个virtqueue最大的描述符数量
    if (num & (num - 1)) {
        dev_warn(&vdev->dev, "Bad virtqueue length %u\n", num);
        return NULL;
    }

         /* 尝试分配空间，大小为num个描述符空间的大小 */
         //  也就是在分配vring的空间，上面说过，vring最核心的就是这个数组
         // num是传入的设备支持的最大desc数量，但是如果没有这么大的内存，这里会进行
         // num的缩小，来换取virtqueue的成功初始化
        // 分配的空间=viring的空间=vring_desc数组大小+vring_used+vring_avail
        // 空间占大头的是vring_desc数组
    for (; num && vring_size(num, vring_align) > PAGE_SIZE; num /= 2) {
                    // 若分配不到连续的物理内存块，就会返回NULL
        queue = vring_alloc_queue(vdev, vring_size(num, vring_align),
                      &dma_addr,
                      GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
        if (queue)
            break;
    }
    if (!num)
        return NULL;
    if (!queue) {
        /* 没有办法了，最后再尝试以当前已经很小了的num再进行分配，已经小于单个page了 */
        queue = vring_alloc_queue(vdev, vring_size(num, vring_align),
                      &dma_addr, GFP_KERNEL|__GFP_ZERO);
    }
    if (!queue)
        return NULL;
          // 基于当前可能缩减过的num，重新计算size
    queue_size_in_bytes = vring_size(num, vring_align);
    vring_init(&vring, num, queue, vring_align);
        // 注意这里的返回值，创建了一个vring_virtqueue,但是返回的是里面virtqueue成员的地址！
        // virtqueue成员是vring_virtqueue的第一个成员
    vq = __vring_new_virtqueue(index, vring, vdev, weak_barriers, context,
                   notify, callback, name);
    if (!vq) {
        vring_free_queue(vdev, queue_size_in_bytes, queue,
                 dma_addr);
        return NULL;
    }
    to_vvq(vq)->split.queue_dma_addr = dma_addr;
    to_vvq(vq)->split.queue_size_in_bytes = queue_size_in_bytes;
    to_vvq(vq)->we_own_ring = true;
          //  
    return vq;
}
~~~
### 2.2.1 vring_size(num, vring_align)
~~~cpp
static __inline__ unsigned vring_size(unsigned int num, unsigned long align)
{
    return ((sizeof(struct vring_desc) * num + sizeof(__virtio16) * (3 + num)
         + align - 1) & ~(align - 1))
        + sizeof(__virtio16) * 3 + sizeof(struct vring_used_elem) * num;
}
~~~
分为两部分：
	• sizeof(struct vring_desc) * num：描述符表的大小，即vring_desc数组的大小
	• sizeof(__virtio16) * (3 + num) ：可用环大小，因为vring_avail中有个0长数组，所以按num分配
	• align - 1：对齐填充
	• sizeof(__virtio16) * 3 + sizeof(struct vring_used_elem) * num ：vring_used大小，因为有个0长数组。

结合上面vring的结构示意图来理解即可。


### 2.2.2 vring_init
在分配完空间后，调用vring_init来初始化vring。
p是上面分配好的空间的首地址。
通过这个函数，将p指向的空间，分为vring_desc数组+vring_avail+vring_used.
并将这块空间的三个相应的地址，赋给vring的成员变量。
~~~cpp
static __inline__ void vring_init(struct vring *vr, unsigned int num, void *p,
                  unsigned long align)
{
    vr->num = num;
    vr->desc = p;
    vr->avail = (struct vring_avail *)((char *)p + num * sizeof(struct vring_desc));
    vr->used = (void *)(((uintptr_t)&vr->avail->ring[num] + sizeof(__virtio16)
        + align-1) & ~(align - 1));
}
~~~


### 2.2.3 __vring_new_virtqueue
也就是创建一个新的vring_virtqueue，并将上面创建好的vring赋给其相关的成员变量。
返回将创建好的vring_virtqueue, 但是是以virtqueue的形式返回的，都一样。基类子类的问题而已。
~~~cpp
/* Only available for split ring */
struct virtqueue *__vring_new_virtqueue(unsigned int index,
                    struct vring vring,
                    struct virtio_device *vdev,
                    bool weak_barriers,
                    bool context,
                    bool (*notify)(struct virtqueue *),
                    void (*callback)(struct virtqueue *),
                    const char *name)
{
    unsigned int i;
    struct vring_virtqueue *vq;
    if (virtio_has_feature(vdev, VIRTIO_F_RING_PACKED))
        return NULL;
    vq = kmalloc(sizeof(*vq), GFP_KERNEL);
    if (!vq)
        return NULL;
    vq->packed_ring = false;// 
    vq->vq.callback = callback; // 设备处理完请求后，触发。设备触发
    vq->vq.vdev = vdev;
    vq->vq.name = name;
    vq->vq.num_free = vring.num;
    vq->vq.index = index;

    vq->we_own_ring = false;
    vq->notify = notify; // 驱动提交新请求到队列后，调用驱动调用
    vq->weak_barriers = weak_barriers;
    vq->broken = false;
    vq->last_used_idx = 0;
    vq->num_added = 0;
    vq->use_dma_api = vring_use_dma_api(vdev);
   // 把这个virtqueue链到virtio device的vqs成员中
    list_add_tail(&vq->vq.list, &vdev->vqs);
#ifdef DEBUG
    vq->in_use = false;
    vq->last_add_time_valid = false;
#endif
    vq->indirect = virtio_has_feature(vdev, VIRTIO_RING_F_INDIRECT_DESC) &&
        !context;
    vq->event = virtio_has_feature(vdev, VIRTIO_RING_F_EVENT_IDX);
    if (virtio_has_feature(vdev, VIRTIO_F_ORDER_PLATFORM))
        vq->weak_barriers = false;

    //  设置vring_virtqueue中的split
    vq->split.queue_dma_addr = 0;
    vq->split.queue_size_in_bytes = 0;
    vq->split.vring = vring;
    vq->split.avail_flags_shadow = 0;
    vq->split.avail_idx_shadow = 0;
    /* No callback?  Tell other side not to bother us. */
    if (!callback) {
        vq->split.avail_flags_shadow |= VRING_AVAIL_F_NO_INTERRUPT;
        if (!vq->event)
            vq->split.vring.avail->flags = cpu_to_virtio16(vdev,
                    vq->split.avail_flags_shadow);
    }
    // 分配一块连续的位置，存放split.desc_state
    vq->split.desc_state = kmalloc_array(vring.num, sizeof(struct vring_desc_state_split), GFP_KERNEL);
    if (!vq->split.desc_state) {
        kfree(vq);
        return NULL;
    }
    /* Put everything in free lists. */
    // 空闲链表的第一个索引，初始为0，因为所有的desc都是空闲的
    vq->free_head = 0;
     // vring中的desc.next代表的是下一个未被使用的描述符的索引。
   // 这里初始化为：所有的desc都未被使用
    for (i = 0; i < vring.num-1; i++)
        vq->split.vring.desc[i].next = cpu_to_virtio16(vdev, i + 1);
   // 状态全部清零
    memset(vq->split.desc_state, 0, vring.num * sizeof(struct vring_desc_state_split));
    return &vq->vq;
}
EXPORT_SYMBOL_GPL(__vring_new_virtqueue);
~~~
## 2.2 vring_create_virtqueue_packed
packed类型的传输，放在以后整理。



# 3 virtqueue_add函数
这个函数随然不是向驱动暴露的接口，但是驱动经常使用的下面函数，都会简介调用到virtqueue_add
	• virtqueue_add_sgs
	• virtqueue_add_outbuf
	• virtqueue_add_inbuf
	• virtqueue_add_inbuf_ctx

定义：

static inline int virtqueue_add(struct virtqueue *_vq,
                struct scatterlist *sgs[],
                unsigned int total_sg,
                unsigned int out_sgs,
                unsigned int in_sgs,
                void *data,
                void *ctx,
                gfp_t gfp)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    return vq->packed_ring ? virtqueue_add_packed(_vq, sgs, total_sg,
                    out_sgs, in_sgs, data, ctx, gfp) :
                 virtqueue_add_split(_vq, sgs, total_sg,
                    out_sgs, in_sgs, data, ctx, gfp);
}

只看split风格的传输类型。
~~~cpp
static inline int virtqueue_add_split(struct virtqueue *_vq,
                      struct scatterlist *sgs[], // sg[i]是一个链表
                      unsigned int total_sg,
                      unsigned int out_sgs,
                      unsigned int in_sgs,
                      void *data,
                      void *ctx,
                      gfp_t gfp)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    struct scatterlist *sg;
    struct vring_desc *desc;
    unsigned int i, n, avail, descs_used, uninitialized_var(prev), err_idx;
    int head;
    bool indirect;
    START_USE(vq);
    BUG_ON(data == NULL);
    BUG_ON(ctx && vq->indirect);
    if (unlikely(vq->broken)) {
        END_USE(vq);
        return -EIO;
    }
    LAST_ADD_TIME_UPDATE(vq);
    BUG_ON(total_sg == 0);
    head = vq->free_head;
          // 简单情况下，不使用indirect
    if (virtqueue_use_indirect(_vq, total_sg))
        desc = alloc_indirect_split(_vq, total_sg, gfp);
    else {
        desc = NULL;
        WARN_ON_ONCE(total_sg > vq->split.vring.num && !vq->indirect);
    }
    if (desc) {
        /* Use a single buffer which doesn't continue */
        indirect = true;
        /* Set up rest to use this indirect table. */
        i = 0;
        descs_used = 1;
    } else {
        indirect = false;
        desc = vq->split.vring.desc;
        i = head; //指向第一个未使用的desc的索引
        descs_used = total_sg;
    }
        // 当前空闲描述符数量，不足以处理当前请求所需的描述符数
    if (vq->vq.num_free < descs_used) {
        pr_debug("Can't add buf len %i - avail = %i\n",
             descs_used, vq->vq.num_free);
        /* FIXME: for historical reasons, we force a notify here if
         * there are outgoing parts to the buffer.  Presumably the
         * host should service the ring ASAP. */
                   // 强制通知设备，目的是让设备尽快处理队列中的已有请求，释放占用的描述符
        if (out_sgs)
            vq->notify(&vq->vq);
        if (indirect)
            kfree(desc);
        END_USE(vq);
        return -ENOSPC;
    }
         // 前out_sgs个的sgs[n]都是 out类型的
    for (n = 0; n < out_sgs; n++) {
                   //  每一个sgs[i]都是一个链表，描述一段物理上不连续但逻辑上连续的内存区域。
              // 所以这是在遍历链表，得到一个个的scatterlist，也就一个个的小块物理内存
        for (sg = sgs[n]; sg; sg = sg_next(sg)) {
                             // 我们不使用dma，所以这里返回的是该scatterlist的物理地址
            dma_addr_t addr = vring_map_one_sg(vq, sg, DMA_TO_DEVICE);
            if (vring_mapping_error(vq, addr))
                goto unmap_release;
                         // 设置空闲的desc，填充信息，占用该desc
            desc[i].flags = cpu_to_virtio16(_vq->vdev, VRING_DESC_F_NEXT);// 该flag表示desc链未结束
            desc[i].addr = cpu_to_virtio64(_vq->vdev, addr);
            desc[i].len = cpu_to_virtio32(_vq->vdev, sg->length);
            prev = i;
                             // 找到下一个空闲的desc
            i = virtio16_to_cpu(_vq->vdev, desc[i].next);
        }
    }
          // sgs[]后面的部分是in 类型的
    for (; n < (out_sgs + in_sgs); n++) {
                  
        for (sg = sgs[n]; sg; sg = sg_next(sg)) {
            dma_addr_t addr = vring_map_one_sg(vq, sg, DMA_FROM_DEVICE);
            if (vring_mapping_error(vq, addr))
                goto unmap_release;
            desc[i].flags = cpu_to_virtio16(_vq->vdev, VRING_DESC_F_NEXT | VRING_DESC_F_WRITE);
            desc[i].addr = cpu_to_virtio64(_vq->vdev, addr);
            desc[i].len = cpu_to_virtio32(_vq->vdev, sg->length);
            prev = i;
            i = virtio16_to_cpu(_vq->vdev, desc[i].next);
        }
    }
    /* Last one doesn't continue. */
         // 此时i还是下一个空闲的desc索引，prev已经是最终的desc索引了，对该desc设置终止标志。
    desc[prev].flags &= cpu_to_virtio16(_vq->vdev, ~VRING_DESC_F_NEXT);

        // 不适用indriect，不走这个
    if (indirect) {
        /* Now that the indirect table is filled in, map it. */
        dma_addr_t addr = vring_map_single(
            vq, desc, total_sg * sizeof(struct vring_desc),
            DMA_TO_DEVICE);
        if (vring_mapping_error(vq, addr))
            goto unmap_release;
        vq->split.vring.desc[head].flags = cpu_to_virtio16(_vq->vdev,
                VRING_DESC_F_INDIRECT);
        vq->split.vring.desc[head].addr = cpu_to_virtio64(_vq->vdev,
                addr);
        vq->split.vring.desc[head].len = cpu_to_virtio32(_vq->vdev,
                total_sg * sizeof(struct vring_desc));
    }

    /* We're using some buffers from the free list. */
         // 我们已经用了一部分desc了
    vq->vq.num_free -= descs_used;

    /* Update free pointer */
    if (indirect)
        vq->free_head = virtio16_to_cpu(_vq->vdev,
                    vq->split.vring.desc[head].next);
    else
        vq->free_head = i;

         // head指向的是存放sg[]的第一个desc的索引
       // 将desc_state[head].data设置为驱动传入的data
    vq->split.desc_state[head].data = data;

    if (indirect)
        vq->split.desc_state[head].indir_desc = desc;
    else
        vq->split.desc_state[head].indir_desc = ctx;

    /* Put entry in available array (but don't update avail->idx until they
     * do sync). */
         // avail_idx_shadow维护的是split.vring.avail中的数组中的索引，初始时是0
    avail = vq->split.avail_idx_shadow & (vq->split.vring.num - 1);// 看作取模，来形成环
       // head：驱动向desc填充完sg[]后的头，见上
       // 将整个的scatterlist（多个块）逐一放到desc之后，将这个desc的起始索引赋给avail->ring[x]
      // vring 中的 avail->ring数组是一个零长数组，记录了每个请求信息在desc中的索引信息
    vq->split.vring.avail->ring[avail] = cpu_to_virtio16(_vq->vdev, head);

    /* Descriptors and available array need to be set before we expose the
     * new available array entries. */
    virtio_wmb(vq->weak_barriers);
    vq->split.avail_idx_shadow++;
    vq->split.vring.avail->idx = cpu_to_virtio16(_vq->vdev,
                        vq->split.avail_idx_shadow);
        //  自上次通知设备以来新增的描述符数量（用于批处理优化）
    vq->num_added++;
    pr_debug("Added buffer head %i to %p\n", head, vq);
    END_USE(vq);
    /* This is very unlikely, but theoretically possible.  Kick
     * just in case. */
    if (unlikely(vq->num_added == (1 << 16) - 1))
        virtqueue_kick(_vq);
    return 0;
 // 错误处理，不走下面
unmap_release:
    err_idx = i;
    i = head;
    for (n = 0; n < total_sg; n++) {
        if (i == err_idx)
            break;
        vring_unmap_one_split(vq, &desc[i]);
        i = virtio16_to_cpu(_vq->vdev, vq->split.vring.desc[i].next);
    }
    if (indirect)
        kfree(desc);
    END_USE(vq);
    return -EIO;
}
~~~

需要注意的下面几点：
	• 传入参数struct scatterlist *sgs[]是一个数组链表，每一个数组元素指向一个struct scatterlist链表。这个链表描述一段物理上不连续但逻辑上连续的内存区域。也就是传入参数是多段物理上不连续但逻辑上连续的内存区域。这多段信息，看成是一条消息。
	• vring中的vring.avail存放的是对设备来讲的可用内容，也就是驱动往virtqueue中写的东西！
	• 这个函数是驱动调用的，scatterlist信息由驱动准备好。调用一次这个函数只会在vring->avial->vring数组中添加一条信息。虽然占用了多个desc，但是这是一个整体，看成往virtqueue中添加了一条信息。
	• scatterlist *sgs[]数组链表，代表一条信息，而不是一个链表代表一个。

## 3.1 virtqueue_add_sgs

主要就是计算出total_sg，然后调用virtqueue_add。
total_sg就是scatterlist *sgs[]代表的信息，所有物理块数量，也就是将会占用的vring的desc的数量。
也就是这条信息的所有物理块数量。
~~~cpp
int virtqueue_add_sgs(struct virtqueue *_vq,
              struct scatterlist *sgs[],
              unsigned int out_sgs,
              unsigned int in_sgs,
              void *data,
              gfp_t gfp)
{
    unsigned int i, total_sg = 0;
    /* Count them first. */
    for (i = 0; i < out_sgs + in_sgs; i++) {
        struct scatterlist *sg;
        for (sg = sgs[i]; sg; sg = sg_next(sg))
            total_sg++;
    }
    return virtqueue_add(_vq, sgs, total_sg, out_sgs, in_sgs, data, NULL, gfp);
}
~~~
## 3.2 virtqueue_add_outbuf
只向virtqueue中添加outbuf，也就是inbuf是0。以此参数来调用virtqueue_add。
传入参数是一条scatterlist链表，这符合逻辑。
int virtqueue_add_outbuf(struct virtqueue *vq,
             struct scatterlist *sg, unsigned int num,
             void *data,
             gfp_t gfp)
{
    return virtqueue_add(vq, &sg, num, 1, 0, data, NULL, gfp);
}

## 3.3 virtqueue_add_inbuf
与virtqueue_add_outbuf同理。
int virtqueue_add_inbuf(struct virtqueue *vq,
            struct scatterlist *sg, unsigned int num,
            void *data,
            gfp_t gfp)
{
    return virtqueue_add(vq, &sg, num, 0, 1, data, NULL, gfp);
}


# 4 virtqueue_kick
这也是向驱动暴露的一个接口。用于通知设备。
更新可用环索引并通知设备有新请求可处理。
bool virtqueue_kick(struct virtqueue *vq)
{
    if (virtqueue_kick_prepare(vq))
        return virtqueue_notify(vq);
    return true;
}

bool virtqueue_kick_prepare(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    return vq->packed_ring ? virtqueue_kick_prepare_packed(_vq) :
                 virtqueue_kick_prepare_split(_vq);
}


所以主要是两个重要函数：
	• virtqueue_kick_prepare_split
	• virtqueue_notify

## 4.1 virtqueue_kick_prepare_split

用于判断是否需要通知设备。

static bool virtqueue_kick_prepare_split(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    u16 new, old;
    bool needs_kick;
    START_USE(vq);
    /* We need to expose available array entries before checking avail
     * event. */
    virtio_mb(vq->weak_barriers);
        // 上次通知设备时的可用环索引, 这次通知多了num_added个
    old = vq->split.avail_idx_shadow - vq->num_added;
       // 当前avail的索引位置
    new = vq->split.avail_idx_shadow;
         // 重置请求计数
    vq->num_added = 0;
    LAST_ADD_TIME_CHECK(vq);
    LAST_ADD_TIME_INVALID(vq);
          // virtqueue中的event代表了是否启用事件索引（用于优化通知频率）
    if (vq->event) {          
        needs_kick = vring_need_event(virtio16_to_cpu(_vq->vdev,
                    vring_avail_event(&vq->split.vring)),
                          new, old);
    } else {
        needs_kick = !(vq->split.vring.used->flags &
                    cpu_to_virtio16(_vq->vdev,
                        VRING_USED_F_NO_NOTIFY));
    }
    END_USE(vq);
    return needs_kick;
}

1）vring_avail_event和vring_used_event
这两个宏需要注意，用于访问vring的事件索引​（Event Index）。
当设备支持VIRTIO_F_EVENT_IDX的时候，所创建的virtqueue中的 bool event字段就会设置为true，代表启用事件索引（Event Index）​ 机制来优化通知频率。
驱动和设备通过事件索引协商通知条件，避免频繁的中断，例如：
	• ​Used Event Index：设备通过此索引告知驱动“当可用环（Avail Ring）的索引超过该值时，需要通知我”。
	• ​Avail Event Index：驱动通过此索引告知设备“当已用环（Used Ring）的索引超过该值时，需要通知我”。

为了兼容旧版 VirtIO 实现，事件索引被放置在可用环和已用环的末尾​（超出原始数组范围），通过指针操作直接访问。 也就是说通过访问vring_avail或者vring_used的“越界”位置实现事件索引的读写.


也就是说，在我们上面vring中的vring_avail的定义应修改为：
struct vring_avail {
    __virtio16 flags;
    __virtio16 idx;  // 下一个可用槽位的索引（驱动维护）
    __virtio16 ring[];// 零长数组，后面可能有n个。表示可用的desc索引！
       __virtio16 used_event_idx; // 支持事件索引机制
};

struct vring_used {
    __virtio16 flags;
    __virtio16 idx;
    vring_used_elem_t ring[];// 0 长数组，表示vring_desc中的已用的desc的索引
       __virtio16 avail_event_idx; // 支持事件索引机制
};


所以vring_used_event的作用为：
	• 从 ​可vring_avail的末尾 获取 ​Used Event Index​（设备期望驱动通知的索引）。

vring_avail_event的作用为：
	• 从 ​vring_used的末尾 获取 ​Avail Event Index​（驱动期望设备通知的索引）


2）vring_need_event

static __inline__ int vring_need_event(__u16 event_idx, __u16 new_idx, __u16 old)
{
        return (__u16)(new_idx - event_idx - 1) < (__u16)(new_idx - old);
}
用于判断在索引从 old 更新到 new_idx 时，是否需要触发事件通知对方（设备或驱动）。
该条件等价于判断：​**从 old 到 new_idx 的索引更新过程中，是否跨越了 event_idx**​（考虑无符号回绕）。

数学原理
​未发生回绕（new_idx > old）​：若 event_idx 在区间 [old, new_idx) 内，触发事件。
	• ​示例：old=3, new_idx=6, event_idx=5 → 触发。
​发生回绕（new_idx < old）​：若 event_idx 在区间 [old, 0xFFFF] 或 [0, new_idx) 内，触发事件。
	• ​示例：old=6, new_idx=1（回绕后），event_idx=7 → 触发。



## 4.2 virtqueue_notify
~~~cpp
bool virtqueue_notify(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    if (unlikely(vq->broken))
        return false;
    /* Prod other side to tell it about changes. */
    if (!vq->notify(_vq)) {
        vq->broken = true;
        return false;
    }
    return true;
}
~~~
最终是调用了virtqueue的notify函数。
virtqueue的notify函数，是在创建virtqueue的时候，也就是在上面分析的__vring_new_virtqueue函数中注册的。
不同的virtqueue的创建方式，会决定不同的notify方式，这也就是必须使用回调的原因。（根据调用者的不同，有不同的实现方式）。

在基于MMIO的设备中，在virtio_mmio的代码中，可以看到，使用的notify函数为：
vq = vring_create_virtqueue(index, num, VIRTIO_MMIO_VRING_ALIGN, vdev,
                 true, true, ctx, vm_notify, callback, name);

static bool vm_notify(struct virtqueue *vq)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vq->vdev);
    /* We write the queue's selector into the notification register to
     * signal the other end */
    writel(vq->index, vm_dev->base + VIRTIO_MMIO_QUEUE_NOTIFY);
    return true;
}

也就是向特定的寄存器中写特定的消息，完成通知。

# 5 virtqueue_get_buf

void *virtqueue_get_buf(struct virtqueue *_vq, unsigned int *len)
{
    return virtqueue_get_buf_ctx(_vq, len, NULL);
}


void *virtqueue_get_buf_ctx(struct virtqueue *_vq, unsigned int *len,
                void **ctx)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    return vq->packed_ring ? virtqueue_get_buf_ctx_packed(_vq, len, ctx) :
                 virtqueue_get_buf_ctx_split(_vq, len, ctx);
}


最终调用的函数是virtqueue_get_buf_ctx_split：
~~~cpp
static void *virtqueue_get_buf_ctx_split(struct virtqueue *_vq,
                     unsigned int *len,
                     void **ctx)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    void *ret;
    unsigned int i;
    u16 last_used;
    START_USE(vq);
    if (unlikely(vq->broken)) {
        END_USE(vq);
        return NULL;
    }
          // 设备是否有响应或向vring中写东西。
    if (!more_used_split(vq)) {
        pr_debug("No more buffers in queue\n");
        END_USE(vq);
        return NULL;
    }
    /* Only get used array entries after they have been exposed by host. */
    virtio_rmb(vq->weak_barriers);

          // 实现回绕
    last_used = (vq->last_used_idx & (vq->split.vring.num - 1));
         // last_used就是当前要处理的vring_used.ring[]索引的位置。(会递增)
      // 返回值i是desc中的一个索引
    i = virtio32_to_cpu(_vq->vdev,
            vq->split.vring.used->ring[last_used].id);

    *len = virtio32_to_cpu(_vq->vdev,
            vq->split.vring.used->ring[last_used].len);

    if (unlikely(i >= vq->split.vring.num)) {
        BAD_RING(vq, "id %u out of range\n", i);
        return NULL;
    }
          //  验证该desc[i]是否为链头（链头的data非空）
    if (unlikely(!vq->split.desc_state[i].data)) {
        BAD_RING(vq, "id %u is not a head!\n", i);
        return NULL;
    }
    /* detach_buf_split clears data, so grab it now. */
    ret = vq->split.desc_state[i].data;

         // 回收描述符链到空闲链表
    detach_buf_split(vq, i, ctx);
         // 递增到下一个将要处理的vring_used.ring的索引处
    vq->last_used_idx++;


    /* If we expect an interrupt for the next entry, tell host
     * by writing event index and flush out the write before
     * the read in the next get_buf call. */
          // 将驱动维护的 last_used_idx写入used_ring的事件索引位置处。
       // 表示当设备处理到 last_used_idx 对应的条目时，再触发中断
    if (!(vq->split.avail_flags_shadow & VRING_AVAIL_F_NO_INTERRUPT))
        virtio_store_mb(vq->weak_barriers,
                &vring_used_event(&vq->split.vring),
                cpu_to_virtio16(_vq->vdev, vq->last_used_idx));
    LAST_ADD_TIME_INVALID(vq);
    END_USE(vq);
    return ret;
}
~~~
需要注意的是，这个函数一次只能处理一个设备请求（last_used_idx递增一次），并没有全部处理完vring_used.vring[]中的全部请求。
要想批量实现处理，就要循环调用这个函数，当more_used_split失败时，就代表处理完了所有的请求。

## 5.1 more_used_split

static inline bool more_used_split(const struct vring_virtqueue *vq)
{
        //  last_used_idx是驱动维护的，初始值是0，记录当前要处理的最后一个vring_used->ring的索引位置
       //   vring.used->idx是设备维护的索引，代表写入的vringt_used->ring的最新位置。
    return vq->last_used_idx != virtio16_to_cpu(vq->vq.vdev,
            vq->split.vring.used->idx);
}

过轻量级的索引比较，高效检测Virtqueue 的新请求。
也就是检测是否有vring_used中是否有设备向里面写入内容。
也就是检测是否设备有响应。


回顾vring中的vring_used:
struct vring_used_elem {
    __virtio32 id; // desc的索引
    /* Total length of the descriptor chain which was used (written to) */
    __virtio32 len;
};
typedef struct vring_used_elem __attribute__((aligned(VRING_USED_ALIGN_SIZE)))
    vring_used_elem_t;

struct vring_used {
    __virtio16 flags;
    __virtio16 idx;// 设备维护的，表示下一个要写入的vring_used.ring[]的位置，循环递增的
    vring_used_elem_t ring[];// 0 长数组，表示vring_desc中的已处理的的desc的索引
};
当驱动提交请求的时候，会构建desc链，更新vring的Avail Ring，调用vritqueue_kick通知设备。
设备读取avail ring，处理请求然后写入used ring，也就是更新vring_used中的idx和ring[]。




![image](https://github.com/user-attachments/assets/9a68859b-a76e-4cef-a56b-90eb33efba72)
