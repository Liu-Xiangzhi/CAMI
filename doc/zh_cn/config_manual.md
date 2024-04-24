# 配置手册
CAMI 配置文件格式及默认值如下
```toml
[cami]
disable_compiler_guarantee_check = false
enable_auto_cache = true
cache_path = "#/tmp/"
enable_log_file = false
log_file_path = "#~/.cami/log.txt"

[cami.log]
time_format = "#%Hh%Mm%Ss"
level = "@log_level_debug"
[cami.log.color]
fatal = "@magenta"
error = "@red"
warning = "@yellow"
info = "@green"
verbose = "@blue"
debug = "@cyan"

[cami.object_manage]
eden_size = "16_M"
old_generation_size = "1_G"
large_object_threshold = "1_M"
promote_threshold = 8

[cami.memory]
heap.page_size = "16_K"
heap.page_table_level = 4
heap.allocator = "cami::am::SimpleAllocator"
mmio.max_file = "1_K"

[cami.file_system]
root = "#~/.cami/"
```
## 转换规则
配置文件中每一个键值对都会（值为 false 的除外）转换为一条宏定义，其中宏名为键名全大写且将`.`替换为`_`后结果，宏替换列表遵循如下规则
+ 若值类型为 bool
+ + 值为 true, 宏替换列表为空
+ + 值为 false, 无当前宏定义
+ 若值类型不为 string, 宏替换列表为该值转换为 string 的结果
+ 若值类型为 string
+ + 若第一个字符为 `#` 宏替换列表为用双引号引起来的该值的[1, n)的子串，其中 n 代表该值的长度
+ + 若第一个字符为 `@` 宏替换列表为以该值的[1, n)的子串为 key 对应的 value（对应关系见后文），其中 n 代表该值的长度
+ + 否则，宏替换列表即为该值

配置文件中可使用的参数映射表如下：
|键|值|
|-----|----------|
|black|"\033[30m"|
|red|"\033[31m"|
|green|"\033[32m"|
|yellow|"\033[33m"|
|blue|"\033[34m"|
|magenta|"\033[35m"|
|cyan|"\033[36m"|
|white|"\033[37m"|
|log_level_fatal|0|
|log_level_error|1|
|log_level_warning|2|
|log_level_info|3|
|log_level_verbose|4|
|log_level_debug|5|

示例：

如下配置（仅演示转换规则，不考虑语义）
```
a = true
b.c = false
[d]
e.f = 2
[g.h]
i = "aaa"
j = "#xxx"
i = "@black"
```
将被转换为
``` c
#define A
#define D_E_F 2
#define G_H_I aaa
#define G_H_I "xxx"
#define G_H_I "\\033[30m"
```

## 配置项含义
下表中：
+ 以`*`标记的配置项代表其暂未被使用，用户设置任意值
+ 以`#`标记的配置项需保证转换后的结果为（C语言含义上的）字符串，未被标记的转换后不应为字符串。
+ 配置数量大小的配置项可以使用字符串的形式（如`"1234"`），且可以添加单位`_k`、`_m`、`_g`、`_K`、`_M`、`_G`（如`"2_k"`），分别代表乘以 1024、1048576、1073741824、1024、1048576、1073741824。
+ 对于后文部分名称的定义，查看[内部实现](./internals.md)

|配置项|类型|含义|
|-----|----|---|
|cami.disable_compiler_guarantee_check|bool|CAMI 会对部分|
|cami.enable_auto_cache*|bool|是否缓存文本形式字节码对应的二进制结果，若是，将结果存放到cami.cache_path指定的路径下|
|cami.cache_path*#| string ||
|cami.enable_log_file | bool |是否允许 CAMI 将日志写入文件中，若否，将打印至终端|
|cami.log_file_path#| string |CAMI 日志文件存放路径|
|cami.log.time_format#| string |需要输出时间时，时间的格式|
|cami.log.level | int or string|日志等级，只有等级大于等于该值的日志内容才会被输出|
|cami.log.color.fatal#| string |fatal 等级的日志打印颜色|
|cami.log.color.error#| string |error 等级的日志打印颜色|
|cami.log.color.warning#| string|warning 等级的日志打印颜色|
|cami.log.color.info#| string|info 等级的日志打印颜色|
|cami.log.color.verbose#| string|verbose 等级的日志打印颜色|
|cami.log.color.debug#| string|debug 等级的日志打印颜色|
|cami.object_manage.eden_size | int or string |eden 区域大小|
|cami.object_manage.old_generation_size | int or string |老年代最大大小，运行时所需要的大小超过该值时会触发内存溢出|
|cami.object_manage.large_object_threshold | int or string|大对象门限，大小大于该值的对象将直接分配在老年代区域|
|cami.object_manage.promote_threshold | int or string|对象提升门限，年龄大于该值的对象将会从年轻代提升至老年代|
|cami.memory.heap.page_size | int or string|堆内存页表的大小|
|cami.memory.heap.page_table_level | int or string|堆内存页表的层级|
|cami.memory.heap.allocator | string |堆内存分配器|
|cami.memory.mmio.max_file | int or string|一个 CAMI 进程最多可打开的文件数量|
|cami.file_system.root#| string |CAMI 进程的文件系统根目录|
