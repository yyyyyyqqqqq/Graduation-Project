# 毕业设计项目总交接文档

更新日期：2026-09-16（Asia/Shanghai；Phase 04 最终封版）

本文是项目唯一的长期动态交接文档。每个阶段结束后，更新当前状态、代码结构、Git、测试、人工验收、技术决策和下一步，避免另建多套状态文件。

本文保留此前用户授权的 Phase 03 封版后审计修订，包括环境证据范围、运行时数据边界、解析与窗口生命周期限制及回归操作方法；在此基础上就地更新 Phase 04 正式状态。本次封版复核已验收的代码、重新执行 Debug / Release 自动回归并核验 Git；人工验收依据用户在 `17.md` 中确认的实际结果，不冒充本轮重新操作。历史环境审计与各阶段验证分别记录。

依据：项目历史资料、已通过的理解与交接审查、`04.md` 开发任务、`05.md` 封版授权及人工验收结果、`06.md` 长期规则增强要求、`07.md` 文档提交授权、`08.md` Phase 01 开发要求、`09.md` Phase 01 封版授权、`10.md` Phase 02 开发要求、`11.md` Phase 02 人工验收及封版授权、`12.md` / `13.md` 隐私规则与维护授权、`14.md` Phase 03 开发要求、`15.md` Phase 03 人工验收及最终封版授权、此前用户授权的 Handoff 审计更新、`16.md` Phase 04 开发要求、`17.md` Phase 04 人工验收及最终封版授权，以及正式工程构建、测试、Git 和 GitHub 实查。

**真实性优先级：实际运行结果 > 当前真实代码 > 当前 Git 状态 > 当前 GitHub 状态 > 已验证环境审计 > 历史项目资料 > 推测。** 当前阶段授权以用户最新具体指令为准。

# 1. 项目基本信息

