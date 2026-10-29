# AC 自动机工具

启用 `PROJECT_ENABLE_TOOLS` 后构建 `ac_automation_tool`。公开库接口、匹配语义及内存和磁盘布局见
[ac_automation_v2.h](../../include/string/ac_automation_v2.h) 中的注释文档。

```text
ac_automation_tool build --input words.txt --output words.ac --nocase --skip-space --skip-punctuation
ac_automation_tool info --input words.ac
ac_automation_tool print --input words.ac
ac_automation_tool print --input words.ac --section keywords --format json --output keywords.json
ac_automation_tool print --input words.ac --section skips
ac_automation_tool print --input words.ac --section folds
ac_automation_tool build --input words.txt --output aliases.ac --replacement-file replacements.txt
ac_automation_tool print --input aliases.ac --section replacements
ac_automation_tool verify --input words.ac --semantic
ac_automation_tool visualize --input words.ac --output words.dot --format dot --max-depth 3
ac_automation_tool visualize --input words.ac --output words.mmd --format mermaid --root-id 1 --max-nodes 500
ac_automation_tool visualize --input words.ac --output words.puml --format plantuml --utf8-labels
ac_automation_tool visualize --input words.ac --output words.d2 --format d2 --max-depth 4
ac_automation_tool visualize --input words.ac --output words.noml --format nomnoml --root-id 1
ac_automation_tool visualize --input words.ac --output words.graphml --format graphml
```

参数使用 `--name value`，不支持 `--name=value`。重复参数、未知参数、缺失值、非法数字都会返回非零退出码。
输出路径必须不存在。工具在相同目录独占创建临时文件，写入并关闭后以禁止覆盖的操作发布；已有文件不被替换。
路径沿用 `file_system` 的窄字符路径约定，不额外承诺 Windows 任意 Unicode 路径支持。

输入词表为 UTF-8，每行一个词，忽略空行及两端 Unicode White_Space。只移除文件开头的 UTF-8 BOM；
`#`、逗号等没有注释或字段分隔含义。非法 UTF-8 会报告行号并停止，不留下最终输出。
词条按 fold → replacement → skip 处理后重复时保留第一条及其 keyword ID；处理后为空的词条计入 ignored。

构建选项：

| 选项 | 作用 |
| --- | --- |
| `--nocase` / `--ignore-case` | 内置单字节 ASCII A–Z 折叠；不是完整 Unicode case folding |
| `--case-fold-file FILE` | 每行两个十六进制 token，例如 `4100 6100`；要求同时指定 `--nocase` |
| `--replacement-file FILE` | 每行两个十六进制 token，源在前、目标在后；不要求 `--nocase`，允许空行 |
| `--boundary-file FILE` | 共享边界规则，每行显式指定 ID；词表左右字段引用该 ID |
| `--keyword-boundaries` | 词表使用 `左规则ID<TAB>右规则ID<TAB>词条`；0 表示该侧不限制 |
| `--skip-space` | Unicode White_Space，25 个码位 |
| `--skip-punctuation` | 保守白名单中的 P*，365 个码位 |
| `--skip-symbols` | 保守白名单中的 S*，2382 个码位 |
| `--skip-invisible` | Cc 及有限格式控制符，81 个码位；不展开整个 Default_Ignorable_Code_Point |
| `--skip-file FILE` | 去开头 BOM 后按 UTF-8 拆成 skip token；包括空格和换行，不做 trim |

## 左右边界

边界是零宽断言，检查关键词旁边的一个原始 token，先 fold/replacement，再判断集合，最后处理 skip。
左右规则独立；断言不消耗相邻字符，也不扩大命中字节范围。规则可以包含或排除任意不透明 token 集合，
不采用 Unicode 单词边界或正则 `\b` 的定义。集合成员表示最终 token 的字节值，自身不做 fold/replacement。

例如 `boundaries.txt`：

```text
10 exclude edge a-z A-Z
20 include no-edge a-z 0-9 text:中 hex:5F
30 exclude edge a-z 0-9
```

