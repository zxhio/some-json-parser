### 一个轻量级的Json解析器

#### 使用
make & ./j4on

解析 json 文件，将内容生成一棵json树存在内部
```cpp
j4on::J4onParser parser("./json/object.json");
parser.parse();  // 解析

parser.format(); // 遍历并且格式化该 json 文本.

// 获取 key 对应的 value.
j4on::Value v = parser.getValue("latex-workshop.view.pdf.viewer");
std::cout << j4on::typeToString(v.type()) << std::endl;
j4on::String str = std::any_cast<j4on::String>(v.getAnyValue());
```



#### 设计
- 对于每种 value 构造一个类
- 构造一个通用 Value，包含类型和一个 `std::any` 对象
- **模块化** 解析步骤，隔离每个模块的干扰，在解析和遍历（格式化）都体现很高的便利性

一些要注意的是，一个 value 必须包含对象实体，因为并不清楚这个这个value是根节点还是复合结构的叶节点。
由于包含实体对象的原因，在解析的时候会有内存的返回申请和析构，这个是一个很影响性能的地方，应该可以使用自建内存管理解决。
object 对象使用线性数组是为了保持JSON原有的结构。