- 题目：**基于 C++ 的软件供应链漏洞风险评估系统的设计与实现**。
- 性质：本科毕业设计，面向软件项目开发者、维护者及答辩演示。
- 定位：使用 C++ 实现能够运行、测试、演示并支撑论文的软件供应链风险评估桌面系统，控制在本科毕业设计合理范围内。
- 实现语言为 C++ 不代表输入组件仅限 C++ 生态；首期支持生态范围待定。
- 正式仓库：[yyyyyyqqqqq/Graduation-Project](https://github.com/yyyyyyqqqqq/Graduation-Project)。

# 2. 当前一句话状态

Phase 04 已完成 SBOM 质量诊断规则、质量摘要和只读问题列表，自动测试与用户 Debug GUI 人工验收均通过并完成 Git 封版；当前准备进入 Phase 05。

# 3. 当前阶段

**CURRENT PHASE：PHASE 04 COMPLETE。**

**NEXT PHASE：Phase 05 —— 组件管理与持久化。**

Phase 05 尚未开始（NOT STARTED），等待用户下一阶段具体指令。

# 4. 正式项目目录

唯一正式根目录：`D:\codex\Graduation Project\project`。

后续源码、CMake、测试、项目文档和 Git 均围绕此目录组织。不维护多个正式工程副本；父目录的 `01.md` 至 `17.md` 是准备、开发、封版与文档维护任务的输入资料。

`D:\codex\SupplyChainRiskAssessment` 是历史环境验证目录，不是正式项目；本轮未检查或修改其内容。

# 5. Git / GitHub 当前状态

以下记录 **2026-09-16 Phase 04 封版状态**；后续接手仍需重新核对真实 Git 状态：

| 项目 | 当前值 |
| --- | --- |
| 当前分支 | `main`；HEAD 指向 `refs/heads/main` |
| commit | main 已包含独立完成提交：`feat: complete phase 04 sbom quality diagnosis`；精确 hash 由 `git rev-parse 'phase-04-complete^{}'` 查询 |
| origin（fetch / push） | `https://github.com/yyyyyyqqqqq/Graduation-Project.git` |
| upstream | `origin/main` |
| 同步状态 | main 已推送，origin/main 已同步 |
| ahead / behind | `0 / 0` |
| tag | annotated tag：`phase-04-complete`，指向 Phase 04 completion commit，已创建并推送远程 |
| tag message | `Phase 04 complete: SBOM quality diagnosis` |
| 封版时工作区 | clean；staged / modified / untracked 均为 0（不计 ignored 产物） |
| 文档维护 | 此前授权审计修订与 Phase 04 正式状态已融合在本文，随本次 completion commit 提交 |

提交使用仓库级身份 `yyyyyyqqqqq` / `104704290+yyyyyyqqqqq@users.noreply.github.com`。仓库级 `credential.https://github.com.username` 已设为 `yyyyyyqqqqq`，用于 HTTPS 认证账户选择；认证账户与 commit 作者配置是不同设置。本次仅核对现有配置，未修改凭据或全局 Git 配置。

五个 annotated tags 已核对本地 tag 对象、远端对象及 peeled target（标签实际指向的提交）：

| Tag | 固定 target | 远端核验 |
| --- | --- | --- |
| phase-00-complete | `765bfaeb69bab26de80ff0bf3b24358688d89bca` | PASS；未移动 |
| phase-01-complete | `64292ab39bc9ffbf13c36058e47b055c812a6344` | PASS；未移动 |
| phase-02-complete | `fc9ef4ea6b68a9924fed9430cb64cbcd7bbeb0b8` | PASS；未移动 |
| phase-03-complete | `e5ef271c5249c9df6afe91abf37f163e4c9e73df` | PASS；未移动 |
| phase-04-complete | 本次 `feat: complete phase 04 sbom quality diagnosis` 提交 | PASS；与本次 completion commit 一致 |

Phase 03 开发前 main 基线为 `e10351523d5115cfb620d1175265061979fc220c`；Phase 04 开发前 main 基线为 `e5ef271c5249c9df6afe91abf37f163e4c9e73df`。旧阶段精确提交号如上保留；本次 completion commit 的精确 hash 在提交生成后由 Git 查询并在封版回复中报告，不把自身尚未生成的 hash 写进自身。后续文档提交不得移动阶段标签。

同步核验使用 `git status`、`git branch -vv`、`git log`、`git rev-list --left-right --count main...origin/main` 及 `git ls-remote origin`。本次远端 main 与本地 HEAD 一致；ahead / behind 只比较提交，不代表工作区没有未提交修改，工作区 clean 已单独核验。旧四个阶段 tags 单独核对上述固定基线，不要求随 main 移动；Phase 04 tag 指向本次完成提交。

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
| Qt Network / Qt JSON | Network 为后续网络能力，当前未链接；JSON 使用 Qt Core 中的 QJson 类型，Phase 03 已接入 |
| Qt Concurrent | Phase 03 接入后台文件读取与解析；Phase 04 复用同一任务完成质量诊断 |
| Qt Test / CTest | 自动测试及测试执行 |
| Graphviz / Qt Svg | Graphviz 16.1.0 当前仅用于 Phase 00 dot → SVG 冒烟；正式应用未接入依赖图或 Qt Svg 展示 |
| windeployqt | 历史环境已验证；当前正式应用尚未完成最终部署包验收 |
| Git / GitHub CLI | 历史版本 2.54.0.windows.1 / 2.96.0；本次使用 Git 核对状态，未重新验证 CLI 版本或 gh 可用性 |

关键历史工具路径：

```text
Qt       D:\program\Qt\6.11.2\mingw_64
MinGW    D:\program\Qt\Tools\mingw1310_64\bin
CMake    D:\program\Qt\Tools\CMake_64\bin\cmake.exe
Ninja    D:\program\Qt\Tools\Ninja\ninja.exe
Graphviz C:\Program Files\Graphviz\bin\dot.exe
```

历史最终结论：**FULLY READY**。审计曾真实验证编译链、C++20 / Qt、SQLite、HTTPS / JSON / NVD / EPSS / KEV、Qt Concurrent、Graphviz / SVG、Qt Test / CTest、Debug / Release、windeployqt、独立运行及端到端集成。

历史结论证明当时工具链集成可用。正式工程的 Phase 00 原位最小复验已通过，后续 Phase 01 / 02 / 03 / 04 延续同一冻结工具链；CMakeCache、编译命令及运行结果均确认 Qt 6.11.2，未混入 6.11.1，没有重装或替换工具链。

# 7. 环境已知提醒

- Qt 6.11.2 是正式基线；6.11.1 仅是已验证回退环境，不随意切换、卸载、升级或替换工具链，不无故修改系统 PATH。
- **NVD API Key 历史状态为未确认 / PENDING，本轮没有重新检查。** 早期阶段不受阻，批量同步前处理；密钥禁止写入源码、日志和 Git，可用用户环境变量 `NVD_API_KEY` 或受忽略的本地配置。
- Graphviz 最终部署策略待定，`dot.exe` 属于外部依赖，不由 windeployqt 自动打包。
- SQLite 的 `QSqlDatabase`、`QSqlQuery` 等对象不跨线程共享；各线程使用自己的连接，处理事务与锁；GUI 更新在主线程执行。
- 历史 `LongPathsEnabled=0`，保持浅目录；正式路径含空格，命令必须正确引用。
- 历史 optional Vulkan Headers / DX12 compiler 提示不阻塞当前 Widgets + SVG 方案；Qt 导入配置为 RelWithDebInfo，历史 Debug 已验证，不据此重装。
- 历史独立部署验证是在当前电脑排除开发路径后完成，尚不等同于全新 Windows 电脑验收。
- **当前无已知环境 Blocker。** Phase 04 封版在本文更新后重新执行正式工程 Debug / Release 构建与回归，未重跑完整历史环境审计。只有出现具体可复现失败才作最小修复。

# 8. 系统核心目标

最终产品应回答：项目使用了什么组件，哪些组件关联漏洞，哪些漏洞适用于当前版本，哪些风险应优先处理，以及为什么风险高。当前已能预览导入 SBOM 声明的组件并执行本阶段规则覆盖的质量诊断；漏洞及风险评估流程尚未实现。解析成功或当前质量规则未发现问题，均不等于 SBOM 完整、组件真实存在或没有风险。

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

**项目管理、SBOM 导入、解析预览及质量诊断已实现当前阶段范围；其余能力仍为规划：**

| 模块 | 主要职责 |
| --- | --- |
| 项目管理 | 已实现创建、列表、详情、删除和持久化；扫描记录尚未实现 |
| SBOM / 质量诊断 | 已支持 CycloneDX JSON 1.4 / 1.5 / 1.6 共同字段子集导入、解析、只读预览及 issue-based 质量诊断；无总体质量分或自动修复 |
| 组件 / 身份 | 已有组件最小内存模型和只读表格；持久化、搜索、身份解析与匹配待后续阶段 |
| 依赖 / Graphviz | 已解析 ref / dependsOn 内存数据并诊断引用一致性、自依赖和重复；依赖图分析、路径与图展示尚未实现 |
| 漏洞数据 | 获取、导入、缓存漏洞及利用信息 |
| 候选匹配 / 适用性 | 识别候选漏洞，再判断当前组件版本是否受影响 |
| 风险 / 解释 | 输出风险分数或等级、处理优先级及理由 |
| Dashboard | 展示风险分布、高风险项、统计和趋势 |
| 扫描 / 比较 / 报告 | 保存分析结果，比较组件、漏洞及风险变化，生成评估报告 |

Project 已实现最小数据结构及 projects 表。SbomDocument、SbomComponent、SbomDependency、SbomQualityIssue、SbomQualityReport 已有内存模型，不持久化。Scan、Vulnerability、Finding 和 RiskResult 及组件、依赖持久化仍为规划；具体类与数据库结构待对应阶段确定，不提前建空架构。

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
| 01 | 应用基础框架：导航、页面、日志、配置、基础数据库访问 | COMPLETE |
| 02 | 项目管理 | COMPLETE |
| 03 | CycloneDX SBOM 导入 | COMPLETE |
| 04 | SBOM 质量诊断 | COMPLETE |
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

**Phase 01 —— COMPLETE。**

以下为该阶段完成时的成果；当前页面与 schema 以第 16 节为准。

- 正式 MainWindow Application Shell；概览 / 项目 / 设置导航、QStackedWidget 页面切换、选中态和状态栏同步；项目页保留 Phase 02 占位。
- AppPaths 使用 QStandardPaths 应用数据路径；AppLogger 提供本地日志；AppSettings 使用独立 INI 保存和恢复 lastNavigationPage。
- AppDatabase 管理 SQLite 连接和基础初始化；schema_version = 1，唯一非业务表为 app_meta。
- Phase 01 自动测试 Debug / Release 均 7/7 PASS；Phase 00 regression 均 3/3 PASS，原测试保持完整。
- 用户 Debug GUI 人工验收 PASS；Release 自动 Build / Test PASS，本阶段未要求 Release 人工验收。
- 最终维护性检查 PASS；独立 completion commit、main 推送及 `phase-01-complete` annotated tag 封版。

**Phase 02 —— COMPLETE。**

- Project 最小模型、projects 第一张正式业务表；schema_version 从 1 升至 2。
- 全新数据库初始化 schema 2；已有 v1 使用事务迁移，具备失败回滚、冲突表保护和未知版本拒绝，重复打开保持数据。
- ProjectRepository 统一创建、列表、按 ID 查询和真实删除；ProjectPage 提供空状态、创建输入、列表选择、详情和删除确认，项目创建与删除结果在重启后保持。
- Debug / Release Build PASS；CTest 均 22/22 PASS，其中 Phase 00 3/3、Phase 01 7/7、Phase 02 12/12。
- 用户 Debug GUI 人工验收 PASS，包含创建、详情、持久化、排序、删除取消 / 确认和最后项目空状态；Release 为 AUTOMATED ONLY。
- 最终维护性检查 PASS；独立 completion commit、main 推送及 `phase-02-complete` annotated tag 封版。

**Phase 03 —— COMPLETE。**

- CycloneDX JSON 1.4 / 1.5 / 1.6 共同字段子集；安全文件读取、50 MiB 上限、分类错误和失败不返回半完成文档。
- SbomDocument 最小内存模型，元信息、metadata root component、嵌套组件展开及 ref / dependsOn 依赖数据；组件深度上限 128，组件、依赖条目及依赖目标总数各上限 100000。
- 有效 Project 下触发导入，后台解析、GUI 线程交付，只读预览；仅成功后替换结果，失败或取消保留旧预览。
- schema 保持 2，仅 app_meta / projects；无数据库迁移或导入写入，Project 模型未改变，解析结果不持久化。
- synthetic 自动测试新增 15 项，Debug / Release Build PASS、CTest 均 37/37 PASS；原有 22 项回归保留。
- 用户在 `15.md` 确认 Debug GUI 人工验收 PASS，包含原生文件框打开、取消及重新打开；Release 为 AUTOMATED ONLY。
- 维护性与 Sensitive / Privacy Review PASS；独立 completion commit、main 推送及 `phase-03-complete` annotated tag 封版。

**Phase 04 —— COMPLETE。**

- 独立 SbomQualityAnalyzer 只读分析 SbomDocument，仅依赖 Qt Core；issue-based diagnosis 使用稳定 code、scope、位置、中文解释和 Info / Warning / Error，不引入总体分数。
- 实现关键字段缺失、重复 bom-ref / PURL、dependency reference validation、自依赖、重复 dependency entry / target、缺根组件和空组件列表诊断；规则与计数见第 16 节。
- 复用后台导入任务完成解析和诊断；质量 Error 不转为 Parse Failure。成功后交付完整文档和报告，取消或解析失败保留旧成功预览与报告。
- SbomImportDialog 提供组件预览与质量诊断页签、数量摘要、只读问题表及明确 clean state；哈希集合支持近似线性的大输入分析。
- schema 保持 2，仅 app_meta / projects；无 migration 或诊断数据库写入，Project 模型不变，无组件持久化或 auto-fix。
- 新增 Phase04Test 的 11 项 CTest 注册项，Debug / Release Build PASS、CTest 均 48/48 PASS，原有 37 项回归保留。
- 用户在 `17.md` 确认 Debug GUI 人工验收 PASS，包括 clean 0 issues、问题文件 6 issues（Error 2 / Warning 4 / Info 0）及两类 Parser Error 保留旧结果；Release 为 AUTOMATED ONLY。
- 最终维护性与 Sensitive / Privacy Review PASS；本文保留此前授权审计修订并更新本阶段状态，独立 completion commit、main 推送及 `phase-04-complete` annotated tag 封版。

# 15. 当前真实工程结构

正式源码结构：

```text
D:\codex\Graduation Project\project\
├─ .gitignore
├─ CMakeLists.txt
├─ PROJECT-HANDOFF.md
├─ src\
│  ├─ main.cpp
│  ├─ MainWindow.h / .cpp
│  ├─ AppPaths.h / .cpp
│  ├─ AppLogger.h / .cpp
│  ├─ AppSettings.h / .cpp
│  ├─ AppDatabase.h / .cpp
│  ├─ Project.h
│  ├─ ProjectRepository.h / .cpp
│  ├─ ProjectPage.h / .cpp
│  ├─ SbomDocument.h
│  ├─ CycloneDxParser.h / .cpp
│  ├─ SbomQualityAnalyzer.h / .cpp
│  └─ SbomImportDialog.h / .cpp
└─ tests\
   ├─ Phase00SmokeTest.cpp
   ├─ Phase01Test.cpp
   ├─ Phase02Test.cpp
   ├─ Phase03Test.cpp
   └─ Phase04Test.cpp
```

`.git` 为版本元数据；本地 `build-debug/`、`build-release/`、Qt Creator 的 `build/` 和 `.qtcreator/` 均被忽略，不进入正式提交。运行时数据库、日志和配置位于应用数据目录，不属于正式源码；测试数据库位于临时目录，截图和构建日志留在被忽略的构建目录。

人工验收用 `phase03-demo-sbom.json`、`phase03-not-cyclonedx.json`、`phase03-invalid-json.json`、`phase04-clean-sbom.json`、`phase04-quality-issues.json` 由测试生成于构建目录，全部为 synthetic 数据，保持 ignored / not tracked / not staged / not committed。

临时理解报告已按封版授权删除，未进入任何正式提交；唯一长期动态交接文档为本文。

当前应用版本为 **0.5.0**，CMake 与 main.cpp 一致。CMake target 边界：`SbomParsing` 包含 Parser 和 Analyzer，仅链接 Qt Core；`AppFoundation` 链接 Qt Core / Sql；应用链接 Widgets / Concurrent 及这两个库。测试开启时增加 Qt Test，并要求可用的 Graphviz dot；不提前链接 Network / Svg。

# 16. 当前已经实现的功能

当前已经建立 **Phase 00 工程基础 + Phase 01 Application Foundation + Phase 02 Project Management + Phase 03 CycloneDX SBOM Import + Phase 04 SBOM Quality Diagnosis**：CMake / C++20 / Qt Widgets Application Shell、基础导航和页面切换、运行时路径、本地日志、基础设置、SQLite 应用级连接及 schema metadata；项目支持 create、list、findById、delete 和 persistence。当前业务能力为 Project → CycloneDX Parse → Preview + SBOM Quality Diagnosis，组件和质量报告仍仅在内存中保存。

默认运行目录由 `QStandardPaths::AppDataLocation` 决定，Windows 通常为 `%APPDATA%/GraduationProject/SupplyChainRiskAssessment`：数据库 `data/supply_chain_risk.db`、日志 `logs/application.log`、配置 `settings.ini`。可用 `--data-dir <绝对路径>` 指定隔离数据根目录，空路径或相对路径会被拒绝。应用自行保存的 UI 配置仅含 `ui/lastNavigationPage`，启动及切换时保存，重启时恢复，未知页面回退概览。

AppLogger 以追加方式写入 UTC 时间、级别和单行消息，写入失败回退标准错误输出；启动时打开或首次写入失败还会显示警告，并继续运行。不能据此宣称所有后续写日志失败都会弹窗。本地基础日志包含运行目录，项目创建 / 删除日志包含项目 ID；它们仍属私有运行数据，不能把 Phase 03 导入日志的安全摘要约束解释为整个日志可以直接公开。

Project 仅包含 `id`、`name`、`description`、`createdAt`：应用生成无花括号 UUID；名称 trim 后必填、1—100 个 Unicode 码点，允许重名；描述 trim 后可空、最多 500 个码点；创建时间为 UTC Unix 毫秒，界面显示本地时间。列表按 `created_at DESC, id DESC` 排序。创建后自动选中；选择项目显示详情；删除需二次确认，取消不写数据库，删除最后一项恢复空状态。

**当前数据库：schema_version = 2；仅有 app_meta 和 projects。**

```sql
projects (
    id          TEXT PRIMARY KEY NOT NULL,
    name        TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    created_at  INTEGER NOT NULL
)
```

AppDatabase 在同一事务中完成全新 schema 初始化或 v1 → v2：先建 projects，再更新版本，成功后一并提交；失败回滚，冲突表保留原数据并拒绝迁移，未知版本拒绝打开，不自动降级或重建用户数据。测试使用 QTemporaryDir，真实程序冒烟通过 `--data-dir` 使用临时根目录，不污染真实数据。

**CycloneDX 支持范围：1.4 / 1.5 / 1.6 的共同字段子集，不是完整 Schema validator。** 读取 bomFormat、specVersion、serialNumber、BOM version、metadata timestamp / root component、components 和 dependencies。SbomComponent 包含 bomRef / type / name / version / purl；SbomDependency 包含 ref / dependsOn。嵌套 components 按顺序展开，metadata root 单独保存且不计入组件数，其子组件参与展开；不从层级隐式生成依赖。缺失 version / purl / bom-ref 等允许为空，PURL 保持原值；解析成功后由独立 Analyzer 诊断质量问题。

CycloneDxParser 接收 QByteArray 或本地文件，输出 SbomParseResult / SbomDocument，仅依赖 Qt Core，不依赖 UI、SQLite 或 AppLogger。文件有界读取，集中限制为 50 MiB、组件嵌套深度 128、组件数 / dependency entries / dependency targets 总数各 100000。深度按收集 components 数组计算，metadata root 单独保存；这不是任意 JSON 字段的统一深度上限。50 MiB 限制输入字节数，不代表解析期间进程内存仅占 50 MiB。错误区分 FileOpenError、FileReadError、FileTooLarge、InvalidJson、NotCycloneDx、MissingSpecVersion、UnsupportedSpecVersion、InvalidStructure、StructureLimitExceeded；失败不返回半完成 Document。

必需头字段为 bomFormat = CycloneDX 与受支持的字符串 specVersion。共同子集的可选字段缺失时保留空值；字段存在但类型错误（包括 null）会失败，未知字段忽略。BOM version 若提供，须为 1 至 9007199254740991 的整数；不校验 PURL 语义、UUID、时间戳格式或依赖引用有效性。

ProjectPage 在触发时重新验证选中 Project，打开窗口模态的 SbomImportDialog。文件选择使用 Windows 原生 QFileDialog；Qt Concurrent 后台任务只捕获文件路径值，先调用 Parser，成功后调用 Analyzer，返回含解析结果与质量报告的 ImportResult，不访问 UI、数据库或 logger；QFutureWatcher 在 GUI 线程交付结果。QObject 所有权及连接上下文保证窗口销毁后不再向它交付结果；关闭窗口不会主动取消已启动的解析或诊断，后台任务继续结束，应用退出可能等待线程池完成。进行中的导入不接受重入，未增加第二套线程机制。

成功状态仅在候选文件解析成功且质量分析正常返回完整报告后更新；Quality Error 仍是成功导入。取消、Invalid JSON、non-CycloneDX、unsupported version 或文件读取错误均保留旧 SbomDocument、SbomQualityReport 和预览；首次失败保持空状态。这里的分析成功指正常完成诊断，不要求 issues 为空，不表示已实现独立的分析错误分类或内存耗尽恢复。

组件预览页签包含文档摘要和只读组件表，摘要显示文件名而非完整路径。组件单元格及摘要中的 serialNumber / 根组件名称 / 时间戳超过 512 个 UTF-16 单元时在显示层截断并加省略号，内存模型保留原文。质量诊断页签显示 Total / Error / Warning / Info 计数和 Severity / Code / Location / Description 问题表；位置使用从 1 开始的组件、依赖及目标编号或独立的元数据根组件位置，不依赖唯一 name。两张表均以 QAbstractTableModel 按需提供可见单元格，支持滚动；默认 900×600 和最小 560×400 布局已验证。clean state 明确显示“当前诊断规则未发现问题。”，不宣称 SBOM 完全正确。

关闭窗口即结束此次预览与报告，重新打开为空；应用业务层不持久化导入路径或历史，此结论不涵盖系统文件选择框自身的最近位置记录。依赖仅有条目数量及质量问题定位，没有依赖明细表或关系图。导入与诊断日志仅记录固定状态、错误类别、组件 / 依赖数量及 errorCount / warningCount / infoCount，不记录 SBOM 原文、组件清单、PURL、bom-ref、完整路径、依赖图或整个 issue list；既有基础日志仍遵循前述私有运行数据边界。

**Quality Model：issue-based diagnosis，无总体质量分、A/B/C grade 或漏洞风险等级。** SbomQualityIssue 使用 enum class code、scope 和从 0 开始的 componentIndex / dependencyIndex / targetIndex（不适用为 -1）；severity、codeName、中文 message 从统一规则派生，避免规则与解释分叉。SbomQualityReport 通过只读接口暴露 issues 和严重程度计数，计数在后台生成；SbomQualityAnalyzer 接收 `const SbomDocument&`，不依赖 UI、SQLite、Project、AppDatabase 或 AppLogger，不改写输入。

| Severity | 正式 Issue Code | 含义与理由 |
| --- | --- | --- |
| Error | MissingComponentName | 无法可靠展示或识别组件 |
| Error | DuplicateBomRef | 引用无法唯一定位对象 |
| Error | MissingDependencyRef、UnknownDependencyRef | 无法确定或解析依赖来源 |
| Error | EmptyDependsOnRef、UnknownDependsOnRef | 无法解析依赖目标对应的边 |
| Warning | MissingComponentVersion、MissingComponentPurl | 降低版本适用性判断或软件包身份识别可靠性 |
| Warning | MissingBomRef、MissingComponentType | 缺少关联或分类依据；不据此认定解析失败 |
| Warning | DuplicatePurl | 完全相同的 PURL 可能是重复记录，不认定业务上绝对非法 |
| Warning | SelfDependency、DuplicateDependencyRef、DuplicateDependsOnTarget | 提示自引用、重复条目或重复边，保留原始数据 |
| Warning | NoComponents | components 列表为空，不含单独保存的 metadata root |
| Info | MissingMetadataComponent | 缺少 SBOM 所描述主体的元数据说明 |

metadata root 与展开后的组件均执行字段检查并参与强标识重复检查；非空 bom-ref 组成 known reference set。空字符串或纯 whitespace 按缺失处理，不计重复或自依赖；其他非空标识按原始字符串精确比较，不 trim、case-fold、normalize 或 fuzzy match，不凭 name / version 判断重复。每个重复组从第二次出现起逐次报告；未知目标与自依赖、重复目标可以在同一位置分别报告。serialNumber / timestamp 缺失不产生问题。稳定输出顺序为文档提示、metadata root、展开组件、dependency entries 及 targets 的输入顺序，不遍历哈希集合生成输出。Analyzer 只诊断，不补字段、生成 bom-ref、合并、删除或去重。

重复和引用检查使用 QSet，平均复杂度接近 O(components + dependency entries + dependency targets)，另计字符串处理和 issue 存储；无全量两两比较。Phase 04 开发验收的本机记录：100000 组件 + 100000 依赖条目 + 100000 目标的 clean 分析 Debug 169 ms / Release 55 ms；同规模生成 799999 issues 为 Debug 92 ms / Release 25 ms。测试另覆盖 100000 组件生成 500000 行问题的 UI 和导入期间关闭窗口。这些是当次运行结果，不是性能保证；封版只运行正常回归中已有的大输入测试，不另作 benchmark。

**Phase 03 / 04 没有 DB migration，导入和质量诊断不写数据库，Project 不保存 sbomPath。** 仍仅有 app_meta / projects，无 sboms / components / dependencies / quality_reports / quality_issues / Scan / Vulnerability 等业务表；组件持久化、依赖图分析、漏洞分析、风险计算、扫描历史和报告尚未开始。

# 17. 当前自动测试状态

**Phase 00 / 01 / 02 / 03 Regression：PASS。Phase 04 Automated Acceptance：PASS。**

下表为 2026-09-16 Phase 04 封版时，在本文更新后重新执行的正式工程结果。早期 Phase 的当时验收记录保留在第 14 / 18 节，不与本次自动回归混淆；本次未重新运行历史完整环境审计或替代用户 GUI 人工验收。

| 正式工程验证 | 结果 |
| --- | --- |
| Debug / Release Build | 均 PASS（Phase 04 封版，交接文档更新后重新执行） |
| Debug CTest | 48/48 PASS |
| Release CTest | 48/48 PASS |
| Phase 00 Regression：Qt / C++20、SQLite、Graphviz SVG | Debug / Release 均 3/3 PASS |
| Phase 01：路径、配置、数据库、未知 schema、日志、导航、真实程序启动关闭 | Debug / Release 均 7/7 PASS |
| Phase 02：schema 初始化 / 迁移 / 回滚 / 冲突保护、项目读写 / 校验 / 排序 / 持久化、UI 和错误处理 | Debug / Release 均 12/12 PASS |
| Phase 03：版本 / 元信息 / 嵌套组件 / 依赖、错误 / UTF-8 / 安全边界 / 文件读取、预览原子性 / Project 集成 / 大预览 / 异步关闭 | Debug / Release 均 15/15 PASS |
| Phase 04：clean / 字段缺失 / 重复标识 / 空文档 / 依赖规则 / 确定性与计数 / 文档不变 / Parse 与 Quality 分离 / UI / 大输入 / 数据库不变 | Debug / Release 均 11/11 PASS |
| GUI smoke | PASS；真实程序隔离启动和关闭、项目与导航回归、只读组件与质量预览、正常 / 最小尺寸 / 超长字段、失败保留旧结果、10000 组件预览 / 500000 问题行及异步关闭检查通过 |
| 最终维护性检查、git diff --check | PASS |

测试源码为 `tests/Phase00SmokeTest.cpp` 至 `tests/Phase04Test.cpp`（Phase 00 文件名含 Smoke）。CTest 保留 Phase00 的 3 项、Phase01 的 7 项、Phase02 的 12 项和 Phase03 的 15 项，新增 Phase04 的 11 项。Phase04 的 missingFields 和 dependencyRules 包含数据驱动用例；本阶段仅将 Phase03 三处通用表查找改为按 sbomComponents 名称定位，原有断言保持完整。测试使用 synthetic 数据与 QTemporaryDir；Phase04 在 clean / 问题分析和 UI 导入前后检查 schema / tables、两表全部内容、SQL total_changes 和 Project 字段不变，并核验日志仅有安全摘要。上述为正式工程自己的结果，区别于历史环境审计。

文件选择自动测试在测试进程内临时启用 AA_DontUseNativeDialogs，并通过作用域清理恢复：Phase 03 开发时自动快速开关原生框曾在 Qt Windows 平台线程出现崩溃，因此这项自动测试覆盖的是 Qt 控件文件框与导入入口连接，不覆盖原生框实现。生产程序仍使用原生框，其打开、取消、重新打开已由用户在 `15.md` 和 `17.md` 人工确认 PASS；尚不能据此声称所有原生框异步关闭场景均经自动验证。未用 QTimer / sleep 延迟补丁替代生命周期处理。

回归命令：`ctest --test-dir build-debug --output-on-failure` 和 `ctest --test-dir build-release --output-on-failure`，使用冻结 CMake 目录中的 ctest.exe。CTest 为子进程设置 Qt / MinGW DLL 搜索路径，不修改系统 PATH。

已有构建目录的 PowerShell 操作（在正式根目录执行，PATH 仅作用于当前进程）：

```powershell
$env:PATH = 'D:\program\Qt\Tools\mingw1310_64\bin;D:\program\Qt\6.11.2\mingw_64\bin;' + $env:PATH
& 'D:\program\Qt\Tools\CMake_64\bin\cmake.exe' --build build-debug --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
& 'D:\program\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Debug tests failed' }
& 'D:\program\Qt\Tools\CMake_64\bin\cmake.exe' --build build-release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
& 'D:\program\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Release tests failed' }
```

构建目录不存在时，先使用冻结 cmake.exe 执行配置：`-S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=D:/program/Qt/Tools/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/program/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_PREFIX_PATH=D:/program/Qt/6.11.2/mingw_64 -DBUILD_TESTING=ON`；Release 对应改为 `build-release` / `Release`，检查成功后再构建。已有缓存的生成器或工具链不符时先核对原因，不直接覆盖。GUI 用例需要可用的 Windows 桌面会话，不能把无显示环境中的结果等同于人工验收。

# 18. 当前人工验收状态

- 项目理解审查：**PASS**，依据用户提供的 `03.md` 中 ChatGPT 审查结论。
- 本文前版审查：**PASS**，依据用户提供的 `05.md`。
- **Phase 00 Manual Acceptance：PASS**，由用户实际操作后在 `05.md` 中确认。
- Phase 00 Debug GUI：**PASS**；Release GUI：**PASS**。
- 用户确认标题和 Phase 00 内容正确，窗口可移动、调整大小和关闭，无闪退、DLL / Qt platform plugin 错误或明显卡死。
- **Phase 01 Manual Acceptance：PASS**，由用户实际操作后在 `09.md` 中确认；Debug GUI：**PASS**。
- 用户确认正式 Shell、概览 / 项目 / 设置导航、页面切换、选中态、状态栏、窗口缩放、Phase 02 项目占位、设置偏好保存及 lastNavigationPage 恢复正常，关闭 / 重启正常，无崩溃、明显卡死、DLL / Qt plugin 或数据库初始化错误。
- Phase 01 Release GUI：**NOT REQUIRED / AUTOMATED ONLY**；本阶段仅要求 Release 自动 Build / Test，未要求用户再次人工验收。
- **Phase 02 Manual Acceptance：PASS**，由用户实际操作后在 `11.md` 中确认；Debug GUI：**PASS**，Release：**AUTOMATED ONLY**。
- 已确认创建、自动选中、详情、关闭 / 重启持久化、第二项目排序及选择切换、删除取消 / 确认、删除后重启不恢复、最后项目空态、空名称 / 纯空格拒绝和输入边界反馈正常。
- 概览 / 项目 / 设置导航、lastNavigationPage 恢复及窗口缩放正常；无崩溃、数据库错误、明显卡顿或 DLL / Qt plugin 错误。
- **Phase 03 Manual Acceptance：PASS**，由用户实际操作后在 `15.md` 中确认；Debug GUI：**PASS**，Release：**AUTOMATED ONLY**。
- 已确认应用启动、项目选择与导入入口、Windows 原生文件框打开 / 取消 / 再次打开正常；synthetic CycloneDX 显示 specVersion 1.6、BOM version 7、3 components、3 dependencies、demo-app root，Name / Version / Type / PURL / bom-ref 只读表及中文布局正常。
- non-CycloneDX 明确提示 bomFormat 必须为 CycloneDX，invalid JSON 错误正常；失败后保留旧成功预览且 Project 不受损。项目创建 / 删除、概览 / 设置导航、窗口缩放及关闭 / 重启正常，无崩溃、明显卡死、数据库或 DLL / Qt plugin 错误。
- **Phase 04 Manual Acceptance：PASS**，由用户实际操作后在 `17.md` 中确认；Debug GUI：**PASS**，Release：**AUTOMATED ONLY**。
- clean synthetic SBOM 解析成功，4 components，Total / Error / Warning / Info 均为 0，正确显示“当前诊断规则未发现问题。”。
- quality-issues synthetic SBOM 解析仍成功，4 components，Total 6 / Error 2 / Warning 4 / Info 0；下表六项代码、严重程度、位置和中文解释均显示正常，Quality Error 没有转为 Parse Failure。

| Code | Severity | 人工确认位置 |
| --- | --- | --- |
| MissingComponentVersion | Warning | Component #2 |
| MissingComponentPurl | Warning | Component #3 |
| DuplicateBomRef | Error | Component #3 |
| MissingBomRef | Warning | Component #4 |
| UnknownDependsOnRef | Error | Dependency #2 / target #1 |
| SelfDependency | Warning | Dependency #2 / target #2 |

- 随后分别导入 non-CycloneDX 和 invalid JSON，均明确提示对应 Parser Error，前一份成功的 4 components、6 quality issues 及 Error 2 / Warning 4 / Info 0 完整保留；成功状态原子性人工验收 PASS。
- Windows 原生文件框打开 / 取消 / 重新打开、项目创建 / 查看 / 删除、概览 / 项目 / 设置导航、缩放、关闭 / 重启均正常；无崩溃、明显卡死、数据库或 DLL / Qt plugin 错误。

# 19. 当前重要技术决策

- 使用 C++20、Qt Widgets、CMake、SQLite、Graphviz，继续冻结环境；Qt Network / JSON / Concurrent / Test 按实际需要使用。
- SBOM 优先 CycloneDX JSON，SPDX 留作可选扩展；匹配优先依赖软件包身份和版本证据。
- GUI、应用逻辑、领域与基础设施可作职责划分，但不强制提前建满架构层。
- 一次只做一个 Phase；每个文件须有实际职责，不创建未来模块、无用接口或 Factory / Adapter / Manager 空架构。
- 稳定代码不无故重构，自动测试不能替代用户 GUI 人工验收。
- 仅维护本文作为长期动态交接文档，不新增多套状态文件。
- SQLite 每线程独立连接、主线程更新 GUI，密钥不进入代码、日志及 Git。
- MainWindow 仅承担 Application Shell、导航和页面协调；QStackedWidget 是当前页面的唯一状态来源，数据库、日志和配置内部职责独立，启动组合由 main.cpp 完成。
- 运行数据默认使用 QStandardPaths::AppDataLocation；QSettings 使用独立 INI，仅保存轻量 UI preference；测试显式使用临时路径。
- SQLite schema version 当前为 2；app_meta 保存版本，projects 保存项目。v1 → v2 使用事务迁移；AppDatabase 在调用线程管理连接，借用的连接句柄和查询须先于连接关闭释放。
- Project 使用 UUID 作为稳定 ID，名称可重名；列表固定按 `created_at DESC, id DESC` 排序，关联和删除均使用 ID。
- ProjectRepository 统一项目校验和 SQL；MainWindow / ProjectPage 不执行 SQL。ProjectPage 负责输入、选择、详情和删除确认，SQLite 是项目数据的持久化来源。
- projects 当前采用真实 DELETE；尚无子记录关系，因此不引入 soft-delete。main.cpp 组合已有数据库、日志、Repository 和页面，并在关闭连接前销毁页面和 Repository。
- Phase 03 / 04 的解析文档与质量报告仅 in-memory；Parser 负责读取、JSON、结构和安全限制，独立 Analyzer 只读诊断字段及引用质量。Quality Error 不等于 Parse Failure，Project 模型不保存 sbomPath，不建立 Component persistence 或依赖图分析。
- Phase 04 使用 issue-based diagnosis、稳定 code / scope / 位置、中文解释及 Info / Warning / Error；暂无总体质量分，不采用任意权重、A/B/C grade 或漏洞风险等级，不负责 auto-fix。缺少 bom-ref 表示关联依据不足，定为 Warning；明确引用冲突或无效引用定为 Error。
- Analyzer 不修改 SbomDocument，使用哈希集合进行重复和 reference checking；非空强标识精确比较，输出按输入顺序确定，不模糊匹配或规范化 PURL，不凭 name / version 合并组件。
- 后台解析与诊断复用一个值捕获任务；GUI 线程接收完整结果并使用已有 AppLogger 记录安全摘要。只有 parse success 且 quality analysis 正常返回后才替换成功预览与报告；取消和 Parser Error 保留旧状态，窗口生命周期决定两者生命周期。质量报告不持久化、不写数据库。
- 真实 SBOM 属于 PRIVATE RUNTIME DATA；Phase 03 / 04 自动与人工验收仅使用 synthetic 数据。人工 JSON、数据库、日志、截图和构建产物均不纳入 Git；本次封版 staged 内容及待推送 commit 已通过 19.7 隐私审查，后续提交仍须重新审查其实际 diff。

**AI 分工：** ChatGPT 负责规划、Phase、Prompt、方案、审查及验收设计；Codex 负责读取真实工程、实现、编译、自动测试和 Git 检查；用户负责 GUI 操作、人工验收和决定是否进入下一 Phase。

**统一阶段闭环：** 明确 Phase → 读取真实工程 → 实现 → Build / 自动 Test → 用户人工验收 → 确认 PASS → 更新本文并完成最终回归、维护性及隐私审查 → 按授权 commit / push / tag → 核验远端与工作区 → 报告完成，等待下一阶段指令。Git 结果必须实查后报告；文档需要修正时如实维护，不以预期代替完成。个别阶段有特殊顺序时，以具体授权为准。

## 19.1 代码质量与职责边界

- 修改须同时保证当前正确性、回归安全和下一次维护的可理解性。最小修改是解决根因所需的最小完整范围，不是行数最少；禁止补丁叠加、特判／flag 堆积、隐藏副作用和重复业务实现。
- 每项能力有明确职责，每个业务状态尽量只有一个权威来源。UI 缓存或派生状态须明确来源、失效条件和更新责任，不能成为第二份业务真相或靠人工同步。
- 优先集中容易分叉的业务不变量、校验及状态规则；少量相似 UI 代码不必强行抽象。只有真实复杂度出现后才引入共享组件，不追求最大抽象或机械 DRY。
- 按职责、业务规则、生命周期和数据所有权拆分，不按固定行数拆文件。独立职责混杂、不相关状态增多、修改原因不同、测试依赖过多或隐含状态难定位，才是检查拆分的信号。
- 命名表达职责和业务含义；注释解释原因、不变量、兼容需求及不可随意改变的边界，不逐行翻译代码。需要长篇注释解释时先检查结构与命名。
- 错误处理须保留诊断信息并转换为调用方能理解的结果，给 UI 安全、清晰的反馈；不吞错、不把不同原因统归为同一个失败、不继续使用不完整状态，也不向用户泄露敏感信息或无关内部细节。

## 19.2 修改前的真实代码审查

实现或修 Bug 前，读取任务相关入口、调用方／被调用方、数据流与状态所有者、数据库存取、已有工具与基础类、测试、CMake 依赖及错误传播路径。先明确：能力现在由谁负责、已有实现和扩展点是什么、有无重复逻辑、哪个对象是权威状态源、新职责应归谁、哪些回归必须保持通过。

优先扩展已有正确职责，延续真实工程中更简单合理的结构。已有数据库、配置或领域规则入口时，不新增平行体系，不在页面散落配置访问或复制领域判断。Prompt 中的类名和目录建议不是机械创建指令。

## 19.3 Bug 修复、重构与测试

- 先建立可复现测试或稳定场景，定位状态所有权、对象生命周期、数据一致性、调用顺序或错误传播等根因，在产生错误的边界修复，再验证问题消失和原回归通过。
- 不以额外刷新、flag、QTimer 延时、sleep、重复请求或重试掩盖状态问题；只有延时／重试本身属于明确设计且有可验证依据时才使用。
- 禁止无关重构不等于禁止修改旧代码。当前需求暴露职责混杂、规则重复、状态归属不清、难以测试或只能继续加特判时，允许并在必要时进行范围内的小型安全重构：先理解原行为，保持原测试通过，为改变的边界补测试，不改变无关行为、不借机改造全项目。大范围调整按 19.4 暂停。
- 新行为、Bug 修复和重构都须评估测试需求，提供对应自动测试或明确验证；不得为通过测试削弱业务约束、删除重要断言、跳过失败路径或迎合错误实现。
- 新实现替代临时逻辑后，确认无真实调用和兼容需要，再删除旧特判、flag、无效分支、临时 debug 输出和注释掉的大段旧实现；Git 保存历史，不长期保留死代码。

## 19.4 高风险问题暂停与询问

以真实代码和已有授权判断。下列问题不能由当前明确范围安全解决时，停止受影响修改并向用户确认：

- Prompt 与工程明显冲突，或继续实施将与已有架构形成两套体系。
- 持久化结构需要变化但兼容／迁移不明，或业务语义的不同解释会改变数据模型、状态机。
- 存在两种以上合理方案且长期架构影响显著，或继续沿用已发现的设计缺陷很可能产生明显技术债／返工。
- 会破坏安全、数据一致性或线程边界；需要尚未明确授权的不可逆／破坏性操作；必须调整冻结环境但原因或影响不清。
- 为继续开发必须进行大范围重构或架构调整。

暂停时提供问题及文件／类／函数位置、当前真实实现、与任务的冲突、至少两个可选方案及各自利弊、推荐方案和理由；等待用户确认，不继续实施待决修改。已有明确授权不重复索取。普通命名、局部风格、代码可直接判断的问题、普通编译错误和局部明确 Bug，能安全解决就继续，不频繁打断。

## 19.5 C++ / Qt 专项维护规则

- QObject 优先采用清楚的 parent-child 生命周期，非 QObject 资源优先 RAII。允许非 owning raw pointer，但必须明确所有者和有效期，避免不明释放责任。
- MainWindow 主要负责窗口 UI 组织、页面协调和交互连接。配置、数据库、解析、漏洞及风险职责在对应功能实际出现时交给明确组件，不堆入 MainWindow，也不提前建空类。
- 网络、大量解析／计算、长时间数据库任务不得阻塞 GUI 主线程；需要异步时明确后台执行与结果返回方式，GUI 更新回主线程。各数据库线程使用自己的连接，QSqlDatabase / QSqlQuery 不跨线程共享或传递。
- signals / slots 须能追踪发出者、处理者、重复 connect 风险及销毁后的生命周期安全，避免循环触发和隐式反馈链。
- QSettings 只保存配置和轻量 UI 偏好，正式业务数据交由 SQLite 等明确持久化层。schema 真正变化时在对应 Phase 明确版本、兼容与迁移方案，不在启动时偷偷进行不可追踪的破坏性修改。
- CMake 只加入当前代码需要的 Qt 模块和依赖，不提前链接未来 Phase 的组件。

## 19.6 完成前检查与交接维护

输出 `READY FOR MANUAL ACCEPTANCE` 前，人工审查当前 diff，并确认：无重复业务实现／平行体系，无死代码、临时 debug 输出或大段注释旧实现；新文件和类有本阶段用途；状态与对象所有权清楚，错误路径和资源释放合理；相关回归通过，新行为有测试或明确验证；没有超出当前 Phase 范围的改动或无关大重构，`git diff --check` 通过。必要测试中的诊断输出不等于临时调试残留。功能虽能运行但已明显堆积不可维护补丁时，应先在当前合理范围整理，不能直接交付验收。

交接文档也须就地更新对应章节，只保留重要成果、Git 基线、测试、人工验收、决策、风险和下一步；不追加与正文冲突的新状态，不积累普通探索、小修或重复测试流水账。新 Codex 应凭本文、真实代码和 Git 即可安全接手，不需要第二套规范文档。

正式封版前必须按 19.7 完成 **Sensitive / Privacy Review**：逐项确认 staged 内容均应公开，无 secrets、真实个人隐私或用户业务数据、运行时数据库、真实用户 SBOM、日志及临时诊断数据、本机私有配置，且未强制绕过 `.gitignore`。该检查与测试、维护性审查及 `git diff --check` 一同执行，尤其适用于 Phase 03 及后续阶段。

## 19.7 隐私、敏感信息与 Git 安全边界

**公开边界：** 当前仓库 `yyyyyyqqqqq/Graduation-Project` 为 **Public**。所有 tracked 内容均应视为可能被公众立即读取；push 后即可公开，不能依赖仓库无人关注作为保护。含敏感信息的内容禁止进入 staging area、commit history、GitHub、tag 所引用的内容或 release artifact；只有用户明确确认该内容公开、安全且应纳入仓库，才可作为例外。

- **凭据与秘密：** API Key（特别是未来的 `NVD_API_KEY`）、Access / Refresh / Session Token、密码、Secret、SSH / 证书等私钥、OAuth credential、数据库密码、Cookie 均默认禁止提交。有效、失效、测试、staging、production 凭据一视同仁，不能以“只是测试”放行。公开配置仅使用空值或 placeholder，例如 `.env.example` 中的 `NVD_API_KEY=`，不得放入真实值。
- **个人隐私与用户数据：** 真实姓名、邮箱、电话、账号／用户 ID、聊天、地址、身份／认证信息、私人备注及其他真实用户数据默认禁止提交。测试优先使用人为构造的 synthetic / fixture data；复现问题应模拟数据形状、边界条件和错误状态，无法安全脱敏的真实数据仅用于本地诊断。
- **项目与 SBOM：** 用户真实导入的 CycloneDX / SPDX SBOM、依赖文件及本地项目资料属于 **PRIVATE RUNTIME DATA**，可能暴露内部组件、私有包、版本、源码路径、供应商和内部结构。不得为调试复制进仓库、加入 tests fixture 或执行 add / commit / push；改名不等于脱敏。仓库示例仅使用经隐私审查的公开来源或 synthetic fixture。
- **数据库：** `supply_chain_risk.db` 及其他 `.db` / `.sqlite` / `.sqlite3` 运行时数据库和伴随文件默认禁止提交；其中的项目、组件、扫描、风险、路径及输入均属运行数据。测试数据库使用 `QTemporaryDir` 或专用隔离测试 fixture，结束后清理；清理仅针对测试产物，不删除真实用户数据库。需入库的 fixture 必须为公开安全的构造数据。
- **日志与诊断：** `application.log` 和其他 `*.log` 默认只用于本地诊断，不进入 Git。确需公开测试日志片段时，先脱敏并确认不含凭据、隐私、绝对路径或真实项目数据，再按公开边界审查。源码不得默认记录完整 API Key、Authorization header、token、密码、用户文件或私有 SBOM；诊断仅记录错误类别、状态码、安全摘要等最小必要信息。
- **路径与本机配置：** 避免写入不必要的真实用户目录、桌面／私人／公司路径、用户名、私有盘符结构及下载目录。正式开发路径可作为交接事实记录，业务代码使用 `QStandardPaths`、相对路径或配置，不依赖该绝对路径。`.env`、`local.ini`、`secrets.ini`、`private.json`、`credentials.json`、`config/local.*`、`config/private.*` 等私有配置必须默认忽略；公开仓库仅保留安全的 example / template / placeholder。
- **报告与生成物：** 扫描／风险报告、Graphviz SVG、JSON / CSV / PDF 导出、缓存、临时下载、网络响应和调试输出默认属于 runtime / generated artifacts，不能因位于工程目录就提交。仅明确作为公开测试 fixture、demo resource 或正式文档素材，且通过隐私审查后，才可纳入 Git。
- **暂存与提交审查：** 不默认使用 `git add .`；add 前审查候选文件及隐私，使用明确文件列表或已审查路径。stage 后执行 `git status`、`git diff --cached --stat`、`git diff --cached`，逐文件检查内容与公开用途。commit / push 前复核 staged 文件及敏感信息；push 还须检查待推送 commits，不能因暂存区为空就跳过。至少检查 `API_KEY`、`TOKEN`、`SECRET`、`PASSWORD`、`PRIVATE KEY`、`Authorization`、`Bearer`、`credential`，并结合文件类型、内容与用途识别真实数据、数据库、日志、SBOM、私有配置、临时文件和报告；关键词零命中不等于安全。
- **忽略规则的限度：** `.gitignore` 只是第一道保护，不是安全证明，也不会自动保护已 tracked 的文件。不得以 `git add -f` 绕过规则提交私有内容；`git status clean` 不代表 ignored 文件可公开，打包、分享、上传和发布时仍须审查这些文件。
- **当前忽略基线：** 已覆盖运行时 SQLite 数据库及 journal / WAL / SHM 伴随文件、明确的私有配置文件名与 `config/local.*` / `config/private.*`；允许安全的 `.env.example`，不整体忽略 `.sql`、`.ini`、`.json`、`.svg`、`.csv` 或 `.pdf`，以保留公开模板、synthetic fixture 和文档资源。真实导入及输出目录待对应 Phase 按实际结构隔离，不提前创建。
- **发现待提交敏感内容：** 停止该文件的 staging / commit；优先将敏感内容移出 tracked 文件或替换为空值／placeholder / example，必要时在获准范围内补充 `.gitignore`，保留真实用户数据供本地使用。不能以“马上会删”或“还没 push”为理由提交。
- **已进入历史的秘密：** 立即停止继续传播并报告用户，判断凭据是否真实有效，优先安排 rotation / revoke，检查 Git 历史及公开暴露范围，再决定历史清理；删除当前文件并再提交并不能清除历史泄露。未经用户明确授权，不得 force push、rewrite history、使用 BFG / filter-repo 或删除远程历史；历史重写按 19.4 停止并询问。

# 20. 当前待定事项

- NVD API Key 当前申请状态、漏洞同步与缓存策略、离线演示数据范围。
- 后续 CycloneDX 格式支持扩展、首期组件生态及质量诊断规则扩展；当前共同字段子集固定为 1.4 / 1.5 / 1.6，Phase 04 已实现第 16 节列出的 16 条规则。
- PURL / CPE 规范化、版本范围比较、匹配和适用性证据表达。
- 正式风险公式、阈值、权重、解释、聚合和评估方法。
- 扫描快照粒度、跨扫描组件身份、漏洞库更新与组件变化的比较规则。
- Graphviz 最终部署、报告格式、全新电脑验证及是否引入 CI。

以上事项在对应 Phase 决定，不要求当前阶段提前解决。

# 21. 当前已知问题 / 风险

- **Phase 04 Blocker = None。** 代码、Parse / Quality 边界、失败原子性、数据库不变验证、自动测试、用户 Debug 人工验收、维护性及隐私检查、Git 封版均已通过。
- optional Vulkan Headers 缺失仍为 non-blocking warning，不影响当前 Qt Widgets 工程；无需据此安装额外组件。
- 本次核对未发现正式目录冲突或错误 origin，Phase 04 main / tag push 及远端核验成功；Qt Creator 本地配置和构建目录继续忽略并保留。
- Windows 原生文件框已有用户人工验收 PASS；自动快速关闭场景的测试限制见第 17 节，后续若修改文件框生命周期，应针对原生框重新验证。
- 项目管理及 SBOM 导入 / 解析 / 只读预览 / 规则化质量诊断已实现；组件持久化、依赖图分析、漏洞和风险业务尚未开始，不要把当前能力与未来完整系统混同。
- 后续主要技术风险是身份／版本误匹配、缺失数据产生错误确定结论、评分缺乏依据，以及范围扩张造成过度设计；应在相应阶段验证。
- 父目录旧资料包含重复版本；后续以本文当前状态和更高优先级实查证据为准。

# 22. 下一步

**READY FOR PHASE 05 —— 组件管理与持久化。Phase 05 尚未开始（NOT STARTED）。**

总体目标：组件管理与持久化；具体数据模型、迁移、交互范围和设计等待下一阶段指令。

Phase 04 正式封版已完成；等待 Phase 05 正式开发指令，本轮未开始组件持久化或其他后续业务。

# 23. 新 Codex 接手规则

1. 先阅读本文及当前阶段具体指令，再读取真实代码、Git 和必要的 GitHub 状态；按 19.2 定位调用链、状态来源、已有实现和扩展点，不凭历史文字假定实现存在。
2. 如实区分已实现、已验证和规划；发现冲突按本文顶部真实性优先级处理，并更新相关状态。
3. 延续已有简单合理结构，不因 Prompt 出现类名或目录建议就机械创建；只执行授权阶段，同时考虑功能正确性、回归风险和后续可维护性。
4. Bug 优先修根因，避免 workaround 链；允许有测试保护、与当前需求直接相关的小型重构。大范围架构调整及 19.4 所列高风险问题先暂停，提供方案并获得用户确认。
5. 实现后完成适当构建、测试及 19.6 的 diff／维护性检查，清楚记录结果与限制；阶段通过必须包含用户要求的人工验收。
6. 未经允许不得 `reset --hard`、`clean -fd`、force push、rebase、删除 branch / tag、删除用户文件或重写 Git 历史；发现已有错误 remote 先报告，不自行覆盖。身份配置优先仓库级，冻结环境不无故变更。
7. 完成阶段后按 19.6 就地更新本文，不另造动态管理文档，不追加重复状态或全过程日志。
8. 长期工程规则持续生效；`17.md` 授权的 Phase 04 最终封版已完成，`phase-00-complete` / `phase-01-complete` / `phase-02-complete` / `phase-03-complete` 保持原位置，`phase-04-complete` 为新的稳定基线。Phase 05 尚未开始，必须等待用户下一条正式开发指令。
9. 任何 git add / commit / push 前均须按 19.7 审查候选／staged 文件及 privacy / sensitive information，push 同时检查待推送 commits；不得将真实 runtime database、SBOM、日志、用户数据、私有配置、API Key、token 或 credential 加入 Git。`12.md` / `13.md` 的隐私规则及 `.gitignore` 维护已独立提交并推送，其规则继续适用于本次及后续封版。
