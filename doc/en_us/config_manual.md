# Manual for configuration
The format and default value of CAMI cofiguration file are as follow:
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
## Transformation Rule
Each key-value pair(K V) in the configuration file (excluding those with a value of false) will be transformed into a macro definition. The macro name is formed by converting the key name to uppercase and replacing '.' with '_', and the macro replacement list is according to the following rules:
+ if the type of V is `bool`
+ + if V is `true`, the macro replacement list is empty
+ + otherwise, the macro should not defined
+ if the type of V is not `string`, the macro replacement list should be the result that converting V to string
+ if the type of V is `string`
+ + if the first character is `#`, the macro replacement list consists of the substring \[1, n) of V enclosed in double quotes, where n represents the length of V
+ + if the first character is `@`, the macro replacement list consists of the value of parameter map(specified later) using the substring \[1, n) of V as key, where n represents the length of V.
+ + otherwise, the macro replacement list is V

The parameter map is as following:
|key|value|
|-----|----------|
|black|"\\\\033[30m"|
|red|"\\\\033[31m"|
|green|"\\\\033[32m"|
|yellow|"\\\\033[33m"|
|blue|"\\\\033[34m"|
|magenta|"\\\\033[35m"|
|cyan|"\\\\033[36m"|
|white|"\\\\033[37m"|
|log_level_fatal|0|
|log_level_error|1|
|log_level_warning|2|
|log_level_info|3|
|log_level_verbose|4|
|log_level_debug|5|

Example:

The following configuration(without considering semantic)
```toml
a = true
b.c = false
[d]
e.f = 2
[g.h]
i = "aaa"
j = "#xxx"
i = "@black"
```
will be transformed into
``` c
#define A
#define D_E_F 2
#define G_H_I aaa
#define G_H_I "xxx"
#define G_H_I "\033[30m"
```

## Meaning of Configuration Item
In the table below:
+ Configuration items marked with `*` indicate they are not currently in use and users can set any value.
+ Configuration items marked with `#` need to ensure that the result after transformation is a string literal, while items not marked should not.
+ Configuration items representing sizes can use string format (e.g., `"1234"`) and can include units such as `_k`, `_m`, `_g`, `_K`, `_M`, `_G` (e.g., `"2_k"`), representing multiplication by 1024, 1048576, 1073741824, 1024, 1048576, 1073741824 respectively.

|configuration item|type|meaning|
|-----|----|---|
|cami.disable_compiler_guarantee_check|bool|CAMI performs relevant validity checks for each instruction at runtime. The compiler can guarantee that certain check results will always be normal. This configuration item determines whether to disable these checks|
|cami.enable_auto_cache*|bool|whether CAMI is allowed to cache text form bytecode file|
|cami.cache_path*#| string |path of cached binary form bytecode file|
|cami.enable_log_file | bool |Whether CAMI is allowed to write logs to a file. If not, they will be printed to the terminal|
|cami.log_file_path#| string |path of CAMI log file|
|cami.log.time_format#| string |the format of output time|
|cami.log.level | int or string|logging level, only logs greater than this value can be output|
|cami.log.color.fatal#| string |color of log in fatal level|
|cami.log.color.error#| string |color of log in error level|
|cami.log.color.warning#| string|color of log in warning level|
|cami.log.color.info#| string|color of log in info level|
|cami.log.color.verbose#| string|color of log in verbose level|
|cami.log.color.debug#| string|color of log in debug level|
|cami.object_manage.eden_size | int or string |size of eden region|
|cami.object_manage.old_generation_size | int or string |max size of old generation region, OOM will be triggered if more memory is needed|
|cami.object_manage.large_object_threshold | int or string|object larger than this value will be allocated to old generation|
|cami.object_manage.promote_threshold | int or string|threshold for object promotion, object older than this value will promote from young generation to old generation|
|cami.memory.heap.page_size | int or string|size of heap page table|
|cami.memory.heap.page_table_level | int or string|level of heap page table|
|cami.memory.heap.allocator | string |heap memory allocator|
|cami.memory.mmio.max_file | int or string|max number of files can be opened by one CAMI process|
|cami.file_system.root#| string |root directory of file system of CAMI process|