每行依次为十进制 ID、`include`/`exclude`、`edge`/`no-edge` 和零个或多个成员说明。
ID 范围为 1..4294967294，允许乱序和稀疏编号；0 保留给“不限制”。重复、越界 ID 及不存在的引用会报错。
`edge` 允许输入开头或结尾，`no-edge` 拒绝输入边缘，与成员集合独立。空 include 集只允许已启用的输入边缘；
空 exclude 集允许任意合法相邻 token。非法编码不能满足任何边界规则。
成员说明支持升序 ASCII 范围 `a-z`、`A-Z`、`0-9`，`text:中_` 按 UTF-8 拆成多个 token，
`hex:5F` 将十六进制字节整体作为一个 token；空白 token 使用 `hex:20` 等形式。规则文件忽略空行，
没有行尾注释。`a-z` 只包含小写，可显式加入 `A-Z` 或启用 `--nocase`。

工具按输入 ID 递增排序并映射到连续内部 ID，避免稀疏编号产生空洞。原始 ID 是构建输入的引用标签，
不写入存储；dump/print 使用内部 ID。例如上述 10、20、30 编译为 1、2、3。规则行重排不改变编译结果。
连续输入 ID 1..N 与内部 ID 一致。旧的不带 ID 文件仍按非空行从 1 编号，但两种格式不能在同一文件中混用。

带规则的词表中，每行三个字段用实际 Tab 分隔，例如 `0<TAB>10<TAB>sm`，然后构建：

```text
ac_automation_tool build --input words.tsv --output words.ac --boundary-file boundaries.txt --keyword-boundaries --skip-punctuation
ac_automation_tool print --input words.ac --section boundaries --format json
ac_automation_tool print --input words.ac --section keywords
```

右边界排除字母时，`smile` 不命中，`sm ile`、`wesm9`、`smé`、`sm中`、`sm_` 命中；即使 `-` 在 skip 中，
`sm-ile` 也命中。配置 `é → i` 后，`sméle` 的相邻 token 是 `i`，不命中。
设置左边界后，`wesm9` 不命中；左右都设置时要求两侧同时成立。若只允许数字作为相邻边界，使用 `include … 0-9`；
若排除字母数字，使用 `exclude … a-z A-Z 0-9`。

普通词表保持每行一词，不解释 `<boundary>` 等标记；只有显式启用 `--keyword-boundaries` 才解析字段。
相同规范化词条的左右规则 ID 必须一致，冲突会拒绝构建，避免导入顺序意外取消限制。
`print` 的关键词记录输出 `left_boundary`/`right_boundary`；`boundaries` 输出模式、输入边缘策略和完整成员。
所有可视化格式在命中节点标签中显示 `left=ID`/`right=ID`，规则详情由 `print` 查询。

库调用先 `add_boundary_rule()` 获取 ID，再将其写入 `keyword_options`，传给 `insert_keyword()` 重载。
规则必须在首次成功插入前配置；UTF-16 和自定义 tokenizer 可直接配置对应的不透明字节 token，不做转码。
查询在命中提交前验证断言，失败后继续后缀输出和后续文本；不会消耗命中限额或触发不重叠状态重置。
右侧前瞻最多缓存一个 token，同一 token 不会重复调用自定义 tokenizer；提前返回的 `bytes_consumed` 不包含前瞻字节。

边界配置写入 2.2 镜像的必需特性位和第 10–12 节；旧读取器会拒绝该镜像。
没有边界配置时继续输出原有 2.0/2.1 格式。Node/Keyword 仍为 16 字节；共享规则和成员单独保存，
每个关键词的左右规则共 8 字节，仅有边界配置时存在。精确布局及校验约束见公共头文件。

## 字符替换

替换表由业务显式提供，不内置跨语言的相似字符规则。例如文件内容：

```text
E5A3B9 E4B880
65 66
67 68
```

分别表示 UTF-8 的 `壹 → 一`、`e → f`、`g → h`。输入词表和匹配输入采用相同处理顺序：
tokenizer 拆分原始字节 → 大小写折叠 → 替换为最终 token → 跳过 canonical skip → AC 转移。
`aec`、`afc` 只建立一条 `afc` 路径；只插入 `ahc` 时也能查询 `agc`。skip 含 `b` 时，
`abc`、`ac` 共享路径，`ab` 不命中该路径。返回的 keyword 保留首个词条原文，位置和长度对应原始查询字节，
包含命中内部的 skip 字节；替换不会改变返回的字节坐标。

