# AC 自动机工具

启用 `PROJECT_ENABLE_TOOLS` 后构建 `ac_automation_tool`。公开库接口、匹配语义及内存和磁盘布局见
[ac_automation_v2.h](../../include/string/ac_automation_v2.h) 中的注释文档。

```text
ac_automation_tool build --input words.txt --output words.ac --nocase --skip-space --skip-punctuation
ac_automation_tool info --input words.ac
ac_automation_tool verify --input words.ac --semantic
ac_automation_tool visualize --input words.ac --output words.dot --format dot --max-depth 3
ac_automation_tool visualize --input words.ac --output words.mmd --format mermaid --root-id 1 --max-nodes 500
ac_automation_tool visualize --input words.ac --output words.puml --format plantuml --utf8-labels
```

参数使用 `--name value`，不支持 `--name=value`。重复参数、未知参数、缺失值、非法数字都会返回非零退出码。
输出路径必须不存在。工具在相同目录独占创建临时文件，写入并关闭后以禁止覆盖的操作发布；已有文件不被替换。
路径沿用 `file_system` 的窄字符路径约定，不额外承诺 Windows 任意 Unicode 路径支持。

输入词表为 UTF-8，每行一个词，忽略空行及两端 Unicode White_Space。只移除文件开头的 UTF-8 BOM；
`#`、逗号等没有注释或字段分隔含义。非法 UTF-8 会报告行号并停止，不留下最终输出。
词条经当前 skip/fold 处理后重复时保留第一条；去 skip 后为空的词条计入 ignored。

构建选项：

| 选项 | 作用 |
| --- | --- |
| `--nocase` / `--ignore-case` | 内置单字节 ASCII A–Z 折叠；不是完整 Unicode case folding |
| `--case-fold-file FILE` | 每行两个十六进制 token，例如 `4100 6100`；要求同时指定 `--nocase` |
| `--skip-space` | Unicode White_Space |
| `--skip-punctuation` | Unicode General_Category P* |
| `--skip-symbols` | Unicode General_Category S* |
| `--skip-invisible` | Cc、Cf 和 Default_Ignorable_Code_Point |
| `--skip-file FILE` | 去开头 BOM 后按 UTF-8 拆成 skip token；包括空格和换行，不做 trim |

这些 Unicode 规则只在工具中展开，库和 dump 保存不透明 token bytes，不保存编码或 Unicode profile。
表来自 Unicode 18.0.0，许可证见 [UNICODE-LICENSE.txt](UNICODE-LICENSE.txt)。
可用 `generate_unicode_ranges.py UnicodeData.txt DerivedCoreProperties.txt unicode_ranges.h` 重建表，再运行 clang-format。

`info`、`verify`、`visualize` 默认各有 1 GiB 文件和候选内存上限，可用 `--max-file-bytes`、
`--max-memory-bytes` 调整。内存预算覆盖新模型 payload 和结构验证 scratch，不包括调用者已有模型、输入缓冲、
分配器开销或整个进程 RSS。`verify --semantic` 还重算最长 failure suffix。

可视化默认最多 1000 个节点，超过上限报错；`--root-id`、`--max-depth` 可明确选择子图。
默认以十六进制显示不透明 token；`--label-format utf8`（别名 `--utf8-labels`）保留合法 UTF-8 可见文字，
语法敏感 ASCII 和控制字符仍显示十六进制。
图中仅绘制选中节点之间的 trie/failure 边；节点的 output 值是输出链头的 keyword ID。
输出函数位于工具，核心库只提供只读节点访问。

## 验证与性能

```text
python tools/ac_automation/test_tool.py /path/to/ac_automation_tool /path/to/build/_agent_tmp
ac_automation_benchmark /path/to/external/words.utf8.txt
```

命令行集成测试生成小型临时词表，检查三个图格式、Unicode trim、失败退出码、不覆盖既有输出和临时文件清理。
基准显式要求外部词表，不会下载或把完整敏感词表写入仓库；缺少/非法输入返回失败。
它在 build/load 后逐词检查原 keyword ID 与完整 byte 范围，防止短词命中掩盖长词丢失；
报告插入、冻结、7 次测量中位数的匹配和内存 dump/load。
扫描文本是词表拼接以及 U+10FFFF 未知字符序列，不代表生产文本分布。v1 的选择/输出接口不同，比较数值须结合语义解读。
磁盘 I/O、峰值 RSS、真实业务流量和不同 CPU 的结果需单独测量。

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

## 库调用

```cpp
using ac = atfw::util::string::ac_automation_v2;
ac dictionary;
dictionary.set_nocase(true);
dictionary.set_skip(" ");
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
