# msptiActivity<a name="ZH-CN_TOPIC_0000002048904921"></a>

msptiActivity为Activity Record的基础结构体，Activity API使用msptiActivity作为Activity的通用表示，**kind**字段用于确定特定的Activity类型，由此可以将msptiActivity对象转换为适合该类型的特定的Activity Record类型。

定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
	msptiActivityKind kind;   // Activity类型
} msptiActivity;
```