库接口 `set_replacements(gsl::span<const replacement_rule>)` 原子替换整张表，空 span 清空。
必须在首次成功插入前配置；之后改规则需要重新建库。规则两端是非空的不透明字节 token，配置接口不分词、
不转码；UTF-8、UTF-16 和自定义 token 可以并存。一个目标始终是一个 token，不能用该接口把一个字符展开为多个字符。
若目标包含多个 UTF-8 字符，只有调用方 tokenizer 将它们作为同一个 token 时，目标的字面输入才与之对应。
非法输入仍由查询 tokenizer/invalid-input policy 处理，不能利用替换规则跨越非法编码单元。

源和目标先经过当前 fold；替换链在配置阶段迭代展开，允许恒等规则。遇到回环时，以环内按无符号字节
字典序最小的 token 为代表，环内节点及进入环的链条均直接映射到它；进入环的链条不参与环内最小值比较。
例如 `e → f → g → e` 最终统一为 `e`，即使另有 `d → g`，最终也为 `d → e`。同源不同目标仍被拒绝，失败保留原设置。
例如 `e → F`、`f → g` 在 nocase 下合成为 `e → g`、`f → g`。skip token 也经过同样的规范化：
skip 含别名 `e` 时，其目标 `g` 及其他归一到 `g` 的别名全部跳过。修改 nocase/case-fold 时会重新验证替换表。

冻结后保留原 FoldTo 以检查大小写规则，并为替换配置增加 `CanonicalTo[S]`（每字符 4 字节）。查询仅查一次
最终映射表，不追踪链或操作 map/hash。没有替换时保持原有内存布局和 2.0 dump；有替换时写出 2.1，
增加必需 feature bit 1 和第 9 节 CanonicalTo。新版本仍读取 2.0；旧版本拒绝 2.1 替换模型，不会悄悄漏掉替换。
内存及磁盘精确布局保留在公共头文件注释中。

