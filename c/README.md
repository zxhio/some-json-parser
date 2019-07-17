
### 最初的 C 版本

核心使用 `侵入式单链表` 将不同类型的 Json **value** 节点连接起来, 每一个节点都有一个共同的 `j4on_value`, 用于保存当前节点的类型和链表

``` c
struct j4on_value {
    value_type j4_type;
    struct slist j4_list;
};
```
通过 type 和 链表的地址，使用 `container_of` 获取各类 value 节点的首地址

*C版本的设计的很糟糕，仅仅是能够解析，但是扩展起来非常麻烦，只能说当作玩具来使用吧，熟悉了侵入式链表的使用*