# 毕业设计项目总交接文档

更新日期：2026-09-16（Asia/Shanghai）

本文是项目唯一的长期动态交接文档。每个阶段结束后，更新当前状态、代码结构、Git、测试、人工验收、技术决策和下一步，避免另建多套状态文件。

依据：项目历史资料、已通过的理解与交接审查、`04.md` 开发任务、`05.md` 封版授权及人工验收结果，以及正式工程构建、测试、Git 和 GitHub 实查。历史环境审计与本项目 Phase 00 验证分别记录。

**真实性优先级：实际运行结果 > 当前真实代码 > 当前 Git 状态 > 当前 GitHub 状态 > 已验证环境审计 > 历史项目资料 > 推测。** 当前阶段授权以用户最新具体指令为准。

# 1. 项目基本信息

- 题目：**基于 C++ 的软件供应链漏洞风险评估系统的设计与实现**。
- 性质：本科毕业设计，面向软件项目开发者、维护者及答辩演示。
- 定位：使用 C++ 实现能够运行、测试、演示并支撑论文的软件供应链风险评估桌面系统，控制在本科毕业设计合理范围内。
- 实现语言为 C++ 不代表输入组件仅限 C++ 生态；首期支持生态范围待定。
- 正式仓库：[yyyyyyqqqqq/Graduation-Project](https://github.com/yyyyyyqqqqq/Graduation-Project)。

# 2. 当前一句话状态

Phase 00 已完成自动验收和用户 Debug / Release GUI 人工验收，正式 Qt/C++20 工程基线已经建立并完成 Git 封版，当前准备进入 Phase 01。

# 3. 当前阶段

**CURRENT PHASE：PHASE 00 COMPLETE。**

**NEXT PHASE：Phase 01 —— 应用基础框架。**

Phase 01 尚未开始，等待用户下一阶段具体指令。

# 4. 正式项目目录

唯一正式根目录：`D:\codex\Graduation Project\project`。

未来源码、CMake、测试、项目文档和 Git 均围绕此目录组织。不维护多个正式工程副本；父目录的 `01.md` 至 `05.md` 是准备、开发与封版任务的输入资料。

`D:\codex\SupplyChainRiskAssessment` 是历史环境验证目录，不是正式项目；本轮未检查或修改其内容。

# 5. Git / GitHub 当前状态

以下记录 **2026-09-16 Phase 00 封版基线**，后续接手仍需重新核对真实 Git 状态：

| 项目 | 当前值 |
| --- | --- |
| 当前分支 | `main`；HEAD 指向 `refs/heads/main` |
| commit | 1 个正式初始化提交：`feat: complete phase 00 project initialization` |
| origin（fetch / push） | `https://github.com/yyyyyyqqqqq/Graduation-Project.git` |
| upstream | `origin/main` |
| ahead / behind | `0 / 0` |
| tag | annotated tag：`phase-00-complete`，指向本次初始化提交，已推送远程 |
| tag message | `Phase 00 complete: project initialization baseline` |
| 工作区 | clean；staged / modified / untracked 均为 0（不计 ignored 产物） |

提交使用仓库级身份 `yyyyyyqqqqq` / `104704290+yyyyyyqqqqq@users.noreply.github.com`。首次提交仅含 `.gitignore`、CMakeLists.txt、本文、三个 src 文件和一个 tests 文件。

本文随该首次提交保存，完整 commit hash 由 `git rev-parse 'phase-00-complete^{commit}'` 获取，亦记录在本轮最终封版回复中，避免把提交自身的 hash 写入同一个提交。

封版核验使用 `git status`、`git branch -vv`、`git log`、`git rev-list --left-right --count main...origin/main` 及 `git ls-remote origin`；远程 main 与 tag 解引用后的 commit 必须与本地 HEAD 一致。

远程是重新创建后的同名仓库，不继承旧仓库历史。今后操作前仍需读取真实状态。

# 6. 正式环境基线

下表为**此前真实工具链集成审计冻结的基线**。Phase 00 已在正式工程复验 CMake 3.30.5、Ninja 1.12.1、MinGW GCC 13.1.0 x86_64、C++20、Qt 6.11.2 运行时及 Graphviz 16.1.0；其余环境项目仍以历史审计为依据。

| 技术 | 版本 / 职责 |
| --- | --- |
| Windows | Windows 11 x64，Build 26200 |
| C++ | C++20，正式核心实现语言 |
| Qt | Qt 6.11.2 MinGW 64-bit；Qt Widgets 为主界面 |
| Qt Creator | 20.0.0；Kit：Desktop Qt 6.11.2 MinGW 64-bit |
| 编译器 | MinGW-w64 13.1.0 x86_64 |
| CMake / Ninja | 3.30.5 / 1.12.1，配置与构建 |
| SQLite / Qt Sql / QSQLITE | 本地持久化 |
| Qt Network / Qt JSON | 网络访问、JSON 数据处理 |
| Qt Concurrent | 按需执行后台任务 |
| Qt Test / CTest | 自动测试及测试执行 |
| Graphviz / Qt Svg | Graphviz 16.1.0，关系图生成与 SVG 展示能力 |
| windeployqt | Windows Qt 依赖部署 |
| Git / GitHub CLI | 历史版本 2.54.0.windows.1 / 2.96.0；本轮确认可用，未重新查询版本 |

关键历史工具路径：

```text
Qt       D:\program\Qt\6.11.2\mingw_64
MinGW    D:\program\Qt\Tools\mingw1310_64\bin
CMake    D:\program\Qt\Tools\CMake_64\bin\cmake.exe
Ninja    D:\program\Qt\Tools\Ninja\ninja.exe
Graphviz C:\Program Files\Graphviz\bin\dot.exe
```

历史最终结论：**FULLY READY**。审计曾真实验证编译链、C++20 / Qt、SQLite、HTTPS / JSON / NVD / EPSS / KEV、Qt Concurrent、Graphviz / SVG、Qt Test / CTest、Debug / Release、windeployqt、独立运行及端到端集成。

历史结论证明当时工具链集成可用。正式工程的 Phase 00 原位最小复验现已通过，CMakeCache、编译命令及运行结果均确认 Qt 6.11.2，未混入 6.11.1；没有重装或替换工具链。正式业务功能尚未开始。

# 7. 环境已知提醒

- Qt 6.11.2 是正式基线；6.11.1 仅是已验证回退环境，不随意切换、卸载、升级或替换工具链，不无故修改系统 PATH。
- **NVD API Key 历史状态为未确认 / PENDING，本轮没有重新检查。** 早期阶段不受阻，批量同步前处理；密钥禁止写入源码、日志和 Git，可用用户环境变量 `NVD_API_KEY` 或受忽略的本地配置。
- Graphviz 最终部署策略待定，`dot.exe` 属于外部依赖，不由 windeployqt 自动打包。
- SQLite 的 `QSqlDatabase`、`QSqlQuery` 等对象不跨线程共享；各线程使用自己的连接，处理事务与锁；GUI 更新在主线程执行。
- 历史 `LongPathsEnabled=0`，保持浅目录；正式路径含空格，命令必须正确引用。
- 历史 optional Vulkan Headers / DX12 compiler 提示不阻塞当前 Widgets + SVG 方案；Qt 导入配置为 RelWithDebInfo，历史 Debug 已验证，不据此重装。
- 历史独立部署验证是在当前电脑排除开发路径后完成，尚不等同于全新 Windows 电脑验收。
- **当前无已知环境 Blocker。** 本轮未重跑工具链；只有出现具体可复现失败才作最小修复。

# 8. 系统核心目标

以下为最终产品目标，尚未实现。系统应回答：项目使用了什么组件，哪些组件关联漏洞，哪些漏洞适用于当前版本，哪些风险应优先处理，以及为什么风险高。

结果需要可解释、可追溯，并能通过扫描记录、历史比较和报告支持实际演示。不能让完整业务完全依赖实时公网 API，应在相应阶段落实导入、缓存或本地演示数据。

# 9. 核心业务流程

**规划流程：**

```text
项目 → SBOM → Component → SBOM Quality → Component Identity
→ Dependency → Vulnerability Data → Candidate Matching
→ Applicability → Risk Assessment → Risk Explanation
→ Dashboard → Scan → Historical Comparison → Report
```

输入组件与依赖经过质量诊断、身份识别及漏洞适用性判断，形成风险结果；结果被展示并保存，供后续比较和报告使用。缺失数据应保留诊断或不确定状态，不能直接解释成无漏洞、无风险。

# 10. 核心模块

**全部为规划，当前未实现：**

| 模块 | 主要职责 |
| --- | --- |
| 项目管理 | 创建、查看、维护、删除项目，组织扫描记录 |
| SBOM / 质量诊断 | 优先 CycloneDX JSON；解析元信息、组件、PURL、bom-ref、依赖，检查缺失字段、重复组件与无效引用 |
| 组件 / 身份 | 管理名称、版本、类型和标识，提供搜索、详情与可靠匹配身份 |
| 依赖 / Graphviz | 分析直接和间接依赖、深度、路径与上下游，展示依赖图 |
| 漏洞数据 | 获取、导入、缓存漏洞及利用信息 |
| 候选匹配 / 适用性 | 识别候选漏洞，再判断当前组件版本是否受影响 |
| 风险 / 解释 | 输出风险分数或等级、处理优先级及理由 |
| Dashboard | 展示风险分布、高风险项、统计和趋势 |
| 扫描 / 比较 / 报告 | 保存分析结果，比较组件、漏洞及风险变化，生成评估报告 |

主要业务概念预计包括 Project、Scan、Component、Dependency、Vulnerability、Finding 和 RiskResult；具体类与数据库结构待对应阶段确定，不提前建空架构。

# 11. 漏洞数据源定位

依据已审查的规划：

- **NVD**：漏洞基础信息，包括 CVE、描述、CVSS、CPE 及相关受影响信息。
- **EPSS**：利用可能性的预测信息，用于补充风险排序，不能证明版本适用性。
- **CISA KEV**：已知被利用漏洞的信息，补充实际利用信号；未收录不等于没有风险。

三者职责不同。具体接口、同步和缓存设计在后续 Phase 决定，本轮未查询实时漏洞服务。

# 12. 风险评估原则

**Risk ≠ CVSS。正式风险模型尚未确定。**

候选因素包括 CVSS / 严重程度、漏洞适用性、直接或间接依赖、依赖深度、利用信息、组件重要性与业务规则。最终分数／等级须有解释依据。

不能单凭名称相似认定漏洞匹配，不能把相关 CVE 直接认定为当前版本受影响；依赖关系也不等于运行时可达性。公式、权重、阈值、缺失值处理、聚合方式与模型验证留待 Phase 12 及相关前置阶段确定。历史环境实验风险值约 `8.99996` 不是正式算法。

# 13. Phase 路线

| Phase | 名称与主要目标 | 当前状态 |
| --- | --- | --- |
| 00 | 工程初始化：最小 Qt/C++20 构建、运行与测试基线 | COMPLETE |
| 01 | 应用基础框架：导航、页面、日志、配置、基础数据库访问 | 未开始 |
| 02 | 项目管理 | 未开始 |
| 03 | CycloneDX SBOM 导入 | 未开始 |
| 04 | SBOM 质量诊断 | 未开始 |
| 05 | 组件管理与持久化 | 未开始 |
| 06 | 依赖关系分析 | 未开始 |
| 07 | Graphviz 可视化 | 未开始 |
| 08 | 漏洞数据获取／导入／缓存模块 | 未开始 |
| 09 | 组件身份解析 | 未开始 |
| 10 | 漏洞候选匹配 | 未开始 |
| 11 | 漏洞适用性判断 | 未开始 |
| 12 | 风险评估模型与解释 | 未开始 |
| 13 | Risk Dashboard | 未开始 |
| 14 | 扫描快照 | 未开始 |
| 15 | 历史比较 | 未开始 |
| 16 | 风险报告 | 未开始 |
| 17 | 最终测试、部署和答辩封版 | 未开始 |

# 14. 已完成 Phase

**Phase 00 —— COMPLETE。**

- 正式 CMake / C++20 工程、Qt 6.11.2 MinGW 64-bit、Qt Widgets 与最小 MainWindow。
- Qt Test / CTest；SQLite / QSQLITE 内存数据库最小读写与连接清理；Graphviz dot → 临时 SVG 生成、内容验证与清理。
- Debug 和 Release 均完成全新目录 Clean Configure / Clean Build。
- Debug CTest 3/3 PASS、Release CTest 3/3 PASS；GUI 自动 smoke PASS。
- 用户 Debug GUI、Release GUI 人工验收均 PASS。
- 首次正式提交、main 推送及 `phase-00-complete` annotated tag 封版。

# 15. 当前真实工程结构

正式源码结构：

```text
D:\codex\Graduation Project\project\
├─ .gitignore
├─ CMakeLists.txt
├─ PROJECT-HANDOFF.md
├─ src\
│  ├─ main.cpp
│  ├─ MainWindow.h
│  └─ MainWindow.cpp
└─ tests\
   └─ Phase00SmokeTest.cpp
```

`.git` 为版本元数据；本地 `build-debug/`、`build-release/`、Qt Creator 的 `build/` 和 `.qtcreator/` 均被忽略，不进入正式提交。没有正式业务数据库、业务模块或 Phase 01 框架。

临时理解报告已按封版授权删除，未进入任何正式提交；唯一长期动态交接文档为本文。

# 16. 当前已经实现的功能

当前已经建立 **Phase 00 工程基础能力**：可编译 CMake 工程、可运行 Qt Widgets 应用、最小 MainWindow、自动测试入口、SQLite 基础能力及 Graphviz 基础调用能力。

SQLite 和 Graphviz 验证位于测试中；GUI 仅显示项目名称和 Phase 00 启动状态。**正式业务功能尚未开始**，没有项目管理、SBOM 解析、漏洞分析、风险计算或 Phase 01 导航框架。

# 17. 当前自动测试状态

**Phase 00 Automated Acceptance：PASS。**

| 正式工程验证 | 结果 |
| --- | --- |
| Debug / Release Clean Configure、Clean Build | 均 PASS（Phase 00 开发轮） |
| Debug CTest | 3/3 PASS（本次封版回归） |
| Release CTest | 3/3 PASS（本次封版回归） |
| Qt / C++20 smoke、SQLite 读写、Graphviz SVG | 均 PASS |
| Debug / Release GUI 自动 smoke | 均 PASS，窗口响应正常并以 0 退出（开发轮） |

测试源码为 `tests/Phase00SmokeTest.cpp`，CTest 项目为 `Phase00.qtCppSmoke`、`Phase00.sqliteReadWrite`、`Phase00.graphvizSvg`。上述为正式工程自己的结果，区别于历史环境审计的 3/3 PASS。

回归命令：`ctest --test-dir build-debug --output-on-failure` 和 `ctest --test-dir build-release --output-on-failure`，使用冻结 CMake 目录中的 ctest.exe。CTest 为子进程设置 Qt / MinGW DLL 搜索路径，不修改系统 PATH。

# 18. 当前人工验收状态

- 项目理解审查：**PASS**，依据用户提供的 `03.md` 中 ChatGPT 审查结论。
- 本文前版审查：**PASS**，依据用户提供的 `05.md`。
- **Phase 00 Manual Acceptance：PASS**，由用户实际操作后在 `05.md` 中确认。
- Debug GUI：**PASS**；Release GUI：**PASS**。
- 用户确认标题和 Phase 00 内容正确，窗口可移动、调整大小和关闭，无闪退、DLL / Qt platform plugin 错误或明显卡死。

# 19. 当前重要技术决策

- 使用 C++20、Qt Widgets、CMake、SQLite、Graphviz，继续冻结环境；Qt Network / JSON / Concurrent / Test 按实际需要使用。
- SBOM 优先 CycloneDX JSON，SPDX 留作可选扩展；匹配优先依赖软件包身份和版本证据。
- GUI、应用逻辑、领域与基础设施可作职责划分，但不强制提前建满架构层。
- 一次只做一个 Phase；每个文件须有实际职责，不创建未来模块、无用接口或 Factory / Adapter / Manager 空架构。
- 稳定代码不无故重构，自动测试不能替代用户 GUI 人工验收。
- 仅维护本文作为长期动态交接文档，不新增多套状态文件。
- SQLite 每线程独立连接、主线程更新 GUI，密钥不进入代码、日志及 Git。

**AI 分工：** ChatGPT 负责规划、Phase、Prompt、方案、审查及验收设计；Codex 负责读取真实工程、实现、编译、自动测试和 Git 检查；用户负责 GUI 操作、人工验收和决定是否进入下一 Phase。

**统一阶段闭环：** 明确 Phase → Codex 读取真实工程 → 实现 → Build → 自动 Test → 用户人工验收 → 确认 PASS → Git commit / push → 必要时 tag → 更新本文 → 下一 Phase。个别阶段有特殊顺序时，以具体授权为准。

# 20. 当前待定事项

- NVD API Key 当前申请状态、漏洞同步与缓存策略、离线演示数据范围。
- CycloneDX 具体版本、首期组件生态、输入容错及质量诊断规则。
- PURL / CPE 规范化、版本范围比较、匹配和适用性证据表达。
- 正式风险公式、阈值、权重、解释、聚合和评估方法。
- 扫描快照粒度、跨扫描组件身份、漏洞库更新与组件变化的比较规则。
- Graphviz 最终部署、报告格式、全新电脑验证及是否引入 CI。

以上在对应阶段决定，不要求 Phase 00 提前解决。

# 21. 当前已知问题 / 风险

- **Phase 00 无 Blocker。** 代码、自动测试、人工验收及 Git 封版均已通过。
- optional Vulkan Headers 缺失仍为 non-blocking warning，不影响当前 Qt Widgets 工程；无需据此安装额外组件。
- 本轮未发现正式目录冲突、错误 origin 或 GitHub 认证问题；Qt Creator 新增的本地配置和构建目录已被正确忽略并保留。
- 正式业务功能尚未开始；不要把 Phase 00 基础能力与未来完整系统混同。
- 后续主要技术风险是身份／版本误匹配、缺失数据产生错误确定结论、评分缺乏依据，以及范围扩张造成过度设计；应在相应阶段验证。
- 父目录旧资料包含重复版本；后续以本文当前状态和更高优先级实查证据为准。

# 22. 下一步

**READY FOR PHASE 01 —— 应用基础框架。Phase 01 尚未开始。**

预计内容：正式 MainWindow 基础框架、基础导航、页面切换、应用日志、基础配置和基础数据库访问。

本轮仅完成 Phase 00 封版并停止；等待用户 Phase 01 具体任务，不提前实现上述内容或后续业务。

# 23. 新 Codex 接手规则

1. 先阅读本文及当前阶段具体指令，再读取真实文件、Git 和必要的 GitHub 状态，不凭历史文字假定实现存在。
2. 如实区分已实现、已验证和规划；发现冲突按本文顶部真实性优先级处理，并更新相关状态。
3. 只执行授权阶段，控制改动和文件数量，不提前实现后续模块，不无故重构或修改环境。
4. 实现后运行适当构建和测试，清楚记录结果及限制；阶段通过必须包含用户要求的人工验收。
5. 未经允许不得 `reset --hard`、`clean -fd`、force push、rebase、删除 branch / tag、删除用户文件或重写 Git 历史；发现已有错误 remote 先报告，不自行覆盖。身份配置优先仓库级。
6. 完成阶段后就地更新本文的状态、结构、Git、测试、验收、决策、问题和下一步，不另造动态管理文档。
7. 当前 `05.md` 授权 Phase 00 正式封版及首次 commit / push / annotated tag，禁止开始 Phase 01。下一阶段须依据新的具体指令。