幂等的最终映射避免二次处理结果变化；这一约束也用于 [UTS #39](https://www.unicode.org/reports/tr39/) 的混淆检测映射。
这里采用业务提供的 token 别名，不实现其 Unicode skeleton 转换或自动导入全量 confusables。

## 内置 skip 范围

四类内置选项合计 2847 个码位（空白与控制字符存在交集），使用 `conservative-pattern-syntax-v1`：
以 `Pattern_Syntax`、`White_Space`、Cc、`Bidi_Control` 和下划线、软连字符、零宽空格、Word Joiner、BOM 为基础，
仅加入递归兼容分解的每个字符都属于基础白名单的标点/符号/空白。排除字母、组合标记、数字、部首、
表意文字及描述符、文字延长符、音调/变音符、连接控制符、变体选择符、数学兼容标识符、私用区、未分配码位，
以及分解后包含上述文字组成部分的字符。规则依据见 [UAX #31](https://www.unicode.org/reports/tr31/#Pattern_Syntax)
和 [UAX #44](https://www.unicode.org/reports/tr44/)。例如 `⼀`、`㍿`、韩文填充字、组合重音符、ZWJ/ZWNJ、
变体选择符均保留为有意义的 token；`--skip-file` 仍按调用者显式提供的字节配置，不受这份内置白名单限制。

`unicode_ranges.h` 的 `kUnicodeSkipRanges` 仅供上述 skip 选项使用；独立的 `kUnicodeDisplayRanges`
保留完整空白/不可见属性，供 trim 和输出转义使用。字符需要转义显示，并不表示匹配时应忽略它。
这些 Unicode 规则只在工具中展开，库和 dump 保存不透明 token bytes，不保存编码或 Unicode profile。
表来自 Unicode 18.0.0，许可证见 [UNICODE-LICENSE.txt](UNICODE-LICENSE.txt)。

```text
python generate_unicode_ranges.py UnicodeData.txt DerivedCoreProperties.txt unicode_ranges.h --prop-list PropList.txt
clang-format --dry-run --Werror unicode_ranges.h
```

`PropList.txt` 默认从 `UnicodeData.txt` 所在目录读取。属性文件的版本必须一致；缺少必需属性会报错，
不会回退成宽泛的 skip 表。表中记录输入文件的 SHA-256；相同输入不会重写文件或改变其修改时间。

`info`、`verify` 默认各有 1 GiB 文件和候选内存上限，`print`、`visualize` 默认各为 64 GiB，可用 `--max-file-bytes`、
`--max-memory-bytes` 调整。内存预算覆盖新模型 payload 和结构验证 scratch，不包括调用者已有模型、输入缓冲、
分配器开销或整个进程 RSS。`verify --semantic` 还重算最长 failure suffix。

## 打印打包内容

`info` 保留简短统计。`print` 默认以文本格式打印全部内容；`--section` 可以只选择一类，
`--format json` 输出便于程序处理的 JSON 对象。省略 `--output` 时写入标准输出，指定时沿用禁止覆盖的文件发布规则。

| `--section` | 内容 |
| --- | --- |
| `all`（默认） | 以下全部七类内容 |
| `config` | 当前 dump 格式版本、节点/词条/字符数量、最长词 token 数、payload/index 字节数、nocase、替换开关及 skip/fold/replacement/boundary 数量 |
| `keywords` | 按 keyword ID 输出完整原始词条，保留第一条插入时的大小写、标点及词中 skip 字节 |
| `skips` | 实际保存 skip bit 的 canonical token；包含只用于配置、未出现在词条里的 token |
| `folds` | 非恒等的有效折叠规则，包含 source/target ID 和两端原始字节；包括内置 ASCII 规则 |
| `replacements` | fold 后源 token 到最终目标的非恒等替换；链已展开，源 ID 已去重 |
| `symbols` | 完整字符表，包含 symbol ID、folded ID、canonical ID、该 ID 的 skip bit 及原始 token |
| `boundaries` | 按规则 ID 输出 include/exclude 模式、输入边缘匹配策略及成员 token 列表 |

每个字节串同时打印 `byte_length`、`text` 和 `hex`。`text` 使用 JSON 字符串转义规则，保留基本多文种平面的可见 UTF-8，
转义引号、反斜杠、空白、控制字符、不可见字符、私用区和 Unicode 非字符；辅助平面字符使用 `\uXXXX\uXXXX`
代理对转义，避免经典控制台替换成乱码。非法 UTF-8 字节以 `\u00XX` 显示。
`hex` 是无分隔符的大写十六进制，是还原原始字节的依据；不要用 `text` 重建非法 UTF-8 或其他编码的数据。
例如换行的 `text` 为 `"\n"`，`hex` 为 `"0A"`，不会在终端产生额外记录或执行控制序列。
JSON 中仅包含所选 section 对应的键；空表为 `[]`。文本格式使用 `[section]` 标题，每条记录占一行。
内容逐条写出，不复制整份词表或字符表。

Windows 的真实控制台使用 Unicode 输出接口，避免把 UTF-8 字节当作 GBK 等活动码页解码而乱码或中途失败；
不修改终端码页。文件和管道仍输出 UTF-8。字体不支持的私用区/辅助平面字符可通过转义和 `hex` 核对，
不会因字符编码无法显示而终止；磁盘写满、管道关闭等实际 I/O 错误仍会返回失败。

打印结果描述冻结后的实际配置。匹配先折叠和替换再检查 skip，`symbols.skipped` 只表示该 ID 自身的存储位，
来源 token 是否被跳过需查看其 `canonical_id` 对应的 skip bit。dump 不保留 `--skip-space` 等选项来源、
已禁用的自定义 fold、替换规则的原始顺序/链、tokenizer/编码或查询时的重叠、非法输入处理及命中数量设置。
`config.version` 表示当前写出格式（有边界规则为 2.2；否则有替换为 2.1；否则为 2.0）；加载兼容的未来 minor 版本也按当前已知表展示。

## 可视化范围

可视化默认最多 1000000000（10 亿）个节点，超过上限报错；`--root-id`、`--max-depth` 可明确选择子图。
`--max-depth` 默认已经是 `UINT32_MAX`（4294967295），深度按相对所选根节点的 token 数计算，
设为 0 只导出根节点。核心节点 ID 和最长词的限制仍遵循库接口，见头文件注释。
默认 `--label-format utf8` 保留合法 UTF-8 可见文字；`--utf8-labels` 是兼容别名，`--label-format hex`
可显式切换十六进制。语法敏感 ASCII、空白、控制字符、Unicode 非字符和非法 UTF-8 字节显示为十六进制。
图中仅绘制选中节点之间的 trie/failure 边；节点的 output 值是输出链头的 keyword ID。
输出函数位于工具，核心库只提供只读节点、字符表及词条访问。

子图选择将每层连续的 BFS ID 合并为区间，写出时仅保留尚未遍历子节点的父节点队列，
不为全部节点建立 map/set 或保留节点副本。完整图与单条长链只需一个选取区间。
按布局估算，1 亿节点的 Node（16 bytes）和 Label（4 bytes）表约占 1.86 GiB，
结构验证的深度数组另需约 0.37 GiB，还需加上关键词、字符表和字节池等空间；默认预算据此留出余量。
完整图文本可能达数十 GiB；上述上限不保证外部绘图器能对整图排版。
大词表通常先用 `--root-id` / `--max-depth` 选取要检查的部分。

## 文本绘图格式

| 格式 | 文件扩展名 | 自动布局组件与用法 |
| --- | --- | --- |
| `dot` | `.dot` | [Graphviz](https://graphviz.org/)：`dot -Tsvg words.dot -o words.svg` |
| `mermaid` | `.mmd` | [Mermaid](https://mermaid.js.org/) 流程图，适合 Markdown/网页集成 |
| `plantuml` | `.puml` | [PlantUML](https://plantuml.com/state-diagram) 状态图，保留实线 trie 与虚线 failure 边 |
| `d2` | `.d2` | [D2](https://d2lang.com/)：`d2 --layout dagre words.d2 words.svg`，支持更换布局引擎 |
| `nomnoml` | `.noml` | [nomnoml](https://github.com/skanaar/nomnoml)：`npx nomnoml words.noml > words.svg`，使用 graphre 自动布局 |
| `graphml` | `.graphml` | [yEd](https://yed.yworks.com/support/manual/fileformat.html) / [Gephi](https://docs.gephi.org/desktop/User_Manual/Import/GraphML_Format/) 导入后应用层次或力导向布局 |

导出这些格式无需安装绘图器。D2、nomnoml 可以直接从文本生成 SVG；GraphML 使用标准 `label`、`kind`
属性，并包含 yWorks 节点标签、箭头和虚线样式扩展，便于在图编辑器中继续操作。
新增组件选择以通用有向图、自动布局和实际维护情况为依据；还调研了
[Diagrams](https://diagrams.mingrammer.com/)（面向 Python 云架构图，使用 Graphviz）和 Blockdiag，
前者的定位与 AC 状态图差别较大，后者近年维护较少，未增加专用导出。

## 验证与性能

```text
python tools/ac_automation/test_tool.py /path/to/ac_automation_tool /path/to/build/_agent_tmp
ac_automation_benchmark /path/to/external/words.utf8.txt
ac_automation_benchmark --scan /path/to/words.ac /path/to/query.utf8.txt
```

命令行集成测试生成小型临时词表，检查六种格式的完整拓扑与子树、默认 UTF-8、转义、
Unicode trim、失败退出码、不覆盖既有输出和临时文件清理。打印检查覆盖完整/分区 JSON、文本与文件输出、
原始词条、配置专用 token、fold、NUL/不可见字节及非法输入；分别检查四类内置 skip 选项、跨语言文字保留、
组合后的规则一致性，以及未跳过的不可见文字仍被正确转义。替换检查覆盖 UTF-8 别名、链展开、fold/skip 组合、
规范化词条去重、回环最小代表及最终映射、2.1 load/verify、替换表打印及图中的 canonical 标签；冲突和非法规则不生成结果文件。
Windows 还在隐藏的真实控制台验证 GBK/UTF-8
码页、中文、辅助平面转义和长词条跨缓冲区输出，检查打印完成且不改变码页。检测到 D2 时还会验证语法并生成 SVG；
设置 `AC_AUTOMATION_NOMNOML_MODULE` 为已安装 nomnoml 模块的绝对目录后，会执行真实的 nomnoml SVG 渲染。
基准显式要求外部词表，不会下载或把完整敏感词表写入仓库；缺少/非法输入返回失败。
它在 build/load 后逐词检查原 keyword ID 与完整 byte 范围，防止短词命中掩盖长词丢失；
报告插入、冻结、7 次测量中位数的匹配和内存 dump/load。
扫描文本是词表拼接以及 U+10FFFF 未知字符序列，不代表生产文本分布。v1 的选择/输出接口不同，比较数值须结合语义解读。
磁盘 I/O、峰值 RSS、真实业务流量和不同 CPU 的结果需单独测量。

`--scan IMAGE INPUT` 复用同一个基准程序，加载已构建的普通或带边界模型，对同一份查询文本测量
callback 匹配和 `contains()`。文件读取、load 和首次 match scratch 分配不计入扫描耗时；每次测量重复扫描，目标为 32 MiB，
最多重复 256 次，输出 7 次测量的中位数以及单遍命中数、输入大小、重复次数、payload 和校验和。
match 每遍检查状态、完整消费和稳定的命中计数。contains 使用独立重复次数，检查状态、消费位置及命中状态；
其吞吐量只使用实际消费字节，不用完整文件长度夸大提前返回的性能。输入必须非空且不超过 1 GiB；镜像使用库默认 load 预算。
比较时使用相同编译选项和 CPU，交替运行多个批次；规则变化可能改变命中数，因此配置间的耗时差
同时包含边界判断和不同命中分布的影响。该模式不下载、修改或自动生成词表。

CI 复用同一个 `ac_automation_benchmark`，增加以下验证模式：

| 模式 | 验证内容 |
| --- | --- |
| `--graph INPUT` | 读取 Library Checker 输入，比较 build/load 节点与匹配结果，输出官方格式的 trie/failure 图 |
| `--unicode ORACLE` | 按独立 oracle 验证 UTF-8、UTF-16LE、UTF-16BE tokenizer |
| `--fuzz` | 固定种子变异 20000 个 dump，重算校验和后检查 load、失败原子性和匹配边界 |

```text
python ci/ac_automation_v2_stress.py --benchmark /path/to/build/bin/ac_automation_benchmark --work-dir /path/to/build/ac_automation_pressure --upstream-cxx clang++ --profile full
```

脚本下载固定版本并校验 SHA-256 的上游生成器，在构建目录生成 72 个 Library Checker 用例，
按上游输入/输出哈希核验图结构，并构造 3363077 条 Unicode oracle 记录。
同时把 `random_02.in` 转为逐行词表，运行原有 v1/v2 性能测试，将结果写入 `outputs/benchmark.csv`。
上游生成器使用 C++17；库和 benchmark 保持 C++14。`--profile quick` 将图用例减为 9 个。
生成器、数据和 `summary.json` 均保留在 `--work-dir`，不加入仓库。
独立的 `AC automation stress` 工作流使用 Clang ASAN/UBSAN 执行上述验证；耗时只作记录，不设置性能阈值。
其中 22 KB 的 Kuhn 原始样本固定在 `test/case/ac_automation_v2/UTF-8-test.txt` 并校验 SHA-256，
Unicode 验证无需访问原始个人站点。工作流从当前 Clang 查询 sanitizer 运行库目录，供测试与工具加载。
专用压测构建关闭网络组件，减少与 AC 无关的依赖；常规构建矩阵继续覆盖网络功能。

静态安装的 CMake 配置保留 xxHash 配置目录的相对提示，以及构建树的原始位置作为后备。
部署时仍须提供 xxHash 库及其 CMake 配置；安装目录与私有依赖前缀一同移动时保持两者的相对位置，
或通过 `xxHash_DIR` / `CMAKE_PREFIX_PATH` 显式选择依赖。

## 库调用

```cpp
using ac = atfw::util::string::ac_automation_v2;
ac dictionary;
dictionary.set_nocase(true);
dictionary.set_skip(" ");
const ac::replacement_rule aliases[] = {{"壹", "一"}};
dictionary.set_replacements(gsl::span<const ac::replacement_rule>(aliases));
dictionary.insert_keyword("ab");
dictionary.build();
ac shared_copy = dictionary;
std::vector<ac::match_t> hits;
ac::match_context scratch;
auto report = shared_copy.match("A B ab", hits, ac::match_options(), &scratch);
```

实际调用应检查配置、insert/build/load 返回的 `status`，以及 `report.code` / `report.stopped`。
默认一次输出多个不重叠命中，选择最早结束的词，同结束位置选 token 数最多者；不等同 leftmost-longest。
`max_matches=0` 不限次数；`allow_overlap=true` 枚举后缀输出链。位置是输入 byte 范围，包含词中间的 skip。
默认非法编码清空当前候选后继续；strict 模式在首个错误停止，vector 回滚本次追加，span/callback 保留已交付内容。
`contains` 早停且不需要位置环。context 仅复用 scratch，每个并发扫描使用独立 context。
