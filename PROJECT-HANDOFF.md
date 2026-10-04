# 毕业设计项目总交接文档

更新日期：2026-10-05（Asia/Shanghai；Phase 13 FINAL SEAL PASS；封版后按用户新授权审查并维护全文，仅更新本文并独立提交推送）

本文是项目唯一的长期动态交接文档。每个阶段结束后，更新当前状态、代码结构、Git、测试、人工验收、技术决策和下一步，避免另建多套状态文件。

本文保留 Phase 00—13 已完成事实及长期规则。Phase 09 按 35.md—42.md 完成证据 enrichment 与缓存可靠性修复封版；Phase 10 按 43.md—46.md 完成 Frozen Design、实现、ChatGPT Code Review、用户授权 GUI 验收及最终封版。Phase 10 是 Exploit-Signal Prioritization MVP，不是完整综合风险模型；Phase 11 已完成冻结验证实验与阈值敏感性分析并封版。Phase 01—11 的 Release GUI 为 AUTOMATED ONLY；Phase 00 的用户 Release GUI 验收 PASS 作为历史事实保留。

Phase 07.5-AUX 已完成只读 MCP 与真实 ChatGPT 连接验收，作为辅助基础设施保留。当前产品已完成 RiskEvidenceProfile → Exploit-Signal Priority Assessment → Decision Evidence Support → Deterministic Explanation、冻结 Validation Experiment、Presentation & Explainability，以及单 Finding 静态 HTML 报告导出。Phase 12 历史 GUI Test 20 的 NOT VERIFIED / NON-BLOCKING 保留；Phase 13 GUI / Report 19/19 PASS，已按 64.md 完成最终回归、唯一 completion commit、main 与 annotated tag 推送。未实现 numeric Risk Score / Risk Level / predictive Assessment Confidence。

依据：各阶段授权（04.md—64.md）、当前真实代码、直接验证证据，以及封版后用户“审查并更新一下整个总交接文档，并提交推送”的新授权。本次仅作独立文档维护，不重新封版或改动实现。Git 固定标识见第 5 节，功能与边界见第 15—16 节，自动测试与人工验收分别见第 17—18 节；历史失败、修复、未验证项和执行归属不因本次维护被覆盖。

**真实性优先级：实际运行结果 > 当前真实代码 > 当前 Git 状态 > 当前 GitHub 状态 > 已验证环境审计 > 历史项目资料 > 推测。** 当前阶段授权以用户最新具体指令为准。

# 1. 项目基本信息

- 题目：**基于 C++ 的软件供应链漏洞风险评估系统的设计与实现**。
- 性质：本科毕业设计，面向软件项目开发者、维护者及答辩演示。
- 定位：使用 C++ 实现能够运行、测试、演示并支撑论文的软件供应链风险评估桌面系统，控制在本科毕业设计合理范围内。
- 实现语言为 C++ 不代表输入组件仅限 C++ 生态；当前身份与漏洞候选匹配支持 PyPI / npm（含 scoped npm）。
- 正式仓库：[yyyyyyqqqqq/Graduation-Project](https://github.com/yyyyyyqqqqq/Graduation-Project)。

# 2. 当前一句话状态

Phase 13 Current Finding Analysis Report Export MVP 已完成 Frozen Design、实现、双配置自动验证、ChatGPT Source-level Code Review 和 GUI / Report Acceptance；最终验收 Test 1–19 全部 PASS，Confirmed Functional FAIL=0、BLOCKED=0、NOT VERIFIED=0。App 0.14.0、schema 4，Phase 11 Rules v1 / 0.90 与冻结科学结果不变。PHASE 13 COMPLETE / FINAL SEAL PASS；completion commit 与 annotated tag 已推送并完成远端核验，固定标识见第 5 节。

# 3. 当前阶段

**CURRENT PHASE: PHASE 13 COMPLETE / FINAL SEAL PASS**

**NEXT: READY FOR FINAL INTEGRATED ACCEPTANCE / THESIS DELIVERY HARDENING REQUIREMENTS / DESIGN。等待新授权，不自动启动 Phase 14。**

59.md 为 Phase 13 冻结设计与实现授权，60.md 为只读源码审查材料，61.md—63.md 为 GUI / Report 验收与补测；64.md 给出最终 ChatGPT Source-level Review PASS、19/19 人工验收及最终混合 provenance，并授权 Final Seal。用户直接澄清优先：User-operated 为 12、13、15、16、17、18；Codex GPT-6 Astra Ultra operated / verified 为 1–11、14、19。历史阶段的验收归属独立保留。

64.md 的 Final Seal 已完成：所有 gates PASS，八个实现文件及本文形成唯一 completion commit，main 与 annotated phase-13-complete 均已普通推送。当前仅按用户新授权维护本文；不修改源码、测试、冻结输入或历史 tag，也不启动下一阶段。

# 4. 正式项目目录

唯一正式根目录：`D:\codex\Graduation Project\project`。

后续源码、CMake、测试、项目文档和 Git 均围绕此目录组织。不维护多个正式工程副本；父目录的 01.md—34.md 为既有准备、设计、开发、封版及辅助 MCP 依据，35.md—42.md 为 Phase 09 依据，43.md—46.md 为 Phase 10 依据；47.md 是 Phase 11 已冻结设计下的正式开发授权；48.md 为审查材料准备，49.md 为首次 Code Review 修复授权，50.md 为自动与 GUI 验证，51.md 为 Phase 11 最终封版授权，52.md 为 Phase 12 冻结设计、正式开发及自动验收授权，53.md 为只读审查材料准备，54.md 为首次审查五项修复，55.md 为历史 UI 测试导航兼容修复及重新自动验证，56.md 为验收执行，57.md 为 Test 16/19 最小修复，58.md 为 Phase 12 最终验收收口与 Final Seal 授权；59.md—64.md 为 Phase 13 冻结实现、源码审查、验收与 Phase 13 正式封版依据。

`D:\codex\SupplyChainRiskAssessment` 是历史环境验证目录，不是正式项目；本轮未检查或修改其内容。

# 5. Git / GitHub 当前状态

Phase 13 已于 2026-10-05 完成 FINAL SEAL PASS，最终封版核验时间为 `2026-10-05T01:40:20.0410721+08:00`。本次独立文档维护开始时再次确认 main / origin/main / remote main 均为下表 completion，ahead/behind=0/0，工作区干净。本文维护提交会使 main 前进，不能将封版时的 main 等式视为永久状态。

| 项目 | Phase 13 固定封版标识 |
| --- | --- |
| 分支 / upstream | main / origin/main |
| origin | https://github.com/yyyyyyqqqqq/Graduation-Project.git |
| completion commit | `aa2cc6fe3731ab528140b246aaf041921885ff39`；message：`feat: complete phase 13 current finding report export` |
| parent | `50afa083a953f0de97a9e498a7ff82b4b70a2362`（Phase 12 completion） |
| annotated tag | `phase-13-complete`；type=`tag`；message：`Phase 13 complete: current finding analysis report export` |
| tag object | `0b66330af5c4c12aec65c45140f97a4c3388cb3d`；本地与远端一致 |
| peeled commit | `phase-13-complete^{commit}` = `aa2cc6fe3731ab528140b246aaf041921885ff39`；本地与远端一致 |
| 封版时 main / 工作区 | HEAD = main = origin/main = remote main = completion；ahead/behind=0/0；staged=0、tracked clean、formal untracked=0 |
| 历史 tag | Phase 00—12 的 object / peeled target 未变；封版只新增 Phase 13 tag |

64.md 采用 single completion commit：`CMakeLists.txt`、`src/ProjectPage.cpp` / `.h`、`src/VulnerabilityPage.cpp` / `.h`、`src/FindingReport.cpp` / `.h`、`tests/Phase13Test.cpp` 及本文，共九个正式文件。八个实现文件与 Source-level Review SHA256 精确匹配；封版时没有修改已验收源码或测试。

**封版与独立文档维护分开。** 64.md 封版过程遵守了 commit 后不为回填 SHA 再改本文或创建第二个 documentation commit 的约定。封版完成后，用户另行明确要求审查、更新全文并提交推送，因此本次将完成态就地写回本文，形成独立 docs commit；它不是第二个 Phase 13 completion，不 amend、不移动 tag。后续接手以 `git log`、`git status` 和 `git ls-remote` 读取实时 main，不要求本文自引用其尚未产生的提交 SHA。

Phase 12 已完成 FINAL SEAL PASS：completion `50afa083a953f0de97a9e498a7ff82b4b70a2362`；annotated `phase-12-complete` object `fa4f2d9484181ffda70ddb75f29930bf898719d5`，本地 / 远端 peeled target 均为该 completion。Phase 00—13 tags 均不得移动。

封版直接证据保留于 ignored `build-debug/phase13-final-seal/`，包括 `Phase13-Final-Seal-Report.md`、`completion-commit.json`、`tag-audit.json`、`git-final-state.json` 及第 17 节验证证据。Phase 13 development、source-review、test-execution-61、test18-execution-63 及各历史阶段证据继续保留。截图、日志、HTML 报告、manual runtime、数据库、cache、审查包及 verification JSON 均不纳入 Git，不因文档维护删除；克隆仓库后不保证这些 ignored 本地证据存在。

## 5.1 Phase 11 历史封版与固定标识

Phase 11 于 2026-10-03 完成最终回归、维护性 / 隐私审查、completion commit、main push 和 annotated tag push；最终核验时间为 `2026-10-03T06:51:25.420189Z`。随后独立文档维护开始时曾确认 HEAD=main=origin/main=remote main 为下列 completion commit、ahead/behind=0/0、工作区干净；该维护提交为历史 Phase 12 起点 d8a588a，不能把下表的历史 main 等式视为当前 HEAD。

| 项目 | 已核验的 Phase 11 固定封版标识 |
| --- | --- |
| 分支 / upstream | main / origin/main |
| origin | https://github.com/yyyyyyqqqqq/Graduation-Project.git |
| completion commit | `d41a82a7060a8b29b8365a92dc7bd21ca9528b94`；message：`feat: complete phase 11 validation and threshold sensitivity` |
| annotated tag | `phase-11-complete`；type=`tag`；message：`Phase 11 complete: validation and threshold sensitivity` |
| tag object | `27f85d8f11d7fbc5caaf6fc668c3f4c2d18e1217`；本地与远端一致 |
| peeled commit | `phase-11-complete^{commit}` = `d41a82a7060a8b29b8365a92dc7bd21ca9528b94`；本地与远端一致 |
| 封版时 main / 工作区 | HEAD = main = origin/main = remote main = completion commit；ahead/behind=0/0；staged=0、tracked clean、formal untracked=0 |
| 历史 tag | Phase 00—10 的 object / peeled target 未变；封版只新增 Phase 11 tag |

封版前基线为 `b2af74d813828b7cc120b00e953c69923b21c831`；与 50.md 测试后快照核对的 93 个正式文件逐字节一致。封版期间只更新本文，已验收实现连同本文共 28 个文件形成唯一 completion commit：`.gitignore`、`CMakeLists.txt`、本文、MainWindow/RiskPriority/main 的改动、Validation 源码与测试、三个准备 / 重放工具和第 16.1 节六个公开冻结输入。所有 gates PASS，main 与 tag 分别普通推送并完成远端核验。

**区分不可变封版与可前进的 main。** Phase 11 后的独立 docs commit 只更新本文，不是第二个 Phase 11 completion commit，没有 amend 或移动任何 tag。当前 main 已领先 `phase-11-complete^{commit}`；52.md 当时禁止 Git 写操作，58.md 随后授权并完成 Phase 12 Final Seal。接手时以 `git log`、`git status` 和 `git ls-remote` 读取实时状态，不将封版时的 main 等式套用到后续提交。

封版直接证据保留于 ignored `build-debug/phase11-final-seal-51/`：`final-seal-verification.json`、`Phase11-Final-Seal-Report.md` 记录 Git / tag / 远端完成态，`final-verification.json` 及相关日志记录第 17 节回归。构建目录内归档、采集记录、截图、结果 artifact、review patch、启动脚本和隔离 runtime 均不提交也不删除。结果 artifact 的 base commit / dirty 是构建期注入信息；第 17 节正式源码指纹关联实际受测实现，不能把 base commit 当作 completion commit。

## 5.2 Phase 10 历史封版记录

Phase 10 最终封版起点为 main / origin/main，HEAD `f88ef484e03c218743446180394622b1f9074852`，ahead/behind=0/0、staged=0；六个已跟踪正式文件修改、三个新增正式文件与 46.md 预期一致。当时仅就地更新本文，将已验收的九个实现文件连同本文纳入一个独立 completion commit，未 amend / rebase / force push。

| 项目 | Phase 10 封版标识与核验方式 |
| --- | --- |
| 分支 / upstream | main / origin/main |
| origin | https://github.com/yyyyyyqqqqq/Graduation-Project.git |
| completion commit | `0f1cca832c382586ec22d8bba8267c0f8d36e582` = `phase-10-complete^{commit}`；message：`feat: complete phase 10 exploit signal prioritization` |
| annotated tag | `phase-10-complete`；message：`Phase 10 complete: exploit signal prioritization` |
| tag object | `3c4307893a938bc50464d666ff9c53fb10e8e4ab`；type=`tag` |
| peeled target | `git rev-parse 'phase-10-complete^{commit}'`；等于 Phase 10 completion commit |
| 封版时 main 同步态 | 当时 HEAD = main = origin/main = remote main = Phase 10 completion；ahead/behind=0/0 |
| tag 同步完成态 | 本地 / 远端 tag object 与 peeled target 分别一致；Phase 00—09 object / target 不变 |
| 工作区完成态 | main；staged=0、tracked clean、正式源码 untracked=0；ignored 本地产物保留 |

Phase 10 已按审查并提交十个正式文件 → 普通 push main → 创建并单独 push annotated tag → 核验远端与 clean 状态的顺序完成封版。上表是历史封版事实；后续文档与 Phase 11 提交已推进 main，Phase 10 tag 保持不变。

正式文件边界：CMakeLists.txt、PROJECT-HANDOFF.md、src/MainWindow.cpp、src/RiskEvidence.cpp、src/RiskEvidenceController.cpp、src/RiskEvidenceController.h、src/RiskPriority.cpp、src/RiskPriority.h、src/VulnerabilityPage.cpp、tests/Phase10Test.cpp。构建、日志、数据库、缓存、review patch、截图、fixtures、生成的 cmd / checklist / 测试 JSON 均不提交，也不因封版删除。

既有提交使用仓库级身份 `yyyyyyqqqqq` / `104704290+yyyyyyqqqqq@users.noreply.github.com`。仓库级 credential.https://github.com.username 为 yyyyyyqqqqq，用于 HTTPS 认证账户选择；认证账户与 commit 作者配置不同。本轮不修改凭据或全局 Git 配置。

## 5.3 Phase 00—13 不可变 tag 基线

以下十四个 annotated tags 已核对本地 / 远端 object 与 peeled target 一致，后续提交不得移动；Phase 09 completion message 为 `feat: complete phase 09 risk evidence enrichment`，tag message 为 `Phase 09 complete: risk evidence enrichment`：

| Tag | Tag object | 固定 target |
| --- | --- | --- |
| phase-00-complete | 019bba4a551d5ff53c23a2859490fc1305bb8f5c | 765bfaeb69bab26de80ff0bf3b24358688d89bca |
| phase-01-complete | 4929a1185cf185e63b789833b63b69b1ac0a1f2f | 64292ab39bc9ffbf13c36058e47b055c812a6344 |
| phase-02-complete | a50625fd87308abf068cdfc91cdb25da96f003b0 | fc9ef4ea6b68a9924fed9430cb64cbcd7bbeb0b8 |
| phase-03-complete | 3238799e4f5d60122d547bbc9a9adbbf8d227951 | e5ef271c5249c9df6afe91abf37f163e4c9e73df |
| phase-04-complete | b7469f182aa8202194a7a1ffdd5fd5d7ea4b5c4f | febb52ef16100eaa61d84e6fc7e6e643b2a5c5f8 |
| phase-05-complete | 107e1dc2b535ee0a8af30f25f2d5c00744e2d922 | a7ff0bb0338841b46975e832373d6eb26e5060b7 |
| phase-06-complete | c9cccc416ea042efbe046a3eba16af9f6da72d03 | a9a1c180b7093c4553c0862d55536cf8e6ae7d0d |
| phase-07-complete | b4a5edd09c2836ccd71786e8d3178fed0307eb0b | ad8c75274a2a8c4db275ab099f949a7c47fcecdb |
| phase-08-complete | dc5d5d5926267c452f3b92e36f33a05efc17599b | ce3bd53f99d3f91e6ea411e836a2a301101d1321 |
| phase-09-complete | e15a8000e410a7cb75299bc7fbb3bf4cb06b2883 | 1452b1982fbdafe35580be36a508b54ea417fe5d |
| phase-10-complete | 3c4307893a938bc50464d666ff9c53fb10e8e4ab | 0f1cca832c382586ec22d8bba8267c0f8d36e582 |
| phase-11-complete | 27f85d8f11d7fbc5caaf6fc668c3f4c2d18e1217 | d41a82a7060a8b29b8365a92dc7bd21ca9528b94 |
| phase-12-complete | fa4f2d9484181ffda70ddb75f29930bf898719d5 | 50afa083a953f0de97a9e498a7ff82b4b70a2362 |
| phase-13-complete | 0b66330af5c4c12aec65c45140f97a4c3388cb3d | aa2cc6fe3731ab528140b246aaf041921885ff39 |

核验使用 git status、git branch -vv、git log、git rev-list --left-right --count main...origin/main、git ls-remote origin，以及上述 tag object / peeled 查询。main 同步不能代替 tag 核验。远程是重新创建后的同名仓库，不继承旧仓库历史；今后操作前仍需读取真实状态。

# 6. 正式环境基线

下表记录**正式工具链基线及已核验的 IDE 更新**。Phase 00 已在正式工程复验 CMake 3.30.5、Ninja 1.12.1、MinGW GCC 13.1.0 x86_64、C++20、Qt 6.11.2 运行时及 Graphviz 16.1.0；2026-09-30 的核验范围见下文，其余环境项目仍以历史审计为依据。

| 技术 | 版本 / 职责 |
| --- | --- |
| Windows | Windows 11 x64，Build 26200 |
| C++ | C++20，正式核心实现语言 |
| Qt | Qt 6.11.2 MinGW 64-bit；Qt Widgets 为主界面 |
| Qt Creator | 20.0.2（2026-09-30 从 20.0.0 更新）；Kit：Desktop Qt 6.11.2 MinGW 64-bit |
| 编译器 | MinGW-w64 13.1.0 x86_64 |
| CMake / Ninja | 3.30.5 / 1.12.1，配置与构建 |
| SQLite / Qt Sql / QSQLITE | 本地持久化 |
| Qt Network / Qt JSON | Network 支撑 OSV 查询与 FIRST EPSS / CISA KEV evidence fetch；JSON 使用 Qt Core 的 QJson 类型 |
| Qt Concurrent | 后台导入 / 诊断、Apply、依赖查询、OSV 解析 / 缓存与整批 Finding 派生；Phase 09 扩展 EPSS / KEV 解析、证据组装及原子缓存写入，Phase 11 用于冻结实验，Phase 13 用于报告渲染及原子写入 |
| Qt Test / CTest | 自动测试及测试执行 |
| Graphviz / Qt Svg | Graphviz 16.1.0 当前仅用于 Phase 00 dot → SVG 冒烟；正式应用未接入 Graphviz 依赖图绘制或 Qt Svg 展示 |
| windeployqt | 历史环境已验证；当前正式应用尚未完成最终部署包验收 |
| Git / GitHub CLI | 历史版本 2.54.0.windows.1 / 2.96.0；本次使用 Git 核对状态，未重新验证 CLI 版本或 gh 可用性 |

2026-09-30 环境对齐：用户提供的 Qt Maintenance Tool 更新记录显示 Qt Creator 20.0.2、SDKTool、Telemetry plugin，以及随 Creator 分发的 jom / wininterrupt 已更新完成；安装组件清单与 qtcreator.exe 文件版本确认 Creator 为 20.0.2。这是 IDE 及其辅助组件更新。实查 qmake 为 Qt 6.11.2、g++ 为 13.1.0 / x86_64-w64-mingw32、CMake 为 3.30.5、Ninja 为 1.12.1；Creator 的 6.11.2 MinGW Kit 仍在，Debug / Release CMakeCache 仍指向原 Qt / MinGW / Ninja 路径。正式工程使用 Ninja，不因 Creator 内置 jom 更新而切换生成器；该 IDE 更新无需调整 CMakeLists.txt、源码、当时的 App 0.11.0 或 schema 4。应用随后由 Phase 11 升至 0.12.0，Phase 12 升至 0.13.0，Phase 13 升至 0.14.0；工具链未变。未重新进行 Creator GUI 操作验收或完整环境审计。

关键工具路径：

```text
Qt       D:\program\Qt\6.11.2\mingw_64
MinGW    D:\program\Qt\Tools\mingw1310_64\bin
CMake    D:\program\Qt\Tools\CMake_64\bin\cmake.exe
Ninja    D:\program\Qt\Tools\Ninja\ninja.exe
Graphviz C:\Program Files\Graphviz\bin\dot.exe
```

历史最终结论：**FULLY READY**。审计曾真实验证编译链、C++20 / Qt、SQLite、HTTPS / JSON / NVD / EPSS / KEV、Qt Concurrent、Graphviz / SVG、Qt Test / CTest、Debug / Release、windeployqt、独立运行及端到端集成。

历史结论证明当时工具链集成可用。正式工程的 Phase 00 原位最小复验已通过，后续 Phase 01—13 延续同一冻结工具链；CMakeCache、编译命令及运行结果均确认 Qt 6.11.2，未混入 6.11.1，没有重装或替换工具链。

# 7. 环境已知提醒

- Qt 6.11.2 是正式基线；6.11.1 仅是已验证回退环境，不随意切换、卸载、升级或替换工具链，不无故修改系统 PATH。
- **NVD API Key 历史状态为未确认 / PENDING，本轮没有重新检查。** 当前产品尚未接入 NVD，不阻塞已完成阶段；未来若授权接入再确认；密钥禁止写入源码、日志和 Git，可用用户环境变量 `NVD_API_KEY` 或受忽略的本地配置。
- Graphviz 最终部署策略待定，`dot.exe` 属于外部依赖，不由 windeployqt 自动打包。
- SQLite 的 `QSqlDatabase`、`QSqlQuery` 等对象不跨线程共享；各线程使用自己的连接，处理事务与锁；GUI 更新在主线程执行。
- 历史 `LongPathsEnabled=0`，保持浅目录；正式路径含空格，命令必须正确引用。
- 历史 optional Vulkan Headers / DX12 compiler 提示不阻塞当前 Qt Widgets 工程；正式应用尚未链接 Qt Svg，不能将历史 SVG 环境验证当作已接入产品展示。Qt 导入配置及 Debug 可用性以既有构建证据为准，不据此重装。
- 历史独立部署验证是在当前电脑排除开发路径后完成，尚不等同于全新 Windows 电脑验收。
- **截至 Phase 13 封版无已知环境 Blocker。** 64.md 完成 Debug / Release Build 与 Phase 00—13 全量回归，沿用冻结工具链；这不是完整环境重新审计。本轮未改变系统 PATH、代理、DNS、证书、防火墙或网络连接状态。

# 8. 系统核心目标

最终产品应回答：项目使用什么组件、哪些组件关联漏洞、哪些漏洞适用于当前版本、哪些应优先处理，以及为什么得到这样的优先级和证据支持结论。Foundation、Vulnerability Finding 与 RiskEvidenceProfile MVP 已完成；Phase 10 已闭合 Exploit-Signal Priority → Decision Evidence Support → Explanation 子模型，Support 不是预测置信度，综合风险模型仍未实现。解析成功或质量规则未发现问题不等于 SBOM 完整、组件真实存在或没有风险。

结果需要可解释、可追溯；Phase 11 已提供冻结实验与方法验证页面，Phase 13 已提供单 Finding 业务报告，后续 Dashboard 和项目级报告的范围须另行设计，以支持论文及演示；扫描记录、历史比较的必要范围在对应阶段再确定。不能让完整业务完全依赖实时公网 API，应按实际 Phase 设计缓存、可复现验证及本地演示方式，不因此提前建设完整离线镜像。

论文与答辩以 **Validation Experiment** 为定位，不作 General Benchmark。Phase 11 已冻结 40 个真实 component-version 样本；结论限于选定测试项目、package ecosystem 与验证数据集上实际观察到的系统功能和方法行为，不宣称所有生态的普遍识别准确率。

Trivy、Grype、Dependency-Track、OSV-Scanner、cve-bin-tool 等成熟工具在覆盖面、成熟度和生态上明显高于本科项目；本系统不以替代它们为目标。项目价值在于完整桌面系统、可运行的 SBOM → Vulnerability → Risk 分析链，以及 Component Identity、Version Applicability、Priority / Decision Evidence Support 分离、Explainability 与验证实验。

主要工作 / 特色设计包括基于 SBOM 的候选匹配、版本适用性判断、利用信号优先级及可追溯解释流程；Phase 10 已将 Priority 与 Decision Evidence Support 分离。质量报告与持久化 Current State 尚未关联，不把 Quality Unavailable 伪装成风险或置信度分数。这些是工程与方法设计。Phase 11 已验证冻结 Rules v1 的行为与阈值敏感性，未证明预测准确率或最优阈值，不作“首次提出”或“全面优于”声明。

# 9. 核心业务流程

**当前已实现：** Project → SBOM Parse → Quality Diagnosis → Explicit Apply → Current Components + Raw Dependencies → Dependency Analysis。

**正式漏洞与优先级链：Current Component → Package Identity → OSV Candidate → Local Applicability → Provider Consistency Gate → Vulnerability Finding → RiskEvidenceProfile → Exploit-Signal Priority Assessment → Decision Evidence Support → Deterministic Explanation。** Profile 包含 scoped Severity、FIRST EPSS、CISA KEV、Dependency Context 与 Quality availability；未实现 numeric Risk Score / Risk Level / predictive Assessment Confidence。

**唯一有效长期路线：Foundation → Vulnerability → Risk → Validation / Presentation。**

| 层 | 正式技术主线 | 当前状态与职责 |
| --- | --- | --- |
| Layer 1 — Foundation | SBOM → Component → Quality → Dependency | 基本完成；Phase 00—06 已建立标准化供应链输入、CycloneDX 解析、组件模型、质量诊断、SQLite / Current Components / Raw Dependencies 持久化及依赖分析，为漏洞分析提供可追溯输入与项目上下文 |
| Layer 2 — Vulnerability | Component Identity → Candidate Matching → Version Applicability → Finding | MVP 已完成：Identity、OSV Candidate、本地 Applicability、Provider Consistency Gate 与内存 Finding；支持范围及 Unknown 边界见第 16 节 |
| Layer 3 — Risk | RiskEvidenceProfile → Exploit-Signal Priority → Decision Evidence Support → Explanation | Phase 10 MVP COMPLETE；Rules v1 与 0.90 research percentile threshold 已冻结；不是完整综合风险模型 |
| Layer 4 — Validation / Presentation | Validation / Calibration → Presentation → Current Finding Report | Phase 11、12 已封版；Phase 13 COMPLETE / FINAL SEAL PASS，GUI / Report 19/19 PASS |

缺失数据保留诊断或不确定状态，不能解释成无漏洞、无风险。第 13 节只将这条主线映射为已完成阶段、下一阶段定位和后续方向，不另设并行正式路线。

# 10. 核心模块

模块职责按第 9 节四层路线组织；这是业务职责划分，不是提前创建未来类、目录或框架的指令：

| 模块 | 主要职责 |
| --- | --- |
| Foundation | 项目创建 / 列表 / 详情 / 删除、CycloneDX JSON 1.4 / 1.5 / 1.6 共同字段子集解析与只读预览、issue-based 质量诊断、当前组件及原始依赖事务持久化、精确引用解析、直接 / 反向及按需传递查询均已实现；无总体质量分或自动修复 |
| Vulnerability | 已实现 PyPI/npm 最小身份与所选组件 OSV 候选查询、原始证据、文件缓存和隐私确认；本地版本适用性、Provider Consistency Gate、内存 Finding 与解释 UI 已实现 |
| Risk | 已实现内存 RiskEvidenceProfile 与纯 Core RiskPriorityAssessment；Controller 本地到期重评估，UI 展示 Priority / Support / provenance / 确定性解释；无 numeric Risk Score / Risk Level / predictive confidence |
| Validation / Presentation | Phase 11 冻结实验及阈值敏感性；Phase 12 Current Project 概览与共享结果解释。Phase 13 单 Finding 静态 HTML Report 已实现；Dashboard / project-wide Report / Graphviz presentation 尚未实现，须另行授权需求与设计 |

Project / projects 与 Component / components 已实现正式持久化；SbomDocument、SbomComponent、SbomDependency、SbomQualityIssue、SbomQualityReport 仍为导入或诊断内存模型。DependencySnapshot 表达一致读取结果，DependencyGraph 为可失效的内存派生图；原始依赖已持久化。PackageIdentity / QueryIdentity、VulnerabilityCandidate / OsvSnapshot、ApplicabilityResult / CandidateAssessment / ApplicabilitySnapshot / VulnerabilityFinding、RiskEvidenceProfile 和 RiskPriorityAssessment 为内存值模型。完整成功 OSV / EPSS / KEV 查询另有外部证据文件缓存；没有正式 identity / vulnerability / finding / risk / profile 表，缓存不是第二业务数据库。未持久化 Finding / Profile / Assessment / Quality；当前仍无 numeric Risk Score / Risk Level / predictive confidence 或批量扫描。Phase 13 已支持 selected confirmed Finding + RiskEvidenceProfile + RiskPriorityAssessment 的业务 HTML 导出；不持久化 Report。ValidationReplay 实验 JSON 与业务报告是不同产物。

# 11. 漏洞数据源定位

依据用户在 `22.md` 确认的路线决策，**OSV 是 Phase 07 MVP 的首选 vulnerability matching source；OSV-first 是 MVP 和第一版端到端闭环的当前主数据源策略，不是系统唯一最终漏洞数据源。**

- **OSV**：围绕 package / version identity 获得真实漏洞候选。选择理由是与 PURL / ecosystem 路线自然衔接、实现成本相对可控、较快跑通候选链，并避免 MVP 起步即陷入完整 PURL → CPE 映射。
- **NVD**：未来可用于漏洞基础信息与 CVSS 等 enrichment，不作为 Phase 07 MVP 主匹配链。
- **EPSS**：Phase 09 已接入 FIRST exploit likelihood evidence，不能证明版本适用性，也不是综合风险评分。
- **CISA KEV**：Phase 09 已接入公共完整目录；NotListed 只针对所用完整目录，不等于没有风险。

OSV 已正式接入：固定 HTTPS https://api.osv.dev/v1/query，仅查询所选 PyPI/npm 组件，支持完整分页、错误分类、取消和 AppData 文件缓存。Phase 09 新增 FIRST EPSS https://api.first.org/data/v1/epss（仅发送经确认的 CVE ID）与 CISA KEV 官方 kev-data 公共完整目录 `https://raw.githubusercontent.com/cisagov/kev-data/develop/known_exploited_vulnerabilities.json`（不发送组件信息）。NVD 尚未接入。Phase 07 历史真实 Qt Network smoke 于 2026-09-18T09:57:04Z 查询公开 PyPI six 1.17.0 成功（当时 0 candidates，约 2.3 秒）；该数量不是永久保证。历史回归及最新 64.md 封版 CTest 均使用 synthetic / fake transport 及公开冻结输入离线回归，不重跑实时服务 smoke；实验 Replay 也不查询 live providers。

# 12. 风险评估原则

**Phase 10 Rules v1 已冻结并实现；它是 Exploit-Signal Prioritization MVP，不是综合 Risk Score / Risk Level 或预测置信度模型。Risk ≠ CVSS。** Finding 是唯一输入前提，不从 Candidate 直接评估。

Phase 08 在原始 affected 证据之上解释 exact versions 与严格 SEMVER。**Identity Resolved ≠ Version Applicable；Candidate ≠ Finding。** 仅 Published Affected 可形成 Finding；Local NotAffected 在 OSV package-version Candidate 上经 Provider Consistency Gate 转为 Published Unknown / ProviderEvidenceConflict。OSV package/version 查询可能使用 fuzzy upstream-version matching，不能宣称 strict equality。Unsupported evidence 保留 Unknown，withdrawn 为 Excluded；不以本地未命中反向推翻 provider，也不把 Unknown 解释成安全。

**Priority** 表达当前可用利用信号支持的处理优先类别；**Decision Evidence Support**（Complete / Partial / Insufficient）表达该规则判断所需证据的当前有效性、完整性与可用性，不是 predictive confidence、Finding correctness、漏洞概率或整体风险确定性。两者及解释由同一纯 Core 评估结果生成。

Fresh KEV Listed 优先；否则按所有 Fresh Available EPSS 的最大 percentile 与 research default 0.90 比较；没有 Fresh driver 则 InsufficientCurrentExploitEvidence。0.90 表示相对排名阈值，不是 90% 被利用概率，也不是 FIRST 官方 High threshold。Below 明确不等于 Safe / Low Risk / Not Exploitable。

Stale evidence 仅作历史上下文；future timestamp 是 Ineligible，不混为 Stale。规则、provenance、24h 边界与本地 timer 详见第 16 节。Severity / Dependency 只作 context；Quality UnavailableForPersistedCurrentState 不改变 Priority，也不降低 Support。质量诊断不能简单转为 Risk 加分项，Dependency 不能证明 runtime reachability。无 numeric score、CVSS 计算或 normalization；Phase 11 实验未扩展生产模型。

# 13. Phase 路线

以下是第 9 节唯一长期路线的阶段状态。Phase 00—13 均已封版；Phase 13 已完成 Frozen Design、实现、源码审查、19/19 GUI / Report 验收及 64.md Final Seal；后续需求与设计须另行授权。

| Phase | 名称与主要目标 | 当前状态 |
| --- | --- | --- |
| 00 | 工程初始化：最小 Qt/C++20 构建、运行与测试基线 | COMPLETE |
| 01 | 应用基础框架：导航、页面、日志、配置、基础数据库访问 | COMPLETE |
| 02 | 项目管理 | COMPLETE |
| 03 | CycloneDX SBOM 导入 | COMPLETE |
| 04 | SBOM 质量诊断 | COMPLETE |
| 05 | 组件管理与持久化 | COMPLETE |
| 06 | 依赖关系分析 | COMPLETE |
| 07 | Component Identity + OSV-first Vulnerability Matching MVP | COMPLETE |
| 08 | Version Applicability & Finding MVP | COMPLETE |
| 09 | Risk Evidence Enrichment MVP | COMPLETE |
| 10 | Exploit-Signal Prioritization MVP | COMPLETE |
| 11 | Validation & Threshold Sensitivity Experiment MVP | COMPLETE / FINAL SEAL PASS；completion 定位见第 5.1 节 |
| 12 | Presentation & Explainability MVP | COMPLETE / FINAL SEAL PASS |
| 13 | Current Finding Analysis Report Export MVP | COMPLETE / FINAL SEAL PASS；固定 commit / tag 见第 5 节 |
| 后续 | Final Integrated Acceptance / Thesis Delivery Hardening | 等待新授权；不自动启动 Phase 14 |

**Historical / Superseded Plan：** Phase 00—06 开发期间使用的旧 Phase 07—17 未来安排已被 `22.md` 路线决策取代，旧表从正式路线中移除，历史可由 Git 查询；不得据其启动开发。Graphviz 仅为后续辅助展示与解释能力，不是 Phase 07 任务。

## 13.1 Phase 07 已完成定位与范围

**Phase 07 — Component Identity + OSV-first Vulnerability Matching MVP，COMPLETE。** 已实现 **SQLite Current Component → Package Identity → OSV → Real Vulnerability Candidate**；正式来源是 Explicit Apply 后的 Current Components，不是 Preview。

当前 PURL-first 严格子集支持 PyPI、npm 与 scoped npm，无 Component.name / type / bom-ref fallback，不改写原始 Component。Quality report 仍为未持久化的导入期内存模型，Current Component 不自动携带完整质量报告；后续证据关联仍需设计。

已输出 ecosystem、查询包名、精确版本、版本来源、Identity 状态 / reason、OSV ID、CVE aliases 与原始证据。Resolved / Insufficient / Ambiguous 和 NotStarted / Working / Success / Failed / Cancelled 分离；只有完整成功查询的候选数才有确定含义。失败、取消或身份不足显示未知，完整零结果只说明当前查询未返回候选。

具体身份规则、分页 / 限额 / 缓存、异步生命周期及 UI 见第 16 节；Phase 07 封版时应用版本 0.8.0，schema 保持 4，没有新增业务表。

## 13.2 Phase 08 已完成定位与边界

**Phase 08 — Version Applicability & Finding MVP，COMPLETE。** 在既有 QueryIdentity / OSV Candidate 后增加纯 Core evaluator、strict SemVer、Provider Consistency Gate 和内存 Finding，扩展既有 Controller / Page，不建立第二套查询或 Provider Framework。

已实现 exact versions、SEMVER、同 ecosystem wildcard package、withdrawn exclusion，以及生命周期保护和解释 UI。ECOSYSTEM / PEP 440、GIT commit graph、完整生态与 vendor/backport 规则不在当前解释范围，保留 Unknown。

Phase 08 封版时未包含 Finding persistence、Risk / Confidence 模型、NVD / EPSS / KEV enrichment、批量扫描、正式 Graphviz 业务、Dashboard 或报告。Phase 09 随后完成 evidence enrichment，当前边界见下节。

## 13.3 Phase 09 已完成定位与边界

**Phase 09 — Risk Evidence Enrichment MVP，COMPLETE。** 仅从当前 confirmed Finding 构造 RiskEvidenceProfile，包含 scoped OSV Severity、FIRST EPSS、CISA KEV、Dependency Context 和 Quality availability；不重复拥有 Finding，不生成 Risk Score / Risk Level / Assessment Confidence，不接入 NVD，不新增持久化业务表。Phase 10 随后在此 Profile 上完成利用信号优先级子模型，当前边界见下节。

## 13.4 Phase 10 已完成定位与边界

**Phase 10 — Exploit-Signal Prioritization MVP，COMPLETE。** 43.md—46.md 的 Frozen Design / Implementation / Automated Acceptance / ChatGPT Code Review / GUI Acceptance 均 PASS；Blocker 0、Must Fix 0、Frozen Design Deviation 0、Scope Creep 0。完整能力、验证与 GUI 证据分别见第 16—18 节。

Phase 10 封版时 App 0.11.0、schema 4；该阶段未新增表、migration、provider 或 persistence。C-WORDING-01 已由 46.md 判定为 FALSE FAILURE / TEST INTERPRETATION TOO LITERAL：Below 的否定式安全提醒正确，C Below / Complete 为 PASS，无需修改代码。它不属于 Known Issue 或待修缺陷。

## 13.5 Auxiliary Development Infrastructure

| Task | Name | Type | Status |
| --- | --- | --- | --- |
| Phase 07.5-AUX | Read-Only Project Context MCP | Auxiliary Development Infrastructure | COMPLETE / CHATGPT CONNECTION PASS |

Phase 07.5-AUX 已完成本地实验验收并采用为辅助开发基础设施。它位于正式 Git 仓库之外的同级独立目录 `project-readonly-mcp`，不属于 Product Feature 或论文业务 Phase，不插入上方正式 Phase 表或 Foundation → Vulnerability → Risk → Validation / Presentation 业务路线；不改变正式 CURRENT PHASE、产品架构、数据库、版本或产品能力。

实现使用 Python + 官方 MCP SDK，经 localhost Streamable HTTP 提供服务，绑定 `127.0.0.1`、endpoint `/mcp`。v1 恰好 7 个 tool：项目高层状态、本地 Git 状态、Handoff 单章节 / outline、历史 build / CTest evidence、匿名 runtime SQLite summary、runtime storage metadata；extra tools / resources / resource templates / prompts 均为 0。

Git 仅受限只读查询，SQLite 使用真正 read-only connection，仅支持的 schema 4 返回匿名 aggregate；runtime storage 仅元数据。无任意 filesystem / shell / Git args / SQL，不返回 Project / Component 业务记录、包身份 / PURL / SBOM、应用日志、settings 或原始 OSV cache 正文。历史 CTest artifact 不证明当前 HEAD 已重新验证，数据库 aggregate 不代表业务内容。

本地验收结论：正式工程与数据库只读 integrity proof PASS；自动测试 76 项（75 PASS / 0 FAIL / 1 SKIP），skip 为当前 Windows 权限无法实际创建 file symlink，真实 junction 与 reparse 防护已验证；MCP Inspector 52 项 PASS。实验未修改正式产品工程、数据库或运行数据。详细技术资料和验收记录留在独立工具的 README.md / REPORT.md，不复制 local 证据，也不作为第二套长期项目状态源。

**CHATGPT MCP CONNECTION TEST: PASS**

用户在 `30.md` 确认真实链路 ChatGPT → OpenAI Secure MCP Tunnel → tunnel-client → localhost Read-Only MCP 已建立；ChatGPT MCP App 已连接，恰好 7 个工具发现 PASS，get_project_status / get_git_status / get_handoff_outline 实际调用 PASS，23 个一级章节 outline 读取成功，停止并重启本地服务后的 reconnect / invocation 也 PASS。这是实际用户 / ChatGPT 验收，不是 Codex 模拟或仅由 Inspector 推断。此前 READY FOR CHATGPT CONNECTION TEST 已完成验收；当前 **PHASE 07.5-AUX: COMPLETE**、**READ-ONLY MCP: OPERATIONAL**。

get_project_status 的 auxiliary_task_status 从本文明确的连接验收标记派生，表示已记录的验收结论，不是实时 Tunnel 健康探测；本次文档维护没有启动或检查 MCP / Tunnel 运行状态。MCP 与 tunnel-client 仍需用户手动启动，用毕 Ctrl+C 停止；两进程关闭后 ChatGPT 无法继续访问。仅 localhost MCP 与出站 Secure MCP Tunnel，无公网 MCP 入站 endpoint、常驻服务或自动启动；不改变产品工程，不构成新业务阶段的开发授权。

# 14. 已完成 Phase

本节按各阶段完成时的状态记录历史事实。旧版本号、schema、测试数量和当时未实现的功能不代表当前状态；当前基线分别见第 5、15—18 节。

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

**Phase 05 —— COMPLETE。**

- 正式 Component 与输入 SbomComponent 分离，保存原始五字段、项目 ID、UUID row identity、metadata root 来源及 Parser 展平顺序；每次 replacement 生成新 UUID，不建立跨导入语义身份。
- schema 3 新增 components 和项目来源位置唯一索引；fresh / v1 → v2 → v3 / v2 → v3 事务迁移、冲突保护、失败回滚、Project 数据保留及实际 FK enforcement 通过验证。
- ComponentRepository 原子执行验证项目、删除旧集、写入完整新集和提交；失败保留旧行及 ID。Project 删除由 ON DELETE CASCADE 统一清理组件。
- 成功 Preview 不写库；显式 Apply 后完整替换当前组件，Quality Error 仍可 Apply；空输入可清空，重复标识和缺失值原样保存，无 merge / auto-fix / 去重。
- 后台 Apply 在所属线程创建并销毁独立连接，使用同一正式数据库路径；Working 状态和重复 Apply 保护、失败重试、窗口销毁安全均有测试。项目当前组件页从 SQLite 读取，项目隔离及重启保持。
- Debug / Release Build PASS，CTest 均 64/64 PASS；旧 Phase 00—04 的 48 项回归保留，Phase 05 新增 16 项。
- 用户在 `19.md` 确认 Debug GUI 12 项人工验收全部 PASS，包括 100000 组件 Apply 约 1 秒且无明显不可接受卡死；Release 为 AUTOMATED ONLY，性能记录仅为当前机器证据。
- 最终维护性、线程、数据库和隐私审查 PASS；独立 completion commit、main 推送及 `phase-05-complete` annotated tag 封版。

**Phase 06 —— COMPLETE。**

- schema 4 新增 dependency_capture、dependency_entries、dependency_targets 及顺序唯一索引；fresh / v1 → v2 → v3 → v4 / v2 → v3 → v4 / v3 → v4 安全事务迁移、失败回滚、实际 FK enforcement 通过验证；老项目保持 Not Captured。
- ComponentRepository 承担统一 Current State 边界，一次 Explicit Apply 原子替换组件、原始 entries / targets 与捕获标记；失败完整保留旧数据及 Component UUID。单一读事务提供一致 snapshot。
- 保留原始顺序、空白、重复声明和空 dependsOn；Not Captured 与 Captured Empty 分开。引用仅按原始 bom-ref exact match，包含 metadata root；Missing / Unknown / Resolved / Ambiguous 明确，重复 bom-ref 不猜测绑定。
- DependencyGraph 构建确定性唯一正向 / 反向 adjacency，顺序由 first resolved occurrence 决定；Raw self occurrences 独立统计，可靠 self-loop 保留于 direct graph。所选组件按需 BFS，包含直接关系、返回最短 depth，排除起点，cycle-safe；无 all-pairs closure 或永久 closure cache。
- 项目依赖页提供摘要、原始 entry / target 明细及四种关系查询。SQLite 为业务权威来源，Runtime Graph 只在内存中派生；Project switch / Apply success / Reload 失效，Project ID + request generation 防止 stale result 覆盖新项目，连续请求合并。
- 后台 worker 在自身线程创建 / 使用 / 销毁数据库连接，值捕获相同正式 DB path；只回 GUI 线程安装结果。图以 shared immutable value 支撑后台查询，窗口销毁与过期交付安全。
- Phase 06 封版时 Debug / Release Build PASS，CTest 均 82/82 PASS；Phase 00—05 regression 64/64，Phase 06 新增 18/18。100000 components / entries / targets、99999 条传递结果及 GUI responsiveness 验证通过，具体数据见第 16 / 17 节。
- 用户在 `21.md` 确认全部 26 项 Debug GUI 人工验收 PASS，包含大规模响应与 stale-result 无串数据；Release GUI 为 AUTOMATED ONLY。该结论来自用户当时操作，性能仅为当次机器与输入条件下的证据。
- 最终维护性、数据库 / 线程 / 图生命周期及 Sensitive / Privacy Review PASS；独立 completion commit、main 推送及 `phase-06-complete` annotated tag 封版。后续路线见第 9 / 13 节，后续阶段的历史完成记录见下。

**Phase 07 —— COMPLETE。**

- 正式链路为 SQLite Current Components → Package Identity → OSV /v1/query → Vulnerability Candidate，Preview 不作为分析来源；支持 PyPI/npm（含 scoped npm）的严格 PURL 子集，无 name / type / bom-ref fallback。
- Identity Resolved / Insufficient / Ambiguous 与版本适用性分离，保留精确查询版本及来源；版本冲突不猜测，不因尚无完整版本 parser 而拒绝 epoch / local / v prefix。
- 所选组件单并发 HTTPS 查询，分页含 token-only page、重复 token 防护、累计字节 / 页数 / 记录限制、超时与 deadline、错误分类、取消及完整结果发布；OSV ID 为候选主身份，CVE 仅是 alias，affected 原始证据保留但不求值。
- AppData 文件缓存随 --data-dir 隔离；安全固定文件名、内部 QueryIdentity 校验、QSaveFile 原子写、完整零结果可缓存、24 小时 Fresh / Stale、Live / Cache、仅本地查询和显式清除。
- 首次实际联网发送确认、最小请求数据、无自动批量查询；Current Components / Identity、解析与缓存后台任务、generation 失效保护及窗口 / reply 生命周期均有回归覆盖。
- 应用 0.8.0，schema 4 和六表保持；未新增 identity / vulnerability / finding / risk 正式表。100000 Components 读取、身份计算、表格与 GUI responsiveness 验证通过。
- Phase 07 封版轮最终 Debug / Release Build PASS，CTest 各 108/108 PASS（旧回归 82 + Phase 07 26）；用户在 `26.md` 亲自确认 Debug GUI 18/18 PASS，Release GUI 为 AUTOMATED ONLY。
- Phase 07 维护性及 Sensitive / Privacy Review 已通过；completion commit 与 annotated `phase-07-complete` 固定标识见第 5 节，历史封版事实不随后续业务阶段改变。

**Phase 08 —— COMPLETE。**

- VersionApplicability 为纯 Core evaluator，按 QueryIdentity 过滤 affected package；exact versions 使用字符串相等，strict SemVer 2.0 使用无整数溢出的十进制字符串比较，支持 prerelease 与 build metadata precedence。
- OSV Provider query 与 Local Applicability 分离；Local NotAffected → Published Unknown / ProviderEvidenceConflict，不宣称 provider strict equality；仅 Published Affected 创建 Finding。
- 同 ecosystem 的 package.name=* 作为 wildcard；withdrawn → Excluded / RecordWithdrawn；不支持的 ECOSYSTEM / GIT / range.type 保留 Unknown。
- Code Review 发现的 limit 特殊值问题已按 33.md 修复为 contains('*')：*、1.*、future* 均为 infinity，finite + infinity 与 invalid finite limit 继续测试。
- Finding 只在内存派生，绑定 component / query / snapshot / fetchedAt，整批结果由 generation 验证后发布；原始 OSV cache、schema 4、六表及迁移保持不变。Phase 08 封版时应用版本为 CMake 唯一来源 0.9.0。
- 历史 34.md 最终 Debug / Release Build PASS，完整 CTest 各 128/128 PASS（既有 108 + Phase 08 20），耗时分别 46.83 s / 42.39 s。
- ChatGPT Code Review PASS；用户在 34.md 确认亲自完成 Debug GUI 全部人工验收 PASS，Release GUI 为 AUTOMATED ONLY。历史维护性 / 隐私审查及 diff 检查通过；由 phase-08-complete 固定定位，tag object / target 见第 5 节。

**Phase 09 —— COMPLETE。**

- Frozen Design、Implementation、ChatGPT Code Review 及两项 Must Fix 均 PASS；Candidate/Finding 请求查找为 O(C + F)，KEV 的 CVE 后缀单独限制 4–19 位，共享 OSV alias 语法仍保持 4+ 位。
- RiskEvidenceProfile 保留各 provider fact、freshness、acquisition 与错误状态，Quality 为 UnavailableForPersistedCurrentState；无风险等级、综合评分或置信度算法。
- Windows cache atomic replacement 修复只在 commit 返回 false 且 RenameError 时有限重试，总尝试 3 次、等待 25/75 ms；每次新建 QSaveFile、复查路径并完整写入。非 Windows 单次尝试，永久失败返回 CacheIo 并保留旧缓存。
- 调查确认原偶发失败位于覆盖已有文件时的 QSaveFile::commit，Qt RenameError / Windows ERROR_ACCESS_DENIED (5)；精确外部触发源仍未定位。确定性文件占用测试与压力回归证明本修复的恢复和保留语义，不代表已识别某个外部进程或根除所有 CacheIo。
- 41.md 修复验证：确定性 transient / permanent denial PASS；Debug / Release 原 epssCache stress 各 200/200 PASS、0 FAIL，耗时 14.35 / 12.78 s。42.md 历史封版 Debug / Release 各 158/158 PASS；后续历史及最新 Phase 13 完整回归见第 17 节。
- 用户 Debug GUI 人工验收为 PASS WITH NON-BLOCKING UNVERIFIED ITEMS，Confirmed FAIL：0；Release GUI 为 AUTOMATED ONLY。三个未验证项永久如实保留于第 18 节。

**Phase 10 —— COMPLETE。**

- 从当前 confirmed Finding 的 Profile 生成 Priority / Decision Evidence Support / Deterministic Explanation；Rules v1、KEV precedence、最大 Fresh EPSS percentile、0.90 research threshold、stale context-only、future-time Ineligible 均已实现。
- 本地 single-shot timer 按真实 UTC 重新评估；同代 Profile / Assessment 原子发布，刷新 / 取消保留旧结果、失效同步清空；无自动联网。
- 46.md 封版时 Debug / Release 全量 CTest 各 182/182 PASS（原 Phase00—09 158/158、Phase10 24/24）；维护性与隐私审查覆盖十个正式提交文件。
- Codex GPT-6 Astra Ultra 在用户授权下执行完整 GUI 验收并保留逐项直接证据；用户接受该验收结果。13 项 PASS、Confirmed Functional FAIL 0、Blocked 0、Mandatory Not Verified 0；Release GUI 为 AUTOMATED ONLY。
- App 0.11.0 / schema 4；独立 completion commit 与 annotated `phase-10-complete` 封版标识见第 5 节。

**Phase 11 —— COMPLETE / FINAL SEAL PASS。**

- Validation Core、方法验证页面和离线 Replay 共享生产 Rules v1 evaluator；生产阈值 0.90 不变，实验比较 0.85 / 0.90 / 0.95，不修改生产设置。
- 六个公开冻结输入固定 40 个真实样本及完整构造审计；54 个独立 synthetic cases 覆盖规则边界。数据、结果、适用范围见第 16.1 节；不作 General Benchmark 或预测准确率声明。
- 首次审查的两项 Must Fix / 两项 Should Fix 已修复，第二次 ChatGPT 独立源码审查 PASS；Debug / Release 最终 Build、CTest 各 222/222 和 frozen Replay 均 PASS。
- GUI / Acceptance 20/20 PASS；Codex 执行 / 验证 Test 1–16、20，用户亲自执行 Test 17–19。Release GUI 为 AUTOMATED ONLY，完整来源见第 18 节。
- App 0.12.0 / schema 4；唯一 completion commit 已推送，annotated `phase-11-complete` 已推送并核验，固定标识见第 5 节。

**Phase 12 —— COMPLETE / FINAL SEAL PASS。**

- 按 52.md 增加 Current Project 概览、共享中文结果解释、明确的 evidence / freshness / CVE 关联语义与只读方法元数据；App 0.13.0，schema 4、Rules v1、生产阈值 0.90 不变。
- 新增 26 项注册测试，历史 222 项保留，Debug / Release 各 248/248 PASS；Phase 11 冻结字节与全部实验科学字段一致。
- First ChatGPT Source-level Code Review：CHANGES REQUIRED（Blocker=0 / Must Fix=3 / Should Fix=2）；五项 fixes implemented。Phase10.ui 历史导航已兼容，未降低断言。Second ChatGPT Source-level Code Review：PASS（Blocker=0 / Must Fix=0 / Should Fix=0 / Confirmed Functional FAIL=0 / Frozen Design Deviation=0 / Scope Creep=0）。GUI 为 PASS WITH NON-BLOCKING UNVERIFIED ITEMS；Test 20 保持 NOT VERIFIED / NON-BLOCKING。

**Phase 13 —— COMPLETE / FINAL SEAL PASS。**

- selected confirmed Finding 的 self-contained UTF-8 HTML 导出已实现；local-only / static snapshot，无导出网络请求，复用 PresentationText，immutable ReportContext、worker render/write、QSaveFile atomic save、single-flight 与来源字段 allowlist。
- App 0.14.0 / schema 4；新增 23 项测试，Debug / Release 各 271/271 PASS，历史 248 项保留；冻结输入与科学结果不变。
- Second-party / ChatGPT Source-level Code Review PASS：Blocker=0、Must Fix=0、Frozen Design Deviation=0、Scope Creep=0、Confirmed Functional Defect=0。GUI / Report Acceptance 19/19 PASS；准确 provenance 见第 18 节。

# 15. 当前真实工程结构

Phase 11 新增 `src/ValidationDataset.h/.cpp`、`ValidationSelection.h/.cpp`、`ValidationExperiment.h/.cpp`、`ValidationSynthetic.cpp`、`ValidationPage.h/.cpp`、`ValidationBuildInfo.h.in`、`tests/Phase11Test.cpp`；准备工具为 `tools/prepare_validation_snapshots.py` 与 opt-in `tools/ValidationPrepare.cpp`，正式离线重放入口为 `tools/ValidationReplay.cpp`。Phase 12 新增 `OverviewPage.h/.cpp`、`PresentationText.h/.cpp`、`RiskPresentationView.h/.cpp` 和 `tests/Phase12Test.cpp`。Phase 13 新增 `src/FindingReport.h/.cpp` 和 `tests/Phase13Test.cpp`，ProjectPage 捕获并协调导出，VulnerabilityPage 提供入口。公开输入位于 `validation/`，六个冻结文件见第 16.1 节；下方为当前正式文件树。

正式源码结构：

```text
D:\codex\Graduation Project\project\
├─ .gitignore
├─ CMakeLists.txt
├─ PROJECT-HANDOFF.md
├─ src\
│  ├─ main.cpp
│  ├─ MainWindow.h / .cpp
│  ├─ OverviewPage.h / .cpp
│  ├─ PresentationText.h / .cpp
│  ├─ RiskPresentationView.h / .cpp
│  ├─ FindingReport.h / .cpp
│  ├─ AppPaths.h / .cpp
│  ├─ AppLogger.h / .cpp
│  ├─ AppSettings.h / .cpp
│  ├─ AppDatabase.h / .cpp
│  ├─ Project.h
│  ├─ ProjectRepository.h / .cpp
│  ├─ Component.h
│  ├─ ComponentRepository.h / .cpp
│  ├─ DependencySnapshot.h
│  ├─ DependencyAnalyzer.h / .cpp
│  ├─ DependencyPage.h / .cpp
│  ├─ PackageIdentity.h / .cpp
│  ├─ Semver.h / .cpp
│  ├─ VersionApplicability.h / .cpp
│  ├─ CveIdentity.h / .cpp
│  ├─ RiskEvidence.h / .cpp
│  ├─ RiskPriority.h / .cpp
│  ├─ RiskEvidenceCache.h / .cpp
│  ├─ EvidenceClients.h / .cpp
│  ├─ RiskEvidenceController.h / .cpp
│  ├─ OsvResponseParser.h / .cpp
│  ├─ OsvCache.h / .cpp
│  ├─ OsvClient.h / .cpp
│  ├─ VulnerabilityController.h / .cpp
│  ├─ VulnerabilityPage.h / .cpp
│  ├─ ProjectPage.h / .cpp
│  ├─ SbomDocument.h
│  ├─ CycloneDxParser.h / .cpp
│  ├─ SbomQualityAnalyzer.h / .cpp
│  ├─ SbomImportDialog.h / .cpp
│  ├─ ValidationDataset.h / .cpp
│  ├─ ValidationSelection.h / .cpp
│  ├─ ValidationExperiment.h / .cpp
│  ├─ ValidationSynthetic.cpp
│  ├─ ValidationPage.h / .cpp
│  └─ ValidationBuildInfo.h.in
├─ tools\
│  ├─ prepare_validation_snapshots.py
│  ├─ ValidationPrepare.cpp
│  └─ ValidationReplay.cpp
├─ validation\
│  ├─ candidate-pool-manifest-v1.json
│  ├─ candidate-pool-manifest-v1.json.sha256
│  ├─ candidate-selection-index-v1.json
│  ├─ candidate-selection-index-v1.json.sha256
│  ├─ phase11-validation-dataset-v1.json
│  └─ phase11-validation-dataset-v1.json.sha256
└─ tests\
   ├─ Phase00SmokeTest.cpp
   ├─ Phase01Test.cpp
   ├─ Phase02Test.cpp
   ├─ Phase03Test.cpp
   ├─ Phase04Test.cpp
   ├─ Phase05Test.cpp
   ├─ Phase06Test.cpp
   ├─ Phase07Test.cpp
   ├─ Phase08Test.cpp
   ├─ Phase09Test.cpp
   ├─ Phase10Test.cpp
   ├─ Phase11Test.cpp
   ├─ Phase12Test.cpp
   └─ Phase13Test.cpp
```

`.git` 为版本元数据；本地 `build-debug/`、`build-release/`、Qt Creator 的 `build/` 和 `.qtcreator/` 均被忽略，不进入正式提交。运行时数据库、日志和配置位于应用数据目录，不属于正式源码；测试数据库位于临时目录，截图和构建日志留在被忽略的构建目录。

人工验收用 `phase05-large-sbom.json`、`phase05-replacement-sbom.json`、`phase05-empty-sbom.json`、`phase03-demo-sbom.json`、`phase03-not-cyclonedx.json`、`phase03-invalid-json.json`、`phase04-clean-sbom.json`、`phase04-quality-issues.json` 由测试生成于构建目录，全部为 synthetic 数据，保持 ignored / not tracked / not staged / not committed。

`build-debug/phase05-manual.cmd`、`build-debug/phase06-manual.cmd`、Phase 06 normal / issues / cycle / empty / replacement / parser-error / large synthetic JSON、人工验收数据目录及截图继续保留于 ignored 构建目录；Release 自动生成的对应产物也保持 ignored / not tracked / not staged / not committed。Phase06Test 仅在人工数据库不存在时生成 synthetic schema 3 起点，不覆盖用户已有人工验收数据库。

临时理解报告已按封版授权删除，未进入任何正式提交；唯一长期动态交接文档为本文。

Phase 07 的 public / large / empty / invalid SBOM、cache fixture、launch 脚本、人工验收说明、截图、manual DB / logs、live smoke 记录均留在 ignored 构建目录；`phase07-manual-data` 不提交、不覆盖或删除。测试源码仅包含公开包或 synthetic 构造数据，不提交运行时 response dump。`.gitignore` 另覆盖 `**/cache/osv-v1/`。

当前应用版本为 **0.14.0**，唯一来源是 CMake project VERSION，经 APPLICATION_VERSION 传入 QApplication::setApplicationVersion、设置页和回放构建信息，日志复用 applicationVersion()。schema_version 保持 **4**，现有六张表（app_meta 元数据表 + 五张业务表）及迁移未改动。

| CMake target | 当前职责与依赖边界 |
| --- | --- |
| SbomParsing | CycloneDX 解析与质量诊断；Qt Core |
| DependencyAnalysis | 原始快照与内存依赖图；Qt Core |
| AppFoundation | 路径、日志、设置、数据库与 Project / Component repositories；Qt Core / Sql |
| VulnerabilityCore | CveIdentity、PackageIdentity、OSV response / cache、Semver、VersionApplicability；Qt Core |
| RiskEvidenceCore | 证据值模型、校验、纯值 RiskPriorityEvaluator 与解释；VulnerabilityCore / DependencyAnalysis，不依赖 Widgets、Network 或 Sql |
| PresentationCore | PresentationText 的确定性中文标签与解释片段；仅 RiskEvidenceCore / Qt Core，不依赖 Widgets / Sql / Network，不计算规则或新鲜度 |
| PresentationWidgets | RiskPresentationView 组合结果、原因、驱动、关键证据与边界；PresentationCore / Widgets，复用于漏洞与方法验证页面 |
| FindingReportCore | immutable FindingReportContext → self-contained UTF-8 HTML 与 QSaveFile 原子保存；复用 PresentationCore，无 UI / DB / Network 访问 |
| ValidationCore | Profile 编解码、数据契约、准备期去重、共享 evaluator 实验与合成规范；只依赖 RiskEvidenceCore / Qt Core，无 Widgets / Network / Sql |
| ValidationWidgets | 方法验证页面；ValidationCore / PresentationWidgets / Widgets / Concurrent；首次显示时后台校验，失败不运行实验 |
| ValidationReplay | 无网络离线重放与 ignored JSON artifact；只依赖 ValidationCore |
| ValidationPrepare | 显式 opt-in research fixture preparation；复用现有 EPSS/KEV transport/parser，不是 GUI 用户功能 |
| RiskEvidenceServices | EPSS / KEV clients、原子缓存、后台协调及本地 assessment freshness timer；RiskEvidenceCore / AppFoundation / Network / Concurrent |
| OsvMatching | OsvClient、VulnerabilityController；继续拥有 Finding，向 RiskEvidenceController 发出不可变请求 |
| SupplyChainRiskAssessment | Widgets 应用与页面组合，main 连接 ProjectPage 权威选择与 OverviewPage；直接链接 ValidationWidgets、PresentationWidgets 与 FindingReportCore，未链接 Qt Svg |
| Phase00SmokeTest—Phase13Test | 自动回归；Graphviz 仅用于 Phase00 smoke，详细数量见第 17 节 |

Phase 09 / 10 fixtures、审查包 / review patch、调查与验收日志、生成的 cmd / checklist / JSON、缓存、数据库及截图留在 ignored build 目录，不进入正式提交；`.gitignore` 覆盖 `**/cache/risk-evidence-v1/`。

# 16. 当前已经实现的功能

当前已经建立 **Phase 00 工程基础 + Phase 01 Application Foundation + Phase 02 Project Management + Phase 03 CycloneDX SBOM Import + Phase 04 SBOM Quality Diagnosis + Phase 05 Component Persistence + Phase 06 Dependency Analysis + Phase 07 Component Identity / OSV Candidate Matching + Phase 08 Version Applicability / Finding + Phase 09 Risk Evidence Enrichment + Phase 10 Exploit-Signal Prioritization + Phase 11 Validation & Threshold Sensitivity Experiment + Phase 12 Presentation & Explainability + Phase 13 Current Finding Analysis Report Export**。业务链见第 9 节；预览文档、质量报告、Finding、Profile 与 Assessment 仍在内存中，当前组件、原始依赖及 Captured State 在明确 Apply 后统一持久化。

默认运行目录由 `QStandardPaths::AppDataLocation` 决定，Windows 通常为 `%APPDATA%/GraduationProject/SupplyChainRiskAssessment`：数据库 `data/supply_chain_risk.db`、日志 `logs/application.log`、配置 `settings.ini`。可用 `--data-dir <绝对路径>` 指定隔离数据根目录，空路径或相对路径会被拒绝。应用自行保存的 UI 配置仅含 `ui/lastNavigationPage`，启动及切换时保存，重启时恢复，未知页面回退概览。

AppLogger 以追加方式写入 UTC 时间、级别和单行消息，写入失败回退标准错误输出；启动时打开或首次写入失败还会显示警告，并继续运行。不能据此宣称所有后续写日志失败都会弹窗。本地基础日志包含运行目录，项目创建 / 删除日志包含项目 ID；它们仍属私有运行数据，不能把 Phase 03 导入日志的安全摘要约束解释为整个日志可以直接公开。

Project 仅包含 `id`、`name`、`description`、`createdAt`：应用生成无花括号 UUID；名称 trim 后必填、1—100 个 Unicode 码点，允许重名；描述 trim 后可空、最多 500 个码点；创建时间为 UTC Unix 毫秒，界面显示本地时间。列表按 `created_at DESC, id DESC` 排序。创建后自动选中；选择项目显示详情；删除需二次确认，取消不写数据库，删除最后一项恢复空状态。

**当前数据库：schema_version = 4；仅有 app_meta、projects、components、dependency_capture、dependency_entries、dependency_targets 六张表，其中 app_meta 为元数据表，其余五张为业务表。**

```sql
CREATE TABLE app_meta (
    key TEXT PRIMARY KEY NOT NULL,
    value TEXT NOT NULL
);

CREATE TABLE projects (
    id          TEXT PRIMARY KEY NOT NULL,
    name        TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    created_at  INTEGER NOT NULL
);

CREATE TABLE components (
    id           TEXT PRIMARY KEY NOT NULL,
    project_id   TEXT NOT NULL,
    source_role  INTEGER NOT NULL CHECK(source_role IN (0,1)),
    source_order INTEGER NOT NULL CHECK(source_order>=0 AND (source_role=1 OR source_order=0)),
    bom_ref      TEXT NOT NULL DEFAULT '',
    type         TEXT NOT NULL DEFAULT '',
    name         TEXT NOT NULL DEFAULT '',
    version      TEXT NOT NULL DEFAULT '',
    purl         TEXT NOT NULL DEFAULT '',
    FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE
);
CREATE UNIQUE INDEX components_project_source
    ON components(project_id,source_role,source_order);

CREATE TABLE dependency_capture (
    project_id TEXT PRIMARY KEY NOT NULL,
    FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE
);
CREATE TABLE dependency_entries (
    id TEXT PRIMARY KEY NOT NULL,
    project_id TEXT NOT NULL,
    source_order INTEGER NOT NULL CHECK(source_order>=0),
    source_ref TEXT NOT NULL DEFAULT '',
    FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE
);
CREATE UNIQUE INDEX dependency_entries_project_order
    ON dependency_entries(project_id,source_order);
CREATE TABLE dependency_targets (
    id TEXT PRIMARY KEY NOT NULL,
    dependency_entry_id TEXT NOT NULL,
    target_order INTEGER NOT NULL CHECK(target_order>=0),
    target_ref TEXT NOT NULL DEFAULT '',
    FOREIGN KEY(dependency_entry_id) REFERENCES dependency_entries(id) ON DELETE CASCADE
);
CREATE UNIQUE INDEX dependency_targets_entry_order
    ON dependency_targets(dependency_entry_id,target_order);
```

AppDatabase 每次打开连接先启用并核验 `PRAGMA foreign_keys=ON`，再在同一事务中完成 fresh schema 4 / v1 → v2 → v3 → v4 / v2 → v3 → v4 / v3 → v4 初始化或迁移；先建表和索引，再更新版本，全部成功才提交。失败整体回滚，冲突表或索引不被覆盖，未知版本拒绝打开，不自动降级或重建用户数据。测试使用 QTemporaryDir，程序冒烟使用临时 `--data-dir`，不污染真实数据。

Component 的 `id` 仅是数据库行 UUID；每次成功整组替换生成新 ID，不按 bom-ref / PURL / name / version 保留 ID。`source_role=0` 为单独的 metadata root，order 固定为 0；`source_role=1` 为 Parser 展平后的普通组件，order 从 0 开始，包含根组件下展开的子组件。order 不是原 JSON path 或嵌套数组下标。索引唯一性约束存储位置，不代表软件包身份；五个原始字段保留空值、空白、大小写、Unicode 和重复标识，不 trim / normalize / 去重。

ComponentRepository 的 `replaceForProject` 在一个事务内完成 BEGIN → 验证 Project → 删除旧 dependency entries（targets cascade）、capture 标记和 components → 插入完整组件、raw entries / targets → 插入 capture 标记 → COMMIT。任一步失败均回滚，旧组件 UUID、依赖行及 capture 状态完整保留，不影响其他项目；不能拆成独立提交。无 root 且普通组件列表为空会清空组件，仅 root 则保存一条 root。ProjectRepository 只删除 Project，组件、capture、entries 及下属 targets 由数据库 ON DELETE CASCADE 统一清理；UI 不组合 delete / insert。

`dependency_capture` 中无项目行即 Not Captured，有行即 Captured；schema 3 升级不插入此标记，不能把老项目解释为 0 dependencies。成功 Apply 即使没有 dependency entries 也写入标记，形成 Captured Empty。entries / targets 使用新 UUID 和从 0 开始的来源顺序，保留空 ref、空白、重复及空 dependsOn entry，不做 normalization / deduplication，不保存导入路径、时间、history 或业务 revision。

**CycloneDX 支持范围：1.4 / 1.5 / 1.6 的共同字段子集，不是完整 Schema validator。** 读取 bomFormat、specVersion、serialNumber、BOM version、metadata timestamp / root component、components 和 dependencies。SbomComponent 包含 bomRef / type / name / version / purl；SbomDependency 包含 ref / dependsOn。嵌套 components 按顺序展开，metadata root 单独保存且不计入组件数，其子组件参与展开；不从层级隐式生成依赖。缺失 version / purl / bom-ref 等允许为空，PURL 保持原值；解析成功后由独立 Analyzer 诊断质量问题。

CycloneDxParser 接收 QByteArray 或本地文件，输出 SbomParseResult / SbomDocument，仅依赖 Qt Core，不依赖 UI、SQLite 或 AppLogger。文件有界读取，集中限制为 50 MiB、组件嵌套深度 128、组件数 / dependency entries / dependency targets 总数各 100000。深度按收集 components 数组计算，metadata root 单独保存；这不是任意 JSON 字段的统一深度上限。50 MiB 限制输入字节数，不代表解析期间进程内存仅占 50 MiB。错误区分 FileOpenError、FileReadError、FileTooLarge、InvalidJson、NotCycloneDx、MissingSpecVersion、UnsupportedSpecVersion、InvalidStructure、StructureLimitExceeded；失败不返回半完成 Document。

必需头字段为 bomFormat = CycloneDX 与受支持的字符串 specVersion。共同子集的可选字段缺失时保留空值；字段存在但类型错误（包括 null）会失败，未知字段忽略。BOM version 若提供，须为 1 至 9007199254740991 的整数；不校验 PURL 语义、UUID、时间戳格式或依赖引用有效性。

ProjectPage 在触发时重新验证选中 Project，打开窗口模态的 SbomImportDialog。文件选择使用 Windows 原生 QFileDialog；Qt Concurrent 后台任务只捕获文件路径值，先调用 Parser，成功后调用 Analyzer，返回含解析结果与质量报告的 ImportResult，不访问 UI、数据库或 logger；QFutureWatcher 在 GUI 线程交付结果。QObject 所有权及连接上下文保证窗口销毁后不再向它交付结果；关闭窗口不会主动取消已启动的解析或诊断，后台任务继续结束，应用退出可能等待线程池完成。进行中的导入不接受重入，未增加第二套线程机制。

成功状态仅在候选文件解析成功且质量分析正常返回完整报告后更新；Quality Error 仍是成功导入。取消、Invalid JSON、non-CycloneDX、unsupported version 或文件读取错误均保留旧 SbomDocument、SbomQualityReport 和预览；首次失败保持空状态。这里的分析成功指正常完成诊断，不要求 issues 为空，不表示已实现独立的分析错误分类或内存耗尽恢复。

组件预览页签包含文档摘要和只读组件表，摘要显示文件名而非完整路径。组件单元格及摘要中的 serialNumber / 根组件名称 / 时间戳超过 512 个 UTF-16 单元时在显示层截断并加省略号，内存模型保留原文。质量诊断页签显示 Total / Error / Warning / Info 计数和 Severity / Code / Location / Description 问题表；位置使用从 1 开始的组件、依赖及目标编号或独立的元数据根组件位置，不依赖唯一 name。两张表均以 QAbstractTableModel 按需提供可见单元格，支持滚动；默认 900×600 和最小 560×400 布局已验证。clean state 明确显示“当前诊断规则未发现问题。”，不宣称 SBOM 完全正确。

关闭窗口即结束此次预览与报告，重新打开为空；已 Apply 的当前组件、原始依赖及捕获状态继续保存在 SQLite。应用业务层不持久化导入路径或历史，此结论不涵盖系统文件选择框自身的最近位置记录。导入对话框继续仅显示依赖条目数量及质量问题定位；已 Apply 的原始依赖明细和可靠关系由项目“依赖关系”页提供，未接入 Graphviz 绘图。导入与诊断日志仅记录固定状态、错误类别、组件 / 依赖数量及 errorCount / warningCount / infoCount，不记录 SBOM 原文、组件清单、PURL、bom-ref、完整路径、依赖图或整个 issue list；既有基础日志仍遵循前述私有运行数据边界。

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

**显式 Apply（Phase 05 建立，Phase 06 扩展完整 Current State）：** 成功预览不等于持久化；关闭预览而不 Apply，数据库不变。Quality Error 有明确提示，仍允许保存原始组件及依赖，不转为 Parse Failure。ApplyState 为 Empty / Ready / Working / Applied；开始即 Working，禁用 Apply、选文件和关闭，阻止重复请求；成功为 Applied，同一预览不能再次 replacement，新成功预览才恢复 Ready。失败保留 candidate 和旧持久化数据，恢复 Ready 允许重试；后续 Parser Failure 不改变 candidate 或其 Applied 状态。

Apply 任务仅值捕获同一正式数据库路径、Project ID 和 SbomDocument；ComponentRepository::replaceInFile 在后台线程创建局部 AppDatabase 和 Repository，连接、查询、事务及 RAII 清理均在该线程完成，不传主线程 QSqlDatabase / QSqlQuery，不访问 UI 或 logger。QFutureWatcher 在 GUI 线程更新状态并通知项目页；窗口强制销毁后不会交付到已销毁 QObject，后台任务继续完成，退出可能等待线程池，不承诺取消事务。

ProjectPage 的“当前组件”页通过 Repository 从 SQLite 按 source role / order 读取只读快照；选择、刷新及成功 Apply 后重读，SQLite 为业务权威来源。显示来源 / Name / Version / Type / PURL / bom-ref，root 单独标识，可见单元格按需格式化并截断超长显示，原数据不变。正常及最小窗口布局、项目切换、重启、删除及滚动已验证。当前列表读取仍在 GUI 线程，Phase 05 封版的 100000 条实际加载记录为 Debug 568 ms / Release 530 ms；后续真实输入规模及字段长度变化须重新评估。

Phase 05 封版自动回归中，100000 条完整 replacement（含删除旧 100000 条、插入及提交）Debug 1789 ms / Release 1466 ms；GUI Apply 派发均 0 ms，完成 2978 / 3238 ms，观测到事件循环唤醒 178 / 198 次、最大间隔 65 / 34 ms。用户人工观察约 1 秒，分别记录不同运行，不相互替代；均为当前机器验收证据，不是长期性能保证。Apply 日志仅记录固定结果类别，内部 SQL diagnostic 不直接显示或记录。

**Phase 06 依赖分析：** ComponentRepository::readSnapshot 在同一读事务内依次验证 Project、读取 Components / Capture / Entries / Targets；第一个 SELECT 建立 SQLite snapshot，成功提交后才交付完整候选值，避免把组件状态 A 与依赖状态 B 拼接。后台 readSnapshotInFile 复用 AppDatabase，在 worker 内创建并销毁连接及查询，使用同一正式数据库文件，不建立第二套数据库体系。

DependencyGraph 只依赖 Qt Core，持有不可变快照、引用结果、metrics 和正向 / 反向邻接表。空或纯 whitespace 为 Missing；其他 ref 按未修改的原始字符串 exact match（包括首尾空白和大小写）。0 个匹配为 Unknown，1 个为 Resolved，多个为 Ambiguous，metadata root 同样参与索引；不 trim 后匹配、不 case-fold / normalize，不使用 PURL / name / version / fuzzy fallback。

Raw metrics 统计 Entry 数、Target 数以及 source 每条一次加 target 每次出现的 Missing / Unknown / Ambiguous occurrences；非缺失 source == target 每次原始出现计为 Self-Dependency Occurrence，包括 Unknown / Ambiguous 自引用。可靠边只接受两端均 Resolved，跨重复声明去重，并以 first resolved occurrence 建立确定性正向 / 反向顺序；不会遍历 QHash / QSet 生成 UI 顺序。Resolved Unique Edges 包含可靠 unique self-loop，Direct Dependencies / Dependents 可以显示自身并明确标注。

每次有效分析构建一次图；选择组件或关系方向复用当前图，执行 on-demand BFS。Transitive 包含 Direct，Shortest Depth >= 1，按最短深度与首次发现顺序输出；visited 初始即包含起点，因此 self-loop / cycle 不会让起点重回结果，起点无 shortest depth。内存图规模为 O(V+E)，所选查询仅使用临时 visited / queue 和当前结果，无 all-pairs closure、全量最短路径或每组件永久 closure cache。以上派生图、depth、adjacency、request generation 均不写入 SQLite。

DependencyPage 作为 ProjectPage 的“依赖关系”页签，提供摘要、原始 Entries / Targets 和四种组件关系查询。Not Captured 明确提示重新导入 Apply，不显示 0 dependency metrics；Captured Empty 明确说明当前已应用 SBOM 未声明 entries。QAbstractTableModel 共享不可变图，按需格式化可见单元格，超长显示截断但数据不变；最小窗口通过滚动区域保留双表可用视口。

Qt Concurrent 在后台完成一致读取和图构建；QFutureWatcher 回 GUI 线程安装。每个结果绑定 Project ID + request generation；切换 / 删除项目、成功 Apply、Reload 或更新请求立即失效并清空旧结果，过期交付安全丢弃。连续请求合并为当前任务加最新待执行请求；所选关系查询独立 generation，捕获 shared immutable graph，窗口销毁后 worker 不访问 UI / logger 或借用连接。后台任务自然结束，未承诺主动取消。SQLite 是业务权威来源，Runtime Graph 是可失效、可重新构建的派生状态。

2026-09-18 Phase 06 封版自动测试的 100000 Components + 100000 Entries + 100000 Targets 实测如下；均返回 99999 条传递结果，末项 shortest depth 为 99999：

| 测量 | Debug | Release |
| --- | --- | --- |
| 原子持久化 / 一致 snapshot / 构图 | 3757 / 3299 / 537 ms | 3081 / 3866 / 207 ms |
| ref index / resolution + metrics / 双向 adjacency | 73.963 / 279.399 / 150.781 ms | 22.9239 / 96.2969 / 66.6009 ms |
| 两次 BFS | 106 ms | 48 ms |
| GUI Apply 派发 / 完成 | 0 / 9674 ms | 0 / 7202 ms |
| GUI analysis 派发 / 完成 | 0 / 3679 ms | 0 / 3114 ms |
| GUI query 派发 / 完成 | 0 / 63 ms | 0 / 66 ms |
| 分析 / 查询期间事件循环唤醒 / 最大观测间隔 | 235 次 / 37 ms | 193 次 / 49 ms |

用户在开发后的人工验收另行确认“大规模响应很快、没有明显卡顿或未响应”，详见第 18 节。自动耗时与人工体验分别记录，不能互相替代；这些只是当前机器的运行证据，不是长期性能保证。真实 SBOM 可能缺失依赖声明，可靠图中的空关系不证明没有依赖；dependency analysis 不等于 runtime reachability。

**当前 SQLite 持久化范围仅为 Project、Current Components、Raw Dependencies 与 Capture State。** 无 sboms / quality_reports / quality_issues / identity / vulnerability / finding / risk / profile / scan / reports 表；Phase 07—13 均未升级 schema。Project 不保存 sbomPath 或 import history。OSV / EPSS / KEV 文件缓存是外部证据，不是第二业务数据库。

## Phase 07：身份、候选、缓存与异步边界

`ComponentRepository::listForProjectInFile` 在 worker 自有连接和单读事务内验证项目并读取 Current Components，不读取整个 DependencySnapshot。身份解析在同一后台任务完成，加载请求以 generation 合并；UI 只呈现结果，不执行 SQL。PackageIdentity 规则集中且不改写 Component，UUID 仅为 row identity；QueryIdentity 为 ecosystem / name / exact version / identityRulesVersion（当前 1）。

PURL 最长 4096 UTF-16 单元，scheme / type 不区分大小写；源文本采用严格 ASCII URL 子集，percent-encoded UTF-8 只解码一次，非法转义 / UTF-8 拒绝，`+` 不转空格。只支持 pypi / npm，无 name / type / bom-ref fallback；qualifiers 与 subpath 明确拒绝。PyPI 名称转小写并合并连续 `[-_.]`，无 namespace；npm 保留大小写，scoped npm 使用 `%40scope/name`，名称长度上限 214。

Identity 为 Resolved / Insufficient / Ambiguous，reason 包括 MissingPurl、MissingVersion、UnsupportedEcosystem、VersionConflict、MalformedPurl、UnsupportedQualifiers、UnsupportedSubpath 及长度 / 输入安全限制。版本来源为 Purl / Component / Both；两者完全相同或仅一个来源可 Resolved，两者明确不同为 Ambiguous / VersionConflict。纯空白视为缺失；非空字符串保留原值，不 trim、补位、转小写或移除 v prefix，最长 256 UTF-16 单元，拒绝控制字符及无效 UTF-16。epoch、local version、v prefix 或不被本地完整 parser 解释的形式不会仅因此被拒绝。身份解析本身不执行 PEP 440、SemVer applicability 或 range evaluation；**Identity Resolved ≠ Version Applicable**。

OSV 固定 HTTPS `POST https://api.osv.dev/v1/query`，正文只含 package.ecosystem / package.name / version，后续分页增加服务返回的 page_token。每次只查询所选组件，最大并发 1；自动分页支持 token-only page，重复 token 拒绝，最多 20 页、累计 10000 条原始记录、16 MiB 解码后响应字节。reply 读取缓冲为 64 KiB，单次 transfer timeout 30 秒，网络及解析链 total deadline 120 秒。Qt Network 异步 I/O，解析 Qt Concurrent 值任务；仅所有页完整成功才发布及缓存，不暴露部分成功。

错误区分 HTTP 400 / 401、403 / 404 / 429 / 5xx、Timeout、TlsFailure、ConnectionFailure、ResponseInvalid、ResponseLimitExceeded、UnexpectedRedirect、Cancelled 及 Cache / Database 错误。429 解析 Retry-After 并限制过早重发，没有自动重试或修改 identity 重试。禁止重定向自动跟随、自动 cookie 读取 / 保存与 TLS 绕过。QNetworkReply 完成后 deleteLater，销毁时断开回调并 abort；借用 transport 必须在相同线程且寿命覆盖 client。解析任务不捕获 UI。

Candidate 以 OSV ID 为主记录身份，保留完整 QJsonObject：summary、modified、published / withdrawn（若存在）、aliases、affected 及未知扩展字段；必要字段 / 已知结构校验后原样保留。CVE 只是 alias，可 0 / 1 / 多个，不按 CVE 合并 OSV 记录。同 ID 跨页按较新 modified 去重，比较保留亚毫秒精度；相同 modified 但内容冲突则整次失败。Parser / Candidate 层不对 affected evidence 求值，由下述 Phase 08 evaluator 解释；**Candidate ≠ Affected ≠ Finding**。

缓存路径为 `<AppData root>/cache/osv-v1/`，随 `--data-dir` 隔离。endpoint / rules / ecosystem / name / version 的结构化键经 SHA-256 生成安全固定文件名，SHA-256 不用于内容真实性或 package identity 判定。文件内部保存完整 QueryIdentity、endpoint、format / rules version、fetchedAt、complete 标记和 vulns，读取再次核对及解析，读写上限 20 MiB。QSaveFile 原子写；仅完整成功结果可写，完整零 Candidate 可写，失败 / 部分页不覆盖旧成功缓存。

24 小时 TTL 仅为刷新策略，不保证漏洞数据不变；支持 Fresh / Stale、Live / Cache、CacheMiss / CacheInvalid / CacheIo、仅本地缓存查询和显式清除。未来时间戳视为 stale。刷新失败仍可呈现以前完整快照，但状态明确 Failed、本次 Count 为未知，与历史快照分离。clear 仅删除受管的 64 位小写十六进制 `.json` 文件，不递归，不碰 SQLite / logs / 外来文件，跳过链接并拒绝缓存路径祖先中的 symlink / junction。没有完整离线漏洞库、LRU、100 MiB 全局配额、enterprise manager 或跨进程协调。

ProjectPage 第四页签为“漏洞匹配”。Identity 表显示状态 / reason / 包名 / 版本及来源；Candidate 表显示 OSV ID、CVE aliases、summary、modified、withdrawn。原始证据以纯文本异步生成，显示最多 65536 字符，完整记录仍保留；小窗口通过滚动容器可达全部操作。NotStarted / Working / Success / Failed / Cancelled 与 None / Live / FreshCache / StaleCache 独立；只有 Success 的完整快照显示确定的本次候选数，零结果文案为“当前查询未返回候选”。

首次实际联网前用非阻塞窗口模态确认框展示待发送 ecosystem / name / version，确认仅在当前 controller 生命周期内保留，不持久化。查询由用户明确点击；启动、Project switch、Apply、组件选择均不自动联网。不发送完整 SBOM、项目描述、UUID、bom-ref、本地路径、依赖图、SQLite、日志或其他组件；分页 token 只用于继续该查询。OSV 日志只记录固定错误代码，不记录包身份、正文、token 或 response dump。

加载与所选查询使用独立 generation；Project switch / selection / Reload / Apply success 使旧结果失效，Preview 与 Apply failure 保留旧状态。隐藏页切换再显示也会正确合并加载。查询 busy 覆盖 cache read → consent → network / parse → cache write → applicability evaluation，失效或取消不会提前释放仍运行的 worker；完成前禁用 clear，clear 期间禁止查询，避免缓存清除后被旧任务重新写回。该 OSV 查询链无 sleep、busy wait、nested event loop、mock endpoint 或 test-only switch；Phase 09 provider cache writer 的 Windows 有限等待是独立、已验证的文件替换策略，见下节。

Phase 07 封版轮 100000 Components 自动用例通过：Debug apply / read+identity+install / scroll+select 为 839 / 729 / 6 ms，GUI wakeups 63；Release 为 755 / 494 / 6 ms，GUI wakeups 50；两者 HTTP=0。这些性能仅为当时当前机器证据。Phase 08 已补齐 Applicability / Finding，Phase 09 已补齐 evidence enrichment；Risk Score / Risk Level / Assessment Confidence、bulk scan / querybatch、正式 Graphviz 业务与项目级报告仍未实现；Phase 13 已提供单 Finding HTML 报告。

## Phase 08：本地适用性、Finding 与解释边界

VersionApplicability::evaluateLocal 根据 QueryIdentity 选择同 ecosystem affected entry；PyPI 复用 PackageIdentity::canonicalPypiName，npm 精确比较。只有 package.name 精确为 * 且 ecosystem 匹配时为 wildcard；不改写 provider JSON。不匹配 entry 的无效范围不参与判断。

versions[] 使用 exact string equality，不进行 SemVer 规范化。SEMVER 使用严格 2.0 解析和 precedence（build metadata 不参与排序）；introduced 包含边界，fixed / finite limit 不包含，last_affected 包含。introduced="0" 为负无穷；缺 limit 为隐式正无穷，任何包含 "*" 的 limit 字符串为显式正无穷。多个有限 limit 使用 BeforeLimits 的任一满足语义。事件先整体验证再排序，同序列 fixed / last_affected 混用、缺 introduced、非法边界或冲突返回 Unknown。相同优先级的不同转换保守返回 InvalidRangeEvents。

可靠 positive evidence 优先；无 positive 时任何 unsupported / missing evidence 保留 Unknown，全部可解释且未命中才为 Local NotAffected。unknown / empty / missing range.type → UnsupportedRangeType，ECOSYSTEM → UnsupportedEcosystemRange，GIT → GitRangeRequiresCommitGraph。Provider source 为 OsvPackageVersionQuery；publish 将 Local NotAffected 改为 Published Unknown / ProviderEvidenceConflict。withdrawn 在 evaluateLocal 之前成为 Excluded，无普通 Local / Published 版本判断。

只有 Published Affected 创建 VulnerabilityFinding。Finding 绑定当前 componentId、QueryIdentity、OSV ID / CVE aliases、applicability evidence、snapshot identity、fetchedAt 与 source；所属 ApplicabilitySnapshot 保存 generation 和整批 assessments / findings / counts。派生结果不进 SQLite 或 OSV cache；Live / Fresh / Stale 原始快照都走相同 evaluator。

既有 VulnerabilityController::finishWithEvaluation 在 QtConcurrent worker 中值捕获快照，主线程校验 generation、component、snapshot identity 与 fetchedAt 后一次发布完整结果。切换组件 / 项目、reload、成功 Apply、cancel 使旧结果失效；Preview / Apply failure 保留旧状态。任务完成前保持 busy，不发布 partial batch；销毁后不调用 UI。无 SQL / Network / Widgets 混入 evaluator，Phase 08 Finding 的权威 owner 不变；Phase 09 新增 RiskEvidenceController 仅协调证据加载，不成为第二 Finding owner，也不引入 generic provider framework。

VulnerabilityPage 增加 Applicability / Reason / Finding 列及 Affected / Unknown / Excluded / Finding Count，解释展示 Local 与 Published、ProviderEvidenceConflict、wildcard、QueryIdentity / fetchedAt 和 Raw JSON；异步解释也使用 generation 防止过期覆盖。Unknown 不表示 Safe / 无风险；刷新失败时明确区分当前操作与保留的历史完整快照。

## Phase 09：风险证据与状态边界

RiskEvidenceRequest 仅由拥有当前 Finding 的 VulnerabilityController 创建，核验 component、OSV ID、query / snapshot identity、fetchedAt、generation、Candidate 唯一性和 Finding equality。请求查找为 O(C + F)，无逐 Finding 重复全扫描；RiskEvidenceController 只持有不可变请求和派生 Profile，不重新计算或接管 Finding。

Severity 只取 top-level 或与适用性证据匹配的 affected scope，保留 vector、source、recordId 与 provenance；冲突、缺失、不支持和结构错误分开表达。CVSS structural-only，不进行完整语义计算或本地打分。

EPSS 按请求长度分块，CVE 去重排序后以最多 2000 字符的逗号拼接列表分块，最多 10000 aliases；严格校验成功 envelope、完整性和数据行；Available 与 NotScored 分离，NotScored 不写成概率 0。KEV 校验完整目录及 count，CVE 后缀为 4–19 位；NotListed 仅来自完整有效目录，旧目录明确标记 Stale。CISA requiredAction / dueDate 是 provider evidence，不是本系统 SLA 或整改指令。 EPSS 单响应上限 2 MiB，KEV 上限 20 MiB / 50000 records；这些是应用资源边界，不能当作外部服务永久规格。

Freshness、Acquisition、Provider fact、操作状态和错误分别保存；24h TTL 只决定刷新政策。Cache Only 不联网，Prefer Cache / Refresh 按需请求，发送 EPSS CVE 前取得用户确认；失败可保留旧完整 Profile 或明确 stale fallback，取消和过期回复不发布部分结果。切换 Candidate / 组件 / 项目、reload、成功 Apply 使旧请求失效；Preview / Apply failure 保留旧结果。

Dependency Context 复用持久化一致快照，表达 root、resolved path、depth、dependents 和 declared reference resolution；不推断 runtime reachability 或完整 dependency coverage。Quality 明确为 UnavailableForPersistedCurrentState，不复用无归属的预览报告。

EPSS / KEV 各有 client / parser / cache，只有最小有界 GET transport 共用，不引入 generic provider framework。缓存只保存完整 provider 原始证据，不保存 Finding / Profile；QSaveFile atomic replacement 由后台 worker 执行。Windows 仅 commit=false 且 RenameError 时重试，3 attempts、25/75 ms；每次重查路径、新建 QSaveFile 并完整写入，永久失败保留旧文件且返回 CacheIo。其他错误不重试，非 Windows single attempt；无 direct-write fallback、target pre-delete、无限重试或敏感日志。

风险证据缓存根目录为 `<AppData root>/cache/risk-evidence-v1/`，下含 `epss-v1/<SHA-256(CVE)>.json` 与 `kev-v1/catalog.json`，随 `--data-dir` 隔离。EPSS 缓存按单 CVE 保存规范化完整成功响应（含 NotScored），KEV 保存完整目录；不保证还原 HTTP 原始字节。读取校验 endpoint、format、complete、fetchedAt 和内容；风险缓存未来时间戳返回 CacheInvalid，区别于上述 OSV 缓存的 stale 处理。现有“清除缓存”操作只清 OSV 受管文件，不清 EPSS / KEV 缓存；未实现风险缓存专用清除 UI、全局容量限制或跨进程协调。

完整 Profile 在 GUI 线程发布后才启动后台缓存持久化。已进入持久化的完整操作不可取消；新加载等待其结束，缓存写入失败只提供 cache warning，不改写已发布 provider fact。每个文件原子写不等于 EPSS 多文件和 KEV 目录组成一个跨文件事务。单个 HTTP 请求 timeout 为 30 秒，证据操作 deadline 为 180 秒；deadline 在发布完成后停止，不限制后续缓存持久化总时长。Windows 最多 100 ms 的重试额外等待是每个缓存文件的上限，不是整批操作耗时上限。

## Phase 10 Exploit-Signal Prioritization MVP

RiskPriorityEvaluator 是纯 Core/value evaluator：读取不可变 RiskEvidenceProfile 和调用方提供的 evaluationTimeUtc，输出 RiskPriorityAssessment；不访问 SQL / Network / Widgets，不重新拥有 Finding，不新增 Provider Framework。VulnerabilityPage 的既有 Risk Evidence 页内显示 Priority 与 Provider Snapshot；解释只消费 assessment，不在 UI 重做规则。

- PriorityClass：KnownExploited、AboveResearchPercentileThreshold、BelowResearchPercentileThreshold、InsufficientCurrentExploitEvidence。rulesVersion=1，epssPercentileThreshold=0.90（Research Default）。
- DecisionEvidenceSupport：Complete / Partial / Insufficient。Fresh KEV Listed 独立充分，KnownExploited / Complete 不因 EPSS 缺失降低 Support；否则 Fresh Available EPSS 的最大 percentile 驱动 Above / Below，只有所有可查询 CVE 同时具备 Fresh KEV NotListed 与 Fresh EPSS Available 才是 Complete，否则 Partial；无 Fresh driver 则 Insufficient / Insufficient。Missing / NotScored 不等于零，NotListed 不证明没有被利用。
- effectiveDecisionFreshness：Fresh / Stale / NotApplicable / Ineligible。可用证据 age < 24h 为 Fresh，age >= 24h 为 Stale；StaleFallback 保持 Stale。evaluatedAt < fetchedAt 的未来时间戳为 Ineligible，并输出 EvidenceTimeInvalidForCurrentDecision；无可用 provider fact 或 acquisition=None 为 NotApplicable。Phase 09 缓存入口仍拒绝未来 fetchedAt 为 CacheInvalid，Ineligible 防御通过纯 Core / renderer 自动覆盖。
- Stale KEV / EPSS 只作 Historical / Stale Evidence Available 上下文，不驱动当前 Priority；没有 stale provisional Priority 或 maxStaleAssessmentAge。
- CVE 先按 sortedCveIds canonical 顺序确定 provenance；多个 Fresh KEV Listed 取 canonical first，EPSS 取最大 Fresh percentile，只有 exact-equality 时 canonical first。排序不表达风险大小、重要性或 confidence；哈希仅查找，输出不依赖哈希迭代。
- EPSS driver provenance 包含 CVE、probability、percentile、FIRST API Version、scoreDate、fetchedAt、acquisition；Phase 09 providerVersion 表达 FIRST API 版本，不是 EPSS Model Version。KEV driver 包含 CVE、catalogVersion、dateReleased、fetchedAt、acquisition；保留 ProfileGeneratedAt、EvaluatedAt 与最早 nextFreshnessExpiryUtc。
- 本地 single-shot precise timer 只读取现有 Profile，在回调时以实际 UTC 重评估，不使用计划到期时刻冒充当前时间；无自动 provider refresh / 网络请求 / consent。Provider Snapshot 保留生成时 freshness，Priority 展示当前 effective freshness；两者语义不同。
- Profile 与 Assessment 同 key / profileGeneratedAt、同代原子发布；refresh / cancel 保留既有成功结果，timer 仍可令旧 Profile 的 assessment 自然过期。Candidate / Component / Project / Reload / Apply 相关失效同步清空两者并停止 timer；queued callback 校验 generation / key / profileGeneratedAt，销毁不访问失效对象。
- Severity / Dependency 只作 context；不计算 CVSS，不把 dependency path 当作 runtime reachability。Quality UnavailableForPersistedCurrentState 不改变 Priority / Support，不增加质量报告持久化。
- 复杂度为 canonical sorting O(N log N) + 评估遍历 O(N)，无 O(N²) 查找；不将整个 evaluator 宣称为 O(N)。实现验收时本机 synthetic 10000 CVE 为 Debug 29 ms / Release 11 ms，仅为历史本机指标，不是跨机器保证；51.md 最终回归中的 scale 用例亦 PASS，不将其阈值当作通用性能承诺。

## 16.1 Phase 11 验证与阈值敏感性实验

**实现范围。** 原生产 `RiskPriorityEvaluator::evaluate(profile, time)` 始终调用 0.90；新增实验入口只接受 0.85 / 0.90 / 0.95，其他值（含 NaN）拒绝。两入口共用原决定树，不在 ValidationCore 复制 Priority/Support 规则；解释使用实际运行阈值，并明确生产默认 0.90 不变。schema 4、生产 Finding 所有权、Controller、缓存、Settings 均无扩展。

**准备与冻结。** 候选发现仅来自 OSV 官方生态快照（[官方导出说明](https://google.github.io/osv.dev/data/)）；每个精确版本只能取自 matching affected.package 的 versions。相同生态与规范包名内，以 id+aliases 共享标识做传递闭包，不使用 related/upstream，不跨包合并。cluster IDs 按 QString UTF-16 code-unit 顺序去重排序，以 compact UTF-8 JSON 的 exact bytes 做 SHA-256；representative 取 `[ecosystem, canonicalPackageName, osvId, exactVersion]` SHA-256 最小者，不查看 EPSS/KEV/实验结果。cluster 审计信息不会增加 representative 的生产 CVE aliases。

| 官方输入 | 下载完成 UTC | exact source bytes / SHA-256 |
| --- | --- | --- |
| `https://storage.googleapis.com/osv-vulnerabilities/PyPI/all.zip` | 2026-09-30T08:44:10.717Z | 34,763,131 bytes；`f167f6190ebc3adec8a706fc94b13d01a55f8209da3902093ef99dbb6aa0e598` |
| `https://storage.googleapis.com/osv-vulnerabilities/npm/all.zip` | 2026-09-30T09:01:32.434Z | 217,014,458 bytes；`33dae49842fe5f4cdb06c3838633aa3a788612e7032106f8752ce5b4a9e53dab` |

分别解出 25,769 / 229,604 条公开记录；工具校验 ZIP exact hash/size 和解出记录，manifest 保留 Last-Modified、ETag、generation、Content-Length 与 recordsSHA256。OSV 代表记录只投影生产解释实际消费的字段；原始 ZIP 与完整解出记录保留在 ignored 准备目录。生产严重度、状态、acquisition、时间、错误、上下文等通过完整 codec 无损重建；无公开 SBOM 的样本使用正常 NotCaptured / QualityUnavailable，不伪造依赖上下文。

采集前即固定 outcome-blind frame：每生态按 representative selectionHash 取最多 512 个 CVE-bearing 和 16 个 no-CVE 单位，不因结果补样。实际 PyPI=528、npm=248，Candidate Pool=776。完成 discovery classification 后先冻结 pool，再单独 Final Capture；discovery evidence 不进入正式实验。Final Capture 使用生产客户端/parser、现有 QueryError 类别，对 transient 错误最多三次总尝试，失败不进入 S2。正式窗口为 **2026-09-30T09:09:21.719Z—09:09:56.893Z**，35.174 秒；统一 **evaluationReferenceTimeUtc=2026-09-30T09:09:56.893Z**，所有预期 Fresh 证据满足 fetchedAt≤reference、age<24h。6h 仅为采集 operational control，不改变生产 24h freshness。

正式 eligible 各层 S1/S2/S3/S4/S5/S6=53/40/19/11/14/639；按层内 selectionHash 升序及既定 quota 选择 **6/6/6/8/8/6，共 40**。FinalCaptureFailed=0、QuotaExceeded=736。源构造审计另记录 NoDeterministicExactAffectedVersion=212878、NotConfirmedFinding=28075、AliasEquivalentDuplicate=6615、DiscoveryFrameLimit=36071；这是审计行计数，不是互斥的漏洞总体频数。版本/候选处理可产生多条拒绝记录。

**Final diversity：** sampleCount=40、uniquePackageCount=32、uniqueOsvIdCount=39、uniqueCveAliasCount=33、uniqueAliasClusterCount=40、multiCveFindingCount=0。N≥30、S4≥1、S5≥1 和 uniqueAliasClusterCount=N 均通过。uniqueAliasClusterCount 按 ecosystem / package name / clusterHash 的组合计数；重复 OSV ID 可以对应不同包，实验单位不跨包合并。

| 公开冻结文件（均在 validation/；各有同名 .sha256 sidecar） | exact SHA-256 |
| --- | --- |
| candidate-pool-manifest-v1.json | `d72e4a9bbf4119ac497503d20bf3d55f153b8292452d83ca0d6dacddfdc2f7b8` |
| candidate-selection-index-v1.json | `6a749b97f08ada62706b5ae354c3702addd7f7974632dfae1e132675c1d626c4` |
| phase11-validation-dataset-v1.json | `25dde16b4bce0b9cb70bd767ef26e8a82aac2a8f03bc85a3e13c1efe2be96c4f` |

准备期缩进版材料保留原位，正式 repository bundle 以 compact JSON 打包，保持候选、排序、Profile、采集事实与 reference time 不变，只更新文件字节及交叉 hash；没有重新采集或按结果换样。每个 sidecar 均直接针对最终落盘的 exact bytes；不是把已存在文件 parse/normalize 后冒充其 byte hash。工具拒绝覆盖既有冻结文件。正式六个文件为已审查公开 research fixtures，纳入 Phase 11 completion 的显式文件集；本轮不改动其 exact bytes 或 sidecar。

**契约与实验。** Full Audit Loader原子验证版本、sidecar、交叉provenance、公开字段边界、时间窗口、完整capture、真实Profile重建、去重、排序、quota和样本门槛。Runtime Loader共享Dataset自身的schema/hash/time/capture/profile/sample严格检查，但不读取Manifest/Index、也不宣称重验pool选择历史。两种入口均坏数据整组拒绝，不静默丢样本。按 Load→Validate→Reconstruct→0.90 baseline→0.85→0.95→Analyze/Replay 执行，除阈值外输入一致。独立合成规范预先写定 Priority/Support/driver/解释事实，覆盖三阈值相等/相邻值、精确 24h、future、stale、KEV precedence、NotScored、multi-CVE/max/tie/order 和 Complete/Partial/Insufficient。

| threshold | Known Exploited | Above | Below | Insufficient |
| --- | ---: | ---: | ---: | ---: |
| 0.90 baseline | 6 | 14 | 14 | 6 |
| 0.85 | 6 | 22 | 6 | 6 |
| 0.95 | 6 | 6 | 22 | 6 |

三次 Support 均 Complete=34、Partial=0、Insufficient=6；driver 均 KEV=6、EPSS=28、None=6。相邻阈值各 8 个 Above→Below，0.85→0.95 共 16 个；其余不变。54 个 synthetic cases、KEV invariance、max Fresh EPSS、仅 EPSS-driven 单调性、driver/Support 不变、alias order、生产与实验 @0.90 一致、deterministic replay 和 explanation reproducibility 均 PASS。真实 multi-CVE / Partial 未覆盖的行为由 synthetic suite 承担，不把 synthetic 计入 N。

**以下比例仅描述本验证数据集。该数据集为分层目的性验证集，不代表 PyPI/npm 生态中的漏洞总体比例。** 未输出 Accuracy / Precision / Recall / F1、总体 prevalence 或“最佳阈值”结论。Percentile 是相对排序，不是利用概率。

**GUI 与重放入口。** 新增一级“方法验证”，首次显示才在无 UI 捕获的 worker 中加载，展示方法、SHA、时间、基线/三阈值分布、迁移、敏感样本和双语证据解释；阈值选择仅切换实验显示，不写 Settings 或生产规则。正式GUI仅嵌入Dataset及其sidecar两份输入；Phase11Test保留六份完整audit resources，ValidationReplay继续从外部validation目录完整审计。两种loader共享Dataset自身严格验证，GUI不重新证明776候选的选择历史；不需要运行时网络、数据库或.git。正式实验可离线复现，不承诺未来在线采集字节相同。

`ValidationPrepare` 由 `-DBUILD_VALIDATION_PREPARATION=ON` 显式启用，仅用于 research preparation；产品没有采集/构造数据集按钮。离线重放可运行 `build-debug/ValidationReplay.exe validation build-debug/phase11-validation-result.json`（沿用第 6 节进程 PATH）。结果 artifact 记录 applicationVersion、build-injected sourceGitCommit/dirty/sourceInputsSHA256、schema/dataset/pool/index 标识与 hash、统一时间、运行时间、规则版本、阈值、分布/迁移/合成检查/不变量，保持 ignored。正式结果不得作为仓库 fixture 提交。

## 16.2 Phase 11 首次 Code Review 修复（49.md）

首次 ChatGPT Source-level Code Review：**CHANGES REQUIRED；Blocker=0、Must Fix=2、Should Fix=2、Confirmed Functional FAIL=0、Scope Creep=0**。49.md 已修复两项 Must Fix 和两项 Should Fix 并通过自动回归。根据 51.md 提供的 **第二次 ChatGPT 独立 Source-level Code Review：PASS**；Blocker=0、Must Fix=0、Should Fix=0、Confirmed Functional FAIL=0、Frozen Design Deviation=0、Scope Creep=0。这是 ChatGPT 的独立审查结论，不是 Codex 自审 PASS。GUI / Acceptance 已按第 18 节闭环。

- Must Fix 1：成功KEV/每块EPSS及failure audit的attempts必须为1—3整数；两处failures数组按builder原样复制的稳定顺序做exact JSON equality。失败candidate的eligible/selected/reason/status/空stratum全部交叉锁定，成功candidate继续验证真实stratum/status/reason。失败CVE不得与成功capture重叠或重复；最终KEV失败与必需的成功catalog冲突即拒绝；full audit要求成功与失败CVE集合完整对应pool请求。没有重新设计capture schema。
- Must Fix 2：discovery与readUnit均通过生产PackageIdentity::resolve验证实际exactVersion，并核对解析后的ecosystem/name/version完全等于预期；不复制validText、不修改生产identity业务规则。先行兼容性门全部通过：776/776 pool、40/40 final samples；C0/C1、非法UTF-16、超长与空版本拒绝。
- Should Fix 1：新增loadRuntimeDataset/loadRuntimeDirectory供ValidationPage使用，保留loadDataset/loadDirectory完整审计入口供Replay、准备验证和contract tests。Runtime仍以原始capture+representative重建生产Profile并全等比较，验证sidecar/schema/metadata格式/时间/身份/排序/唯一cluster/分层/coverage；源快照时间仅验证Dataset内合法性，是否等于Manifest来源时间由Full Audit补验。产品resource移除full Manifest/Index，只保留原Dataset+sidecar；测试target保持full resources。
- Should Fix 2：明确增加N=29、N>=30但S4=0、N>=30但S5=0三种内存变异，Runtime与Full Audit均在coverage gate拒绝。另有重试/失败矛盾、合法失败审计正例、生产等价identity、Runtime契约与两种loader结果全等测试。

三份JSON及三个sidecar exact bytes/SHA-256与第16.1节一致，未重新下载、采集、采样或替换样本；Rules v1、三阈值、strata/quota、54个synthetic expected、App/schema、Overview/Settings均未改。修复前后完整实验JSON仅runTimestampUtc和反映源码变更的sourceInputsSHA256不同；全部assessments（含explanation）、分布、迁移、敏感样本与不变量相同。

工程观察（非benchmark）：Release产品EXE由37,334,460变为2,284,345 bytes，产品资源obj由35,294,849变为225,057 bytes。旧48.md full-audit CLI为8.27s/约742MiB工作集峰值；49.md 修复验证的 Full Audit Release 重放为4.75s。Release ValidationPage两文件加载、实验至UI可用的定向计时为206ms；这与旧完整GUI test约6.22s不是严格等工作量基准。独立Release Phase11.ui进程观测为0.30s、工作集峰值约51.2MiB（测试宿主仍包含full resources但此case不加载它们，不冒充正式产品进程内存测量）。

直接证据在ignored `build-debug/phase11-review-fixes/`：verification-summary.json、phase11-review-fixes.patch、fixes-report.md、fixture前后哈希、注册目录、build/CTest/replay日志及性能观察。源码修复仅涉及CMake、ValidationDataset、ValidationSelection、ValidationPage与Phase11Test；工具/生产规则保持不变，另更新本文。

## 16.3 Phase 12 Presentation & Explainability MVP（52.md）

**职责边界：Assessment 决定结论，Profile 提供 evidence facts，Presentation 负责表达。** 本阶段没有新风险算法、Score、schema、provider、批量扫描、业务报告、Dashboard 或 Graphviz 业务功能；没有新的 QSettings key 或业务持久化。

- **Current Project 单一权威：** ProjectPage 的实际列表选择是唯一 UI 权威；`currentProjectId()` 读取该选择。`currentProjectChanged` 只在身份变化时发出，成功 Apply 后发出 `currentProjectStateChanged`；通知用的 last ID 仅抑制重复通知，不供其他模块作为选择来源。main 组合页面，OverviewPage::follow 连接上述信号后显式读取当前 accessor，同步连接建立前已发生的初始 reload；Overview 不自选首个 / 最新项目、不另存 current ID、不改 AppSettings。
- **窄聚合摘要：** ComponentRepository::readOverviewSummary 使用一条参数化 SELECT，以 projects 行校验项目存在，相关子查询 `COUNT(*) WHERE project_id=p.id AND source_role=Component(1)` 排除 metadata root，并用 `EXISTS(dependency_capture WHERE project_id=p.id)` 表达捕获状态。没有组件逐行物化、完整 DependencySnapshot 或图重建；项目不存在 / 数据库错误单独返回，Overview 显示“读取失败”，不伪装成 0。Captured Empty 与 Not Captured 仍可区分。
- **Overview 四区：** 项目名称 / 描述 / 本地创建时间；普通组件数 / 依赖捕获状态；OSV、PyPI/npm、选定组件按需分析、本地适用性、FIRST EPSS / CISA KEV 能力；Phase 11 已完成的方法状态及其限制入口。明确“当前系统不提供项目级批量漏洞统计”。管理项目、进入项目分析、查看方法验证三按钮只导航，不导入、不分析、不查询 provider。
- **共享元数据：** RiskPriority.h 的只读 `ProductionRulesVersion=1`、`ProductionEpssPercentileThreshold=0.90` 被生产 evaluator、默认 options / assessment、原技术解释及设置页实际复用；没有平行可编辑配置。PackageIdentity::supportedEcosystems 与 resolver 共用 PyPI/npm 名称来源。App 版本来自 CMake，schema 来自 AppDatabase::SchemaVersion。
- **PresentationCore：** PresentationText 是小型无副作用标签 / 文本片段 API，不是整份报告生成器。对 Priority、Support、Driver、reason、acquisition、provider status、freshness、severity、dependency、quality 逐枚举显式映射中文与可追溯英文；无 default 原始枚举兜底。GCC switch / switch-enum 警告按错误处理（MSVC 对应 4061 / 4062），测试核对映射，使新增枚举值不能静默遗漏。Core 不依赖 Widgets / Sql / Network，无 clock、evaluator 调用或 CVE normalization。
- **Freshness 两层：** 主视图当前决策新鲜度只取 Assessment 的 driverFreshness / evidenceFreshness；Profile.freshness 仅在技术详情明确标记“Profile 生成时新鲜度”。获取来源与新鲜度分开：StaleFallback 固定为“在线刷新失败后使用缓存 (Stale Fallback)”，不称“过期缓存回退”。缓存未满 24h 的失败回退仍遵守已有 Stale 规则，future 为 Ineligible；呈现层不重新推算。
- **CVE join 与缺口：** DecisionFreshnessLookup 按 exact canonical CVE 建索引、分别读取同一行的 epss / kev，不依赖数组位置；合法 CVE 缺行或重复关联显示“关联信息不可用（呈现一致性异常）”，绝不回退到 Profile.freshness。复用既有 validCveId 判定；空 / invalid CVE 且 provider 为 NotQueryable 时显示“无可关联项”，不把正常 No-CVE 当成契约缺行，也不产生第二套身份规范化。KEV NotQueryable 固定为“无可用于 KEV 目录匹配的合法 CVE (NotQueryable)”，解释为在已获取目录中无法本地匹配。
- **六层结果呈现：** 共享 RiskPresentationView 表达结论、为什么、驱动、关键证据、结论边界；页面保留第六层技术详情及原 priorityExplanation / profileText、code 和 provenance。主视图拒绝 Profile / Assessment key 或生成时刻不一致的组合。所有动态内容 HTML 转义，链接不打开；Overview 动态 QLabel 使用 PlainText。多 CVE 所有成功、失败、缺口行完整保留，展示 Partial 原因，不制造 coverage score。Severity / Dependency / Quality 仅 context，明确 Support ≠ confidence、percentile ≠ probability、Below ≠ safe、NotListed ≠ 从未利用、path ≠ runtime reachability。
- **页面与生命周期：** VulnerabilityPage 保留选组件、候选、Applicability、Finding、加载 / 刷新 / CacheOnly / 取消及原 controller 连接；订阅既有 changed 更新或清空呈现，不引入网络请求或 timer。ValidationPage 共用同一呈现组件，样本和实验阈值切换读取原实验结果，旧技术详情仍可查看。设置页仅增加只读 app/schema/rules/生产阈值，侧栏仍为“Phase 12 · 结果呈现”（现存界面文案，不代表当前阶段仍为 Phase 12）。Overview、设置及漏洞页面保持小窗口滚动路径；测试覆盖反复切换、销毁和转义。

57.md 最小修复只扩展现有 boundaryText：通用说明完整公开 KEV catalog 的获取与本地匹配，明确不逐 CVE 向 CISA 查询；Below 只表示未达研究百分位阈值，所有 Priority 结果均说明 Below ≠ Safe / Low Risk / Not Exploitable。现有四个 Phase12 测试扩展断言，未改算法、provider 或 fixture。

## 16.4 Phase 12 首次审查修复与历史测试兼容（54.md / 55.md）

**First ChatGPT Source-level Code Review：CHANGES REQUIRED；Blocker=0、Must Fix=3、Should Fix=2、Confirmed Functional FAIL=0、Scope Creep=0。** 下列五项 fixes implemented，历史首次审查不改写为 PASS。58.md 确认 Second ChatGPT Source-level Code Review：PASS；Blocker=0、Must Fix=0、Should Fix=0、Confirmed Functional FAIL=0、Frozen Design Deviation=0、Scope Creep=0。

1. **Must Fix — Stale wording：** Effective Stale 标签改为“当前决策证据为 Stale 上下文 (Stale)”，解释“不作为 Fresh 决策驱动；可能因为超过刷新窗口，也可能因为在线刷新失败后使用缓存”。Profile snapshot Stale 为“Profile 生成时新鲜度：Stale（当时未满足 Fresh 条件）”。StaleKevContextOnly 等呈现文案不再推断必为超时；真实 evaluator 的 1h EPSS / 2h KEV StaleFallback 测试确认 effective Stale，完整主视图不含“已过刷新窗口”。未修改 domain enums 或 evaluator。
2. **Must Fix — Key Evidence Fetched At：** EPSS / KEV 每行显示已有 fetchedAt，格式为 UTC + Qt::ISODateWithMs，invalid 显示“不可用 (Unavailable)”。不生成当前时间、不改变原 timestamp；测试用确定的不同时间、+08:00 输入及 invalid 两行验证完整主视图。
3. **Must Fix — Technical Details 隔离：** VulnerabilityPage 的 riskViews 为三个独立页签：“结果解释”（仅 RiskPresentationView）、“技术详情 / Priority”（原 riskPriorityAssessment）、“技术详情 / Provider Snapshot”（原 riskEvidence）。默认人类解释与技术详情隔离，原 priorityExplanation / profileText 未修改，完整文本和切换可达性由 Phase12.vulnerabilityPresentation 验证。ValidationPage 保持已有两页签结构。
4. **Should Fix — 版本测试：** Phase12Test::settingsInfo 读取 APPLICATION_VERSION，不硬编码 0.13.0；该次修复时 App 版本为 0.13.0；当前已随 Phase 13 升至 0.14.0。
5. **Should Fix — 显式依赖：** SupplyChainRiskAssessment 直接链接 PresentationWidgets，同时保留 ValidationWidgets，不改变 target 架构。

54.md targeted 26/26 和双配置 Build PASS，但 full CTest 各 247/248；唯一失败为旧 Phase10.ui 在技术 Priority 页签未选中时检查其滚动条。按该次 mandatory gate STOP 停止，没有运行 Replay、修改历史测试或更新本文。此失败记录保留于原目录，不覆盖。

55.md 的独立判定为 **HISTORICAL UI TEST NAVIGATION ASSUMPTION OBSOLETE**，并明确授权极窄兼容修复。实际只在 Phase10.ui 增加六行：查找 riskViews、断言技术文本非默认页、`indexOf(text)` 得到非负索引、切换、处理事件、确认 currentWidget。没有硬编码新页签序号；移除这六行后测试文件与修改前全文一致，原 technical text、Rules v1 / 0.90、EPSS / KEV / reason / provenance、Provider Snapshot、双层滚动、小窗口可达性、截图、Known Exploited、stale、future / Ineligible、selection clear 全部断言保留，无 timeout 变更、跳过或降级。55.md 未修改产品源码、CMake、Phase11 / Phase12 测试或 frozen files。

## 16.5 Phase 13 Current Finding Analysis Report Export MVP

针对 selected confirmed Finding + RiskEvidenceProfile + RiskPriorityAssessment，导出 self-contained UTF-8 HTML。ProjectPage 在保存对话框前同步校验 Current Project、Candidate/Finding 关联及 Risk 操作状态，捕获 immutable FindingReportContext；worker 只消费值副本与目标路径，渲染和 QSaveFile 原子写入，不访问 UI / DB / 网络，不重新计算规则或 freshness。single-flight 覆盖保存框和写入期间；错误消息不泄露内部路径。

导出要求 Risk 操作状态为 `EvidenceOperationState::Complete` 且 Profile / Assessment 存在，request 的 Project ID、request / profile / assessment key 及 profileGeneratedAt 必须一致；这不要求 Decision Evidence Support=Complete，Partial / Insufficient 或 No-CVE 仍可如实导出。失败、取消或 Stale 状态下保留的旧 Risk 结果不能绕过该门控。默认文件名为 `current-finding-report.html`，生产使用原生保存框并保留覆盖确认；取消不写文件。`QSaveFile` 禁用 direct-write fallback，保存失败不直接覆盖旧目标。

报告复用 PresentationText semantics；保留 Profile Generated At、Assessment Evaluated At、Report Snapshot Captured At 三个时间锚点，打开后不自动刷新 freshness。source-based privacy allowlist 排除内部数据库 / cache 路径、内部 ID 与 raw provider dumps；合法 reportable Project Description 不做 token/path 猜测删除，统一 HTML escape。用户输入的合法内容可能包含其自行提供的敏感信息，保存和分享位置由用户选择。

严格只有一个 Finding；不是 project-wide vulnerability report、overall project security assessment、full vulnerability inventory、Risk Score、Risk Level 或 certification。没有 batch analysis、project-wide ranking、Report history、Report persistence、native PDF、Dashboard 或 forced-exit recovery。性能证据仅适用于当前机器。

# 17. 当前自动测试状态

## 64.md Phase 13 Final Seal 自动验证

2026-10-05 的 64.md 封版轮重新执行正式 Debug / Release Build、Full CTest 与 Validation Replay，全部通过；没有修改源码或测试、没有失败重试。

| 命令 | exit | registered / executed / passed / failed | 进程耗时 s |
| --- | ---: | --- | ---: |
| build-debug | 0 | N/A | 0.370 |
| build-release | 0 | N/A | 0.142 |
| ctest-debug | 0 | 271 / 271 / 271 / 0 | 203.103 |
| ctest-release | 0 | 271 / 271 / 271 / 0 | 190.953 |
| replay-debug | 0 | N/A | 5.570 |
| replay-release | 0 | N/A | 4.686 |

Debug / Release 均为 271/271 PASS；Phase 00—12 sealed list 的 248 个 registered names 全保留，新增 Phase13=23，missing=[]、disabled=0。两个 Replay 的 automatedPass=true，与 Phase 12 sealed 及 Phase 13 已验证结果逐个顶层字段（递归包含完整 nested values）比较，scientific-field differences=0；仅允许 applicationVersion、runTimestampUtc、sourceGitCommit、sourceInputsSHA256、sourceWorkingTreeDirty 元数据变化。六个 Phase 11 frozen inputs exact bytes / SHA256 不变。

八个 Source-level Review 文件 8/8 exact match；全部正式 source inputs 指纹在 commit 前再次核对，并与双配置回放的 build fingerprint 一致。App 0.14.0、schema 4；无 reports table、迁移或 schema 5。完整命令、exit、耗时、日志和审计位于 ignored `build-debug/phase13-final-seal/`。该次 Build 为已有构建目录的增量核验（Ninja: no work to do），不是 clean rebuild；CTest 与 Replay 则重新执行。

受测 source input count=95；sourceInputsSHA256=`02eee493780030453b9631aa634c2191a0c6fa4abfa60ea421594418f4be7da9`；构建注入 sourceGitCommit=`50afa083a953f0de97a9e498a7ff82b4b70a2362`、sourceWorkingTreeDirty=true，表示封版前实现的构建来源，不是 Phase 13 completion SHA。Debug / Release 注入指纹与实际源码精确一致；PROJECT-HANDOFF.md 不属于 source inputs。本次仅维护文档，不重新运行 Build / CTest / Replay / GUI，不将上述封版结果称为本次新测试。

Phase 13 的 23 项注册用例均以 `Phase13.` 为前缀：`semantics`、`presentationConsistency`、`multiCve`、`noCve`、`freshness`、`htmlSafety`、`privacy`、`staticTime`、`provenance`、`candidateCapture`、`eligibility`、`retainedRisk`、`ownership`、`projectFailure`、`saveCancel`、`saveSnapshot`、`singleFlight`、`workerLifetime`、`fileSafety`、`noSideEffects`、`resolvedPath`、`scale`、`architecture`。它们验证报告语义、捕获归属、状态门控、保存安全及生命周期等，不替代第 18 节人工验收。

## 58.md Phase 12 历史 Final Seal 自动验证

2026-10-04 的 58.md 封版轮按授权顺序新执行双配置构建、完整 CTest 和离线 Replay，全部通过；没有失败重试，没有改产品源码或测试。

| 命令 | exit | registered / executed / passed / failed | 进程耗时 s |
| --- | ---: | --- | ---: |
| build-debug | 0 | N/A | 1.34 |
| build-release | 0 | N/A | 0.157 |
| ctest-debug | 0 | 248 / 248 / 248 / 0 | 212.079 |
| ctest-release | 0 | 248 / 248 / 248 / 0 | 191.981 |
| replay-debug | 0 | N/A | 6.368 |
| replay-release | 0 | N/A | 4.823 |

Debug / Release 均注册 248 项，历史 222 个 test names 全部存在，disabled=0，failed names=[]。六个 frozen files exact bytes/SHA256 与第 16.1 节完全一致。两次 Replay 相对 57.md 的所有科学字段（含逐样本 assessment / explanation、分布与合成结果）不变，仅 runTimestampUtc 变化。N=40，S1–S6=6/6/6/8/8/6；0.85、0.90、0.95 四类 Priority 分别为 6/22/6/6、6/14/14/6、6/6/22/6；Support=34/0/6；Driver=28/6/6；Sensitive=16；Synthetic=54/0；all invariants PASS。

受测 source input count=92；sourceInputsSHA256=`38eca77447d881016bff3fd131957395f531965a23f747b1235f21f90a2c207f`；sourceGitCommit=`d8a588a4f5cd553bc24d145dd8ad2d6a48aad2f6`，sourceWorkingTreeDirty=true。按现有 CMake exact-byte 算法重算与 Debug / Release 注入指纹均一致；PROJECT-HANDOFF.md 不属于 source inputs。commit 前再次核对源码哈希，确保该轮提交的是受测实现。此 base SHA 不是随后生成的 completion SHA。

完整命令、exit、时间戳、耗时与原始日志保存在 ignored `build-debug/phase12-final-seal/`：verification-summary、registration-audit、frozen-check、replay-comparison、source-fingerprint。未执行 live provider query、Validation Prepare、重新采集、GUI 复验或网络设置变更。正式封版结果以该目录 Phase12-Final-Seal-Report.md + remote main + phase-12-complete peeled commit 为准；本文遵守单 completion commit 约定。

## 55.md 历史导航兼容自动回归

**历史完整自动验证为 55.md（2026-10-03）：历史 UI 导航兼容修复后，全部指定 gates PASS。** 本文在 Debug / Release Phase10.ui、Phase12 targeted、双配置 Build / Full CTest / Replay 全部通过后更新。

| 55.md 检查（按授权顺序） | exit | 实际结果 | 耗时 |
| --- | ---: | --- | ---: |
| Debug Phase10.ui | 0 | 1/1 PASS | 1.57s CTest real |
| Release Phase10.ui | 0 | 1/1 PASS | 0.85s CTest real |
| Debug Phase12 targeted | 0 | 26/26 PASS | 2.58s CTest real |
| Debug Build | 0 | PASS | 21.177s 进程 |
| Release Build | 0 | PASS | 5.024s 进程 |
| Debug Full CTest | 0 | registered/executed/passed=248/248/248；failed=0 | 222.51s CTest real；222.595s 进程 |
| Release Full CTest | 0 | registered/executed/passed=248/248/248；failed=0 | 207.51s CTest real；207.592s 进程 |
| Debug ValidationReplay | 0 | N=40；Synthetic=54/0；all invariants PASS | 5.650s 进程 |
| Release ValidationReplay | 0 | N=40；Synthetic=54/0；all invariants PASS | 4.750s 进程 |

单项执行前分别构建对应 Phase10Test，再依次执行 Debug / Release `ctest --test-dir <build-dir> -R '^Phase10\.ui$' --output-on-failure`；随后 Phase12 targeted、双配置全量构建、`ctest --test-dir <build-dir> --output-on-failure --parallel 2`；仅在两个 full CTest PASS 后执行各配置离线 `ValidationReplay.exe validation <ignored-result-path>`。历史 222 个 test names 全部存在，无 disabled tests，总数仍为各 248。未进行 live OSV / EPSS / KEV、断网或网络设置操作；测试使用 offline / synthetic / fake network / frozen data。

六个 Phase 11 frozen files exact bytes 均与开发前基线一致。两个 Replay 的核心结果一致：N=40；S1—S6=6/6/6/8/8/6；0.85 / 0.90 / 0.95 四类 Priority 分别为 [6,22,6,6] / [6,14,14,6] / [6,6,22,6]；三组 Support（Complete/Partial/Insufficient）均为 34/0/6，Driver（EPSS/KEV/None）均为 28/6/6；Sensitive=16，Synthetic=54/0，all invariants PASS。

Debug / Release 完整 JSON 仅 runTimestampUtc 不同；相对 52.md 结果只有 runTimestampUtc、sourceInputsSHA256 不同；相对 Phase 11 封版结果只有 applicationVersion、runTimestampUtc、sourceGitCommit、sourceInputsSHA256 不同，逐样本 assessment、原技术解释及全部科学字段一致。55.md 历史 sourceInputsSHA256=`56f1a367a07bb7fc5f972a3c76057929291a6c7fd8d00df295c0c51b18cb8b53`，sourceGitCommit=`d8a588a4f5cd553bc24d145dd8ad2d6a48aad2f6`、dirty=true；表示基线之上的实际受测源码，不是 completion commit，本文不属于指纹输入。

55.md 直接证据位于 ignored `build-debug/phase12-ui-compatibility-55/`：before.json（修改前 Git / 正式文件哈希）、navigation-only-check.json、各配置单项 / targeted / build / full / replay 日志、registration-check / frozen-check / replay-comparison / verification-summary、compatibility-report.md 及最终完整 v2 patch。54.md 失败停止证据和 52.md 历史 PASS 继续保留；新结果不覆盖历史失败。计时受并行负载影响，不作性能承诺；自动 Qt 截图不构成人工 GUI 验收。

## 52.md 首次开发自动验收（历史）

**历史 52.md Phase 12 开发验收（2026-10-03）：双配置 Build、CTest 与 frozen Replay 全部 PASS。** 下表为首次开发完成时的结果，55.md 修复后历史结果见上文；均不代表人工 GUI PASS。

| 52.md 历史检查 | exit | 注册 / 执行 / 通过 / 失败 | 耗时 |
| --- | ---: | --- | ---: |
| Debug Build | 0 | 完整构建成功 | 111.646s 进程计时 |
| Release Build | 0 | 完整构建成功 | 121.488s 进程计时 |
| Debug Phase12 targeted CTest | 0 | 26 / 26 / 26 / 0 | 3.63s CTest real |
| Debug Full CTest | 0 | 248 / 248 / 248 / 0 | 221.73s CTest real；221.901s 进程 |
| Release Full CTest | 0 | 248 / 248 / 248 / 0 | 209.01s CTest real；209.189s 进程 |
| Debug frozen Replay | 0 | N=40；Synthetic=54/0；invariants PASS | 5.847s 进程 |
| Release frozen Replay | 0 | N=40；Synthetic=54/0；invariants PASS | 4.903s 进程 |

命令为既有工具链的 `cmake --build build-debug --parallel 4`（Release 对应 build-release）、`ctest --test-dir build-debug --output-on-failure --parallel 2`，targeted 增加 `-R '^Phase12\.'`，Replay 为各配置 `ValidationReplay.exe validation <ignored-result-path>`。PATH 只在子进程补入原 Qt / MinGW 路径。全量注册审计确认历史 222 个名字全部保留、无禁用项；仅 Phase11.exportArtifact 的硬编码 0.12.0 断言改为当前 APPLICATION_VERSION，所有科学结果断言保留。计时受并行负载影响，不作性能承诺。

Phase12 共 26 个注册项（下列每项统一带 `Phase12.` 前缀）：

```text
vocabulary exactThreshold forbiddenSemantics acquisition kevNotQueryable
freshnessDrift statusFreshness cveJoin noCve missingAssociation multiCvePartial
selectionOwnership initialSynchronization projectSwitch createDelete successfulApply
overviewSummary largeSummary summaryError overviewNavigation settingsInfo validationVocabulary
uiLifetime markupSafety structuralContract vulnerabilityPresentation
```

测试使用 QTemporaryDir、synthetic data、隔离路径和 fake network manager；未进行真实在线 OSV / EPSS / KEV 请求，未改网络连接状态、未运行在线准备工具或重采冻结输入。Overview 覆盖未选项目、创建 / 删除 / 切换、成功 Apply、metadata root 排除、Captured Empty、错误不冒充零及大摘要；呈现覆盖枚举穷尽、0.90 等号、source/freshness 独立、No-CVE、乱序 join、关联缺行、Multi-CVE Partial、两页面一致词汇、按钮与清空、对象生命周期、HTML 转义及小窗口自动截图。

Phase 11 六文件 SHA256 均与开发前基线一致；新 Replay 与 51.md 封版 artifact 的全部科学字段（含逐样本 assessment、原解释字符串、排序、分布、敏感性与 synthetic 结果）全等。实际变化字段仅 applicationVersion、runTimestampUtc、sourceGitCommit、sourceInputsSHA256；sourceWorkingTreeDirty 同为 true。Debug / Release 新结果仅 runTimestampUtc 不同。N=40，阈值 0.85 / 0.90 / 0.95 的四类分布分别为 [6,22,6,6] / [6,14,14,6] / [6,6,22,6]，Sensitive=16、Synthetic=54/0。

52.md 历史受测 sourceInputsSHA256=`46a531f1029f0d574f1aa6046650d4b216d5e7f9d56d948ae7722ff59ea376b6`；build-injected sourceGitCommit=`d8a588a4f5cd553bc24d145dd8ad2d6a48aad2f6`、dirty=true，表示该基线之上的当时未提交源码，并非新 completion commit。本文不属于源码指纹输入。52.md 直接证据在 ignored `build-debug/phase12-development/` 的 baseline / registration / frozen / replay comparison JSON、双配置构建 / CTest / Replay 日志；自动 UI 截图仅作布局检查证据。

## Phase 11 封版的历史自动验收

**历史 51.md Final Seal（2026-10-03）：Debug / Release build、全量 CTest 与 frozen Replay 全部 PASS；各注册 / 执行 / 通过 222 项，失败 0，无失败重试。** 下表仅记录该次执行；最新 Phase 13 封版验证及 Phase 12 历史结果见上文，不覆盖或复用历史日志冒充新结果。

| 最终检查 | exit | 实际结果 | 进程耗时 | 完成时间 UTC |
| --- | ---: | --- | ---: | --- |
| Debug Build | 0 | 增量构建成功 | 1.550s | 2026-10-03T06:44:17.322808+00:00 |
| Release Build | 0 | 增量构建成功 | 1.561s | 2026-10-03T06:44:17.333474+00:00 |
| Debug Full CTest | 0 | registered/executed/passed=222/222/222；failed=0 | 228.799s | 2026-10-03T06:48:07.187870+00:00 |
| Release Full CTest | 0 | registered/executed/passed=222/222/222；failed=0 | 207.773s | 2026-10-03T06:47:46.162112+00:00 |
| Debug Replay | 0 | N=40；Synthetic=54/0；invariants PASS | 6.624s | 2026-10-03T06:48:13.814274+00:00 |
| Release Replay | 0 | N=40；Synthetic=54/0；invariants PASS | 4.877s | 2026-10-03T06:48:12.067304+00:00 |

CTest 内部 real time：Debug 228.71s，Release 207.69s。计时受并行负载影响，不作性能承诺。

51.md 对 `build-debug` 和 `build-release` 分别执行 `cmake --build <build-dir> --parallel 4`、`ctest --test-dir <build-dir> -N`、`ctest --test-dir <build-dir> --output-on-failure --parallel 2`，以及各配置 `ValidationReplay.exe validation <ignored-result-path>`；尖括号参数须替换为实际目录。PATH 仅在子进程补入既有 Qt / MinGW / CMake 工具目录；Replay 只读 frozen validation directory，不访问 live providers。

原 Phase00—10 的 182 项全部保留，Phase11 为 40 项。49.md 新增的 11 个 targeted 注册项为 captureAttempts、failureAuditConsistency、failedCandidateMetadata、successCandidateMetadata、productionVersionValidity、frozenIdentityCompatibility、coverageN29、coverageMissingS4、coverageMissingS5、runtimeLoader、runtimeContract；54 个 synthetic 与 40 个真实样本不算额外 CTest 注册项。

Debug / Release Replay 除 runTimestampUtc 外完整结果全等：N=40，S1—S6=6/6/6/8/8/6；三阈值分布见第 16.1 节；Sensitive=16、Synthetic=54/0、全部 invariants PASS。sourceInputsSHA256=`2999c7e89ae2bdd120943edca929a58926beebc57dccc6cbbbf5c47af24d88e1`，按 CMake 的 85 个 source inputs exact-byte 算法重新计算一致；本文不属于该指纹输入。build-injected sourceGitCommit 为封版前基线 b2af74d813828b7cc120b00e953c69923b21c831、dirty=true，不把它冒称 completion commit。

51.md 直接证据在 ignored `build-debug/phase11-final-seal-51/`：pre-seal.json、final-verification.json、build/discovery/ctest/replay 日志、Replay JSON 与 replay-comparison.json、maintenance-verification.json。Manifest=79,776,141 bytes、Index=76,937,806 bytes、Dataset=1,663,896 bytes；三份 JSON SHA 与三个 sidecar 均匹配第 16.1 节，封版核验过程中仅本文内容变动。

历史来源保留：49.md 修复后双配置各 222/222；50.md 新执行双配置各 222/222（CTest 217.06s / 204.77s），GUI 证据与后来用户验收闭环见第 18 节。旧 Debug LastTestsFailed.log 时间为 2026-09-30T09:16:05.071586Z，早于 51.md 最终回归，未作为当前失败或新执行证据。

历史 47.md 首次实现最终 Debug/Release 各 211/211（153.01s/142.88s）；早期 Debug 曾 209/211，applicationStartup 因提前加载 audit 退出超时、orderingContract 因 QJsonValueRef 交换误构造重复值，均在 47.md 修正。旧资源数组构建及一次 Release 测试曾主动中止；既有日志保留，不覆盖或伪装为本轮日志。

## Phase 10 与环境对齐的历史自动验收

**Phase 10 历史全量验证（2026-09-28）：Original Phase 00—09 regression 158/158 PASS，Phase 10 Automated Acceptance 24/24 PASS；Debug / Release 均 182/182 PASS，0 FAIL。**

2026-09-30 Qt Creator 更新后，Debug / Release 增量 Build 均 PASS（no work to do）；两种配置各运行 Phase00 基础冒烟 3/3 PASS，覆盖 Qt 6.11.2 / C++20 / Widgets、SQLite / QSQLITE 和 Graphviz SVG。该次 IDE 对齐没有重跑 182 项全量回归，也没有新增人工 GUI 验收结论；后续 Phase 11 完整回归见本节开头。核验日志留在 ignored `build-debug/qtcreator-update-check-20260930/`；下表继续保留 Phase 10 封版时的全量结果。

下表来自 **2026-09-28 的 46.md 最终封版验证**，不是复用 43.md—45.md 旧日志。Debug 09:11:17—09:12:44、Release 09:12:44—09:13:53（Asia/Shanghai）；两种增量构建均为 no work to do。验证后仅修改交接文档，生产源码、测试及构建配置以 SHA-256 对照封版起点保持不变；不将其表述为提交后再次测试。GUI 验收来源单列第 18 节。

| 正式工程验证 | 结果 |
| --- | --- |
| Debug Build | PASS；no work to do |
| Release Build | PASS；no work to do |
| Debug CTest | 182/182 PASS；85.94 s |
| Release CTest | 182/182 PASS；69.27 s |
| Phase 00 Regression：Qt / C++20、SQLite、Graphviz SVG | Debug / Release 均 3/3 PASS |
| Phase 01：路径、配置、数据库、未知 schema、日志、导航、真实程序启动关闭 | Debug / Release 均 7/7 PASS |
| Phase 02：schema 初始化 / 迁移 / 回滚 / 冲突保护、项目读写 / 校验 / 排序 / 持久化、UI 和错误处理 | Debug / Release 均 12/12 PASS |
| Phase 03：版本 / 元信息 / 嵌套组件 / 依赖、错误 / UTF-8 / 安全边界 / 文件读取、预览原子性 / Project 集成 / 大预览 / 异步关闭 | Debug / Release 均 15/15 PASS |
| Phase 04：clean / 字段缺失 / 重复标识 / 空文档 / 依赖规则 / 确定性与计数 / 文档不变 / Parse 与 Quality 分离 / UI / 大输入 / 数据库不变 | Debug / Release 均 11/11 PASS |
| Phase 05：schema / 迁移 / 回滚 / FK / 原值与顺序 / replacement / 错误 / Preview / Apply / Parser Failure / 重试 / 项目 UI / 大替换 / 大 Apply / worker 生命周期 | Debug / Release 均 16/16 PASS |
| Phase 06：schema 4 / 全迁移链与回滚 / raw 保存 / capture / 原子 Apply / 一致 snapshot / exact resolution / metrics / deterministic graph / self-loop / BFS / UI / graph reuse / stale result / 大规模 / worker 生命周期 | Debug / Release 均 18/18 PASS |
| Phase 07：identity / version / parser / merge / network errors / pagination / limits / cancel / privacy / cache / controller / lifecycle / hidden page / UI / Apply integration / 100000 components / fixtures | Debug / Release 均 26/26 PASS |
| Phase 08：SemVer / exact versions / ranges / limits / wildcard / provider gate / withdrawn / findings / cache-live / lifecycle / cancellation / destruction / Apply / large snapshot / UI / fixtures | Debug / Release 均 20/20 PASS |
| Phase 09：ownership / severity / EPSS / NotScored cache / KEV / dependency / lifecycle / UI / scale / Candidate lookup scale / KEV CVE boundaries / transient rename / permanent denial | Debug / Release 均 30/30 PASS |
| Phase 10：rules / provenance / freshness / context / timer / lifecycle / UI / 10000 CVE / fixtures | Debug / Release 均 24/24 PASS |
| Windows cache reliability：transient rename denial recovery / permanent denial preserves old cache | 46.md 封版时 Debug / Release 均 PASS |
| Windows cache reliability stress（41.md 正式修复验证） | Debug 200/200 PASS、0 FAIL、14.35 s；Release 200/200 PASS、0 FAIL、12.78 s |
| GUI smoke | PASS；真实程序隔离启动和关闭、项目与导航回归、只读组件与质量预览、正常 / 最小尺寸 / 超长字段、失败保留旧结果、10000 组件预览 / 500000 问题行及异步关闭检查通过 |
| 最终维护性检查、git diff --check | PASS |

Phase 10 封版时测试源码为 tests/Phase00SmokeTest.cpp 至 tests/Phase10Test.cpp；当前已增加 tests/Phase11Test.cpp、tests/Phase12Test.cpp 与 tests/Phase13Test.cpp。原 Phase 00—08 的 128 项注册用例保持，Phase 09 新增 30 项（含两项 Windows-specific atomic replacement 测试），加上 Phase 10 的 24 项，Windows 总计 182。原 epssCache 保留 Fresh/Stale × Available/NotScored 四种同路径连续覆盖，不以测试重试掩盖生产错误。Win32 no-delete-sharing handle 制造真实拒绝，目录通知确认失败临时文件清理后释放句柄，验证生产重试；持续占用覆盖三次失败、CacheIo、旧字节及合法快照不变，释放后独立正常写入成功。KEV 增加完整 catalog / entries / fetchedAt 回读断言。非 Windows 配置不注册这两项 Windows 专用测试：Phase 10 / 11 历史静态数量为 180 / 220。最新各 271/271 与历史各 248/248、222/222、182/182 的实测结论均来自 Windows，不代表 Linux / macOS 已构建或验收。

Phase07 使用 FakeNetwork / ControlledReply 离线覆盖错误、分页、token-only、重复 token、跨页去重、限额、取消及最小发送数据。缓存覆盖过期 / 未来时间戳、完整零结果、坏格式 / 键、原子写和清除边界；controller 覆盖状态分离、刷新失败保留历史、stale result、隐藏页切换与销毁。测试内以确定性线程池控制复现 read / parse / write 未结束时 cancel / clear 的边界，不给生产加入延迟或钩子。项目集成用真实 Preview / Apply 和 SQLite trigger 注入失败，验证回滚保留、重试成功及结果失效。100000 Components 表格与响应验证 HTTP=0；manualFixtures 仅生成公开包 / synthetic 文件于 ignored build 目录。普通 CTest 不访问公网；`liveSmoke` 不注册 CTest，且须显式按名字调用，历史 live 结果见第 11 节。

Phase08 使用纯值规则测试及离线 Network / Reply 验证 cache / live 路径、取消、切换、重读、销毁、成功 / 失败 Apply 与大规模后台派生。Phase09 覆盖 10000 Candidate/Finding 选取、1200 CVE / 5000 KEV / 4000 dependency nodes、取消和 stale replies；fixture 均为构造数据，运行时产物仅写 ignored build 或临时目录。Phase07 隐私断言按既有授权修正 timestamp prefix 与 payload 分离检查，原敏感值断言保留，避免时间戳中的 1.0 被误判为泄露版本。

文件选择自动测试仅在测试进程启用 AA_DontUseNativeDialogs；既有用例通过作用域恢复，Phase07 / Phase08 / Phase09 / Phase13 在独立测试进程初始化时设置。Phase 03 开发时自动快速开关原生框曾在 Qt Windows 平台线程出现崩溃，因此自动测试覆盖的是 Qt 控件文件框与导入入口连接，不覆盖原生框实现。生产程序仍使用原生框，其打开、取消、重新打开已由用户在 `15.md`、`17.md`、`19.md` 及 `21.md` 人工确认 PASS；尚不能据此声称所有原生框异步关闭场景均经自动验证。未用 QTimer / sleep 延迟补丁替代生命周期处理。

46.md 封版时 Debug / Release full CTest 均 0 FAIL。维护性审查覆盖纯 Core evaluator、单一 Finding owner、Profile / Assessment 原子发布、timer generation guard、确定性 provenance、UI 不重复评估、复杂度和 scope；无新表或 persistence。十个正式候选文件经 diff 与隐私审查，不纳入 runtime provider payload、真实 SBOM、数据库、缓存、日志、截图或构建产物。

Phase 10 自动测试覆盖 exact 24h boundary、未来时间戳 Ineligible、stale exclusion、KEV precedence、最大 EPSS / tie-break、Partial / NotScored、context isolation、queued timer、refresh / cancel overlap、late callback 的实际 UTC、失效与销毁、UI renderer 和规模。未来时间戳、精确边界与竞态覆盖属于 AUTOMATED ONLY，不记录为人工 GUI PASS。

Phase 10 最终封版日志：`build-debug/phase10-final-seal-20260928-091102/`，含 Debug / Release build / full CTest 与验证元数据。Phase 09 历史封版日志为 `build-debug/phase42-final-seal-20260927-161208/`，缓存压力日志为 `build-debug/phase41-cache-reliability-20260927-160435/`；200/200 stress 是历史 41.md 结果，46.md、51.md 封版以及 52.md、58.md、64.md 和本次文档维护均未重跑该压力循环。上述目录均 ignored，不要求克隆后存在；历史 PASS 不证明后续修改通过。

回归命令：`ctest --test-dir build-debug --output-on-failure` 和 `ctest --test-dir build-release --output-on-failure`，使用冻结 CMake 目录中的 ctest.exe。CTest 为子进程设置 Qt / MinGW DLL 搜索路径，不修改系统 PATH。

已有构建目录的 PowerShell 操作（在正式根目录执行，PATH 仅作用于当前进程）：

```powershell
$env:PATH = 'D:\program\Qt\6.11.2\mingw_64\bin;D:\program\Qt\Tools\mingw1310_64\bin;' + $env:PATH
& 'D:\program\Qt\Tools\CMake_64\bin\cmake.exe' --build build-debug --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
& 'D:\program\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-debug --output-on-failure --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Debug tests failed' }
& 'D:\program\Qt\Tools\CMake_64\bin\cmake.exe' --build build-release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
& 'D:\program\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-release --output-on-failure --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Release tests failed' }
```

构建目录不存在时，先使用冻结 cmake.exe 执行配置：`-S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=D:/program/Qt/Tools/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/program/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_PREFIX_PATH=D:/program/Qt/6.11.2/mingw_64 -DBUILD_TESTING=ON`；Release 对应改为 `build-release` / `Release`，检查成功后再构建。已有缓存的生成器或工具链不符时先核对原因，不直接覆盖。GUI 用例需要可用的 Windows 桌面会话，不能把无显示环境中的结果等同于人工验收。

# 18. 当前人工验收状态

## Phase 13 — GUI / REPORT MANUAL ACCEPTANCE PASS

64.md 的最终直接用户确认：Total=19、PASS=19、Confirmed Functional FAIL=0、BLOCKED=0、NOT VERIFIED=0。

| 来源 | Test | 结果 |
| --- | --- | --- |
| User-operated manual verification | 12, 13, 15, 16, 17, 18 | PASS |
| Codex GPT-6 Astra Ultra operated / verified | 1–11, 14, 19 | PASS |

最终 provenance 以 64.md 直接澄清为准，覆盖历史材料对 Test 18 的旧归属；不写 User 19/19 或 Codex 19/19。历史 execution evidence 保留，不以自动测试替代用户操作。Test 18 确认 synthetic Project Description 的 Windows path-style text 和 token-like text 保留，`<b>` / `<script>` 为 literal text，脚本未执行，HTML 其余结构正常；这里只记录 synthetic 验收事实，不记录真实 secret。

Second-party / ChatGPT Source-level Code Review：PASS；Blocker=0、Must Fix=0、Frozen Design Deviation=0、Scope Creep=0、Confirmed Functional Defect=0。该独立结论由 64.md 提供，Codex 本轮 fingerprint / scope audit 不替代 ChatGPT Review。

## Phase 12 — PASS WITH NON-BLOCKING UNVERIFIED ITEMS

58.md 提供最终验收收口及执行来源：Total=27；PASS=26；Confirmed FAIL=0；BLOCKED=0；NOT VERIFIED=1。Test 1–19、21–27 PASS；Test 20 为 NOT VERIFIED / NON-BLOCKING。57.md 已补齐公开完整 KEV 目录获取 / 本地 CVE 匹配 / 不逐 CVE 向 CISA 查询的说明，以及所有 Priority 结果的通用 Below 边界；本轮不重做 GUI 验收。

| Execution provenance | Tests | 最终结果 |
| --- | --- | --- |
| User-operated manual verification | 16、19 | PASS |
| User-operated manual verification | 20 | NOT VERIFIED / NON-BLOCKING |
| Codex GPT-6 Astra Ultra operated / verified | 1–15、17–18、21–27 | PASS |

Test 20：用户本人检查现有 Phase 10 A–G synthetic fixtures 后，Dependency Root 均无法提供正向路径前置条件（实际 Root Missing）。Reason: existing A–G synthetic fixtures do not provide a valid positive resolved dependency-path manual scenario。已验证 **Dependency Path ≠ Runtime Reachability**；未验证 **positive ResolvedPathFound GUI scenario**。这不是产品功能 FAIL、不是 BLOCKED，也不阻塞 Final Seal；不得记录为 PASS。没有为此修改 fixture、SBOM、database、dependency graph、产品代码或测试环境。

Second ChatGPT Source-level Code Review：PASS；Blocker=0、Must Fix=0、Should Fix=0、Confirmed Functional FAIL=0、Frozen Design Deviation=0、Scope Creep=0。First Review 的 CHANGES REQUIRED 与五项 fixes implemented 历史保留在第 16.4 节。不得写“用户完成全部 27 项”“Codex 完成全部 27 项”或“27/27 PASS”。Phase 11 的用户 Test 17–19 归属仅适用于下方历史阶段。

## Phase 11 — Acceptance Closure PASS

**PHASE 11 GUI / ACCEPTANCE TESTS — PASS：Test 1–20 共 20/20 PASS；Confirmed FAIL=0、BLOCKED=0、NOT VERIFIED=0。** Execution provenance is mixed，不将所有项目归为用户操作，也不将用户操作归为 Codex。

| 执行来源 | 测试范围 | 最终结果 / 来源 |
| --- | --- | --- |
| Codex GPT-6 Astra Ultra operated / verified | Test 1–16、Test 20 | PASS；50.md 的实际 GUI 捕获与逐项报告 |
| User-operated manual verification | Test 17 — 离线验证 | PASS；用户于 2026-10-03 补充确认由本人实际执行且通过（确认日期，不推定实际执行时间） |
| User-operated manual verification | Test 18 — 加载过程中切页 | PASS；51.md 用户提供的完成记录：无崩溃、白屏或异常状态 |
| User-operated manual verification | Test 19 — 加载过程中关闭程序 | PASS；51.md 用户提供的完成记录：正常退出、后续可正常启动 |

51.md 将 Test 17 归属 Codex 的文字已由用户于 2026-10-03 纠正；最终记录以该直接澄清为准。Do NOT classify all 20 tests as user-operated. 不写“USER-CONFIRMED 20/20”，也不写“Codex 执行 Test 17”。

50.md 原始报告保留当时的真实状态：Test 17 USER-RESERVED，Test 18/19 NOT VERIFIED；51.md 及用户补充确认使三项闭环，不回改历史测试记录。Codex GUI 证据位于 ignored `build-debug/phase11-test-execution-50/`。51.md Final Seal 沿用上述验收证据，未重做 GUI、改变网络状态或重新采集 provider 数据；后续文档维护和 52.md 当轮也未新增 Phase 11 人工 GUI 验收结论。

第二次 ChatGPT 独立 Source-level Code Review PASS 与零遗留项见第 16.2 节。人工入口仍为 ignored `build-debug/phase11-manual.cmd`，使用独立 `phase11-manual-data`；既有 Phase00—10 验收事实继续保留如下。

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

- **Phase 05 Manual Acceptance：PASS**，用户在 `19.md` 亲自操作并确认全部 12 项通过；Debug GUI：**PASS**，Release GUI：**AUTOMATED ONLY**，不是 Codex 后续封版重新操作的结果。
- Preview 不持久化、Explicit Apply、重启保持、Project 隔离、完整 Replacement 而非 Merge、Parser Failure 保留旧 persisted Components、Quality Error 明确提示且可 Apply、防双击 / 重复 Apply、Empty Replace、Project Delete Cascade 均 PASS。
- 100000 Components GUI responsiveness PASS；用户实际观察 Apply 约停留 1 秒左右完成，无明显不可接受卡死。这是当前机器的一次人工验收证据，不是长期性能保证。
- Phase 00—04 GUI regression PASS；未发现崩溃、SQLite、DLL / Qt platform plugin 错误或其他阻塞问题。

**Phase 06 Manual Acceptance：PASS。Phase 06 Debug GUI Manual Acceptance: PASS。**

用户在 `21.md` 明确确认亲自完成全部 26 项 Debug GUI 人工验收；以下为用户实际 GUI 人工验收证据，不是 Codex 后续封版重新操作：

- old schema 3 Project → Not Captured、Preview 不持久化、Explicit Apply、restart persistence、Captured Empty 与未捕获区分均 PASS。
- Raw / Derived metrics、Direct Dependencies / Dependents、Transitive Dependencies / Dependents、shortest depth、root Component resolution 均 PASS。
- Missing / Unknown / Ambiguous、DuplicateBomRef ambiguity、Raw duplicate 保留与 Derived unique 去重均 PASS。
- Self-Dependency Occurrences、direct self-loop 可显示自身、transitive 不返回起点、cycle safety 均 PASS。
- Quality Error 仍可 Apply、Parser Failure 保留完整 Current State、second Apply 原子替换 Components + Dependencies + Capture State 均 PASS。
- Project isolation、Project delete cascade、Phase 00—05 GUI regression 均 PASS。
- 100000 Components + 100000 Entries + 100000 Targets 的大规模 Apply / dependency analysis、99999 条 transitive result 与 large table 使用均 PASS；用户观察响应很快，无明显卡顿、卡死或 Windows“未响应”。仅为当前机器验收证据，不是长期性能保证。
- stale-result protection PASS；快速 Project A / B 切换及连续 Reload 没有旧 Project graph 串入新 Project。
- 无 crash / SQLite / DLL / Qt platform plugin 或其他 blocker。

**Phase 06 Release GUI：AUTOMATED ONLY。** Phase 06 封版轮按 `21.md` 授权仅补做 Debug / Release 自动回归，未增加 Release GUI 人工验收。

**Phase 07 Manual Acceptance：PASS。Debug GUI：18/18 PASS。** 用户在 `26.md` 明确确认“全部测试通过”，亲自完成完整 18 项验收；以下记录来自用户实际操作，不是 Codex 模拟，也不是自动测试替代。后续封版未重做这 18 项。

- 漏洞匹配页面、Preview / Explicit Apply 分离、Current Components 正式来源；PyPI、npm、scoped npm；MissingPurl / MissingVersion / UnsupportedEcosystem / VersionConflict；epoch / local version 原值保留均 PASS。
- 首次实际在线发送确认、Candidate Found、零 Candidate 正确措辞、npm 在线查询均 PASS；实时 OSV 候选数量按当时实际结果验收通过，不写成永久保证。
- Fresh Cache、重启复用、Cancel / Working 防重入、网络失败与历史缓存分离、Stale Cache、CacheInvalid、显式清除边界均 PASS。
- Preview / Apply / Project switch / Reload 失效边界、100000 Components GUI responsiveness、无自动批量网络查询均 PASS。
- 无 crash / SQLite / TLS / DLL / Qt plugin blocker。

**Phase 07 Release GUI：AUTOMATED ONLY。** 当前仅有 Release Build / CTest 自动验收，没有新增用户 Release GUI 人工验收结论。

**Phase 08 Manual Acceptance：PASS。** 用户在 34.md 确认亲自完成 Debug GUI 全部人工验收；这是用户真实操作结果，不是 Codex 模拟或自动测试替代。

- synthetic demo：8 Candidates / 4 Affected / 3 Unknown / 1 Excluded / 4 Findings。
- ExplicitVersionMatch、SEMVER、fixed / last_affected 的 2.0.0 / 2.0.1 边界、ECOSYSTEM / GIT Unknown、ProviderEvidenceConflict、wildcard、Withdrawn / Excluded 均通过。
- Fresh / Stale、重复查询不重复 Finding、component / project switch、reload / cancel、Empty SBOM Preview / Apply invalidation、Demo SBOM 恢复通过。
- 正常 / 小窗口、evidence / Raw JSON 与稳定性通过；无崩溃、明显卡死、SQLite / DLL / Qt platform plugin 异常，无 Safe / 无风险 / 已安全 / 无漏洞错误结论。

**Phase 08 Release GUI：AUTOMATED ONLY。** 未收到用户 Release GUI 人工验收结论。

**Phase 09 Manual Acceptance：PASS WITH NON-BLOCKING UNVERIFIED ITEMS。Confirmed FAIL：0。**

用户 Debug GUI：**PASS WITH NON-BLOCKING UNVERIFIED ITEMS**；Release GUI：**AUTOMATED ONLY**。依据为用户在 39.md / 42.md 的明确确认，不能用自动测试替代或扩大人工验收结论。

用户已确认 Fixture A 完整证据、B MatchingAffected CVSS V3、C No CVE / NotQueryable、D Stale Cache 及离线刷新失败后的旧证据 / StaleFallback、E SchemaConflict；确认框取消、in-flight cancel、Cancelled / ProfileAvailable、Component / Project / Reload 失效、空 SBOM Preview / Apply、Apply 后重新派生、大小窗口与滚动、关闭重启及重新派生均通过。Confirmed crash、异常 Profile 清空、业务失败或 SQLite / DLL / Qt plugin blocker 均为 0；此记录不覆盖下列三项。

以下表格永久保留 **Phase 09 当次验收** 的真实边界。Phase 10 的 Candidate Switch 与自然到期已获得新证据，分别见后表；不回写 Phase 09 当次记录，也不把自动 race 覆盖变成人工 PASS：

| Phase 09 人工验收项 | 当次结论 | 原因 / 覆盖边界 |
| --- | --- | --- |
| Candidate Switch | BLOCKED / NON-BLOCKING | synthetic fixture 无合法第二 Candidate，不得改写成 PASS |
| Stale Reply Race | NOT VERIFIED MANUALLY | 已有自动 lifecycle test，不等于人工 race PASS |
| Natural 24h Fresh → Stale | NOT VERIFIED IN THIS RUN | 自动测试覆盖 Fresh/Stale，未自然等待 24h 验证 |

## Phase 10 GUI Acceptance

**GUI Acceptance：PASS。Confirmed Functional FAIL：0。Blocked：0。Mandatory Not Verified：0。**

**Codex GPT-6 Astra Ultra 在用户授权下执行完整 GUI 验收并保留逐项直接证据；用户接受该验收结果。** 实际执行来自 45.md，用户在 46.md 接受结果；不能表述为用户本人逐项点击。Release GUI 为 AUTOMATED ONLY。

| 验收项 | 结果与直接观察 |
| --- | --- |
| G 初始 Fresh | PASS；Above / Complete，EPSS probability 0.035、percentile 0.91 |
| G 自然 Fresh → Stale | PASS；未点击 Refresh / Cache Only、未切换、Reload 或重启，自然变为 Insufficient / Insufficient、Driver None；Priority 当前 Stale、Provider Snapshot 仍为生成时 Fresh |
| A KEV | PASS；Known Exploited / Complete，CISA KEV driver 与 catalog / date / fetchedAt / acquisition |
| B EPSS Above | PASS；Above / Complete，EPSS provenance、FIRST API Version、Rules v1 / 0.90 / Evaluated At |
| C EPSS Below | PASS；Below / Complete，0.50；明确“不是 Safe、Low Risk 或 Not Exploitable” |
| D Partial | PASS；Above / Partial，第二 CVE NotScored，probability / percentile 不伪装成零 |
| E Stale Context | PASS；Insufficient / Insufficient，历史 KEV Listed 与 EPSS 仅作上下文 |
| F No CVE | PASS；NotQueryable，Insufficient / Insufficient、Driver None |
| Candidate Switch | PASS；旧 Assessment 即时清空 / NotStarted，重新加载产生 SECOND-B 对应结果 |
| Reload | PASS；清空旧状态，没有旧结果回流；重新派生当前 Candidate |
| Restart | PASS；沿用原 B 数据目录且未重新 Prepare，启动 NotStarted，按正常流程重新派生 Above / Complete |
| Small Window / Scroll | PASS；实际 1003×671 窗口下内外滚动可用，随后恢复 1707×1019；不把自动 680×420 尺寸写成人工实测 |
| Context Semantics | PASS；CVSS 原始 vector、severity context-only、dependency 非 runtime reachability、Quality unavailable 文案正确 |

G 的 fetchedAt 为 2026-09-26T14:42:02.179Z，生产 24h 边界为 2026-09-27T14:42:02.179Z；初始 evaluatedAt 为 14:40:01.064Z，自然到期后的实际 evaluatedAt 为 14:42:02.180Z（同日 UTC），profileGeneratedAt 保持 14:40:01.061Z。首次观察 Stale 为 22:42:02.573（Asia/Shanghai），到期后截图为 22:42:40.637。GUI 未弹出联网 consent；无网络 / 缓存写入的程序行为另由自动 fake transport 与缓存字节 / 时间验证，不宣称 GUI 抓包证明。

C-WORDING-01 的早期字面判定由 46.md 正式纠正为 FALSE FAILURE / TEST INTERPRETATION TOO LITERAL；否定式安全提醒符合设计，C 为 PASS，未修改代码以迎合该误判。既有直接证据保留于 ignored `build-debug/phase10-acceptance-20260927-223506/`（141 个 UI JSON、56 张 PNG），不提交到 Git。

future timestamp Ineligible、queued timer、refresh / cancel / destruction races 与 exact freshness boundaries 是 AUTOMATED ONLY；这些非必需 GUI 项不会伪称人工 PASS，也不构成 Mandatory Not Verified。

# 19. 当前重要技术决策

- **唯一有效长期方向**为第 9 节 Foundation → Vulnerability → Risk → Validation / Presentation。Phase 12 已封版；Phase 13 Frozen Design、实现、ChatGPT Code Review PASS 与 19/19 GUI / Report 验收已完成；64.md Final Seal 已完成；本次用户仅另行授权文档维护及提交推送，不引入新业务能力。下一阶段须先授权 REQUIREMENTS / DESIGN；旧 Phase 07—17 安排仍属 Historical / Superseded Plan。
- **大方向冻结 + 单 Phase 逐步冻结。** 长期冻结四层方向，未来数据模型、schema、repository、provider、API、service、UI、scoring formula 仅在对应 Phase 开始前正式设计和冻结。这样控制本科毕设复杂度、避免过度设计，根据真实实现结果调整后续方案，防止下一阶段建立在错误假设上，并降低 Codex 长上下文开发的错误假设风险。
- 不得因为路线中未来存在功能，就提前创建对应 database table、domain model、repository、service / provider framework、UI 或评分公式。Phase 11 按 47.md—51.md 完成共享 Rules v1 实验、审查与验收闭环及封版；Phase 12 呈现层只表达已有事实，不扩大生产风险模型。
- **Phase 12 呈现契约：** ProjectPage 保持 Current Project 唯一 UI 权威，Overview 只做 observer，初始连接后显式同步；summary 以 COUNT / EXISTS 读取已存在状态，不隐式分析。Priority / Support / Why / Driver / 当前 freshness 来自 Assessment，provider facts 来自 Profile；No-CVE 正常不可关联与合法 CVE 关联缺行严格区分，exact CVE + provider join，不按数组位置或快照 freshness fallback。只读生产规则元数据和生态列表复用领域来源，详见第 16.3 节。
- Applicability 是 Risk 前置门控；仅 Published Affected 形成 Finding。Local NotAffected 在当前 OSV package-version 查询来源下经 Provider Consistency Gate 转为 Unknown / ProviderEvidenceConflict，Unknown 不能输出确定 Risk。Phase 10 Priority 与 Decision Evidence Support 分离，Support 不等于 predictive confidence；Quality 不直接增加风险，当前 Quality Unavailable 不降低 Support。Rules v1 与 0.90 percentile research threshold 已冻结，但完整风险模型、权重、分数映射不在本阶段范围。
- 工业工具比较与论文声明遵循第 8 节：承认成熟工具的覆盖、成熟度与生态优势，不以替代为目标；将候选工作称为主要工作 / 特色设计，经验证后再作有证据范围的结论，不声称已有创新算法或普遍领先。
- **C++20 + Qt 是软件工程技术选型**，不是因为 C++ 天然比 Python 更适合漏洞分析。Qt 当前统一提供 Desktop GUI、JSON、SQLite、Concurrent 和本地处理，已支撑 SBOM Parse → Persistence → Background Analysis → GUI Presentation；Phase 07 已正式接入 Network 查询 OSV。Python 等生态在安全工具和快速原型方面更成熟。
- 继续冻结现有 C++20、Qt、CMake、SQLite、Graphviz 工具环境；Graphviz 仅已有 Phase 00 冒烟验证，未来业务能力后移到辅助展示 / 解释，不构成下一 blocker；Network 已有实际职责，Svg 仍未链接，不安装无关新工具。
- SBOM 优先 CycloneDX JSON，SPDX 留作可选扩展；匹配优先依赖软件包身份和版本证据。
- GUI、应用逻辑、领域与基础设施可作职责划分，但不强制提前建满架构层。
- 一次只做一个 Phase；每个文件须有实际职责，不创建未来模块、无用接口或 Factory / Adapter / Manager 空架构。
- 稳定代码不无故重构，自动测试不能替代 GUI 验收；用户可亲自执行，或明确授权 Codex 执行并由用户接受逐项直接证据与结果。
- 仅维护本文作为长期动态交接文档，不新增多套状态文件。
- SQLite 每线程独立连接、主线程更新 GUI，密钥不进入代码、日志及 Git。
- MainWindow 仅承担 Application Shell、导航和页面协调；QStackedWidget 是当前页面的唯一状态来源，数据库、日志和配置内部职责独立，启动组合由 main.cpp 完成。
- 运行数据默认使用 QStandardPaths::AppDataLocation；QSettings 使用独立 INI，仅保存轻量 UI preference；测试显式使用临时路径。
- SQLite schema version 当前为 4；app_meta 保存版本，projects 保存项目，components 与三张 dependency 表保存一致 Current State。fresh schema 4 / v1 → v2 → v3 → v4 / v2 → v3 → v4 / v3 → v4 使用统一事务迁移；AppDatabase 在调用线程管理连接，借用的连接句柄和查询须先于连接关闭释放。
- Project 使用 UUID 作为稳定 ID，名称可重名；列表固定按 `created_at DESC, id DESC` 排序，关联和删除均使用 ID。
- ProjectRepository 统一项目校验和 SQL；MainWindow / ProjectPage 不执行 SQL。ProjectPage 负责输入、选择、详情和删除确认，SQLite 是项目数据的持久化来源。
- projects 当前采用真实 DELETE；components、dependency_capture、dependency_entries 及下属 dependency_targets 通过实际启用的 FK ON DELETE CASCADE 清理，不复制手工删除逻辑，不引入 soft-delete。main.cpp 组合已有数据库、日志、Repository 和页面，并在关闭连接前销毁页面和 Repository。
- Phase 03 / 04 的解析文档与质量报告仅 in-memory；Parser 负责读取、JSON、结构和安全限制，独立 Analyzer 只读诊断字段及引用质量。Quality Error 不等于 Parse Failure，Project 模型不保存 sbomPath；Phase 05 显式 Apply 建立 Current Components persistence，Phase 06 扩展为组件、原始依赖及捕获状态的统一持久化，并由数据库一致快照构建派生图。
- Phase 04 使用 issue-based diagnosis、稳定 code / scope / 位置、中文解释及 Info / Warning / Error；暂无总体质量分，不采用任意权重、A/B/C grade 或漏洞风险等级，不负责 auto-fix。缺少 bom-ref 表示关联依据不足，定为 Warning；明确引用冲突或无效引用定为 Error。
- 当前质量 Analyzer 不修改 SbomDocument，使用哈希集合进行重复和 reference checking；非空强标识精确比较，输出按输入顺序确定，不模糊匹配或规范化 PURL，不凭 name / version 合并组件。Phase 07 PackageIdentity 单独派生包身份，不回写或改变上述原始数据语义。
- 后台解析与诊断复用一个值捕获任务；GUI 线程接收完整结果并使用已有 AppLogger 记录安全摘要。只有 parse success 且 quality analysis 正常返回后才替换成功预览与报告；取消和 Parser Error 保留旧状态，窗口生命周期决定两者生命周期。质量报告不持久化、不写数据库。
- Project → Current Components + Raw Dependencies + Capture State，不保存 import history；ComponentRepository 原子 replacement，不 merge。UUID 仅为 row identity，每次替换重建，不提前推断跨导入身份；sourceOrder 仅指 Parser 展平顺序。
- 显式 Apply 与 Preview 分离，Quality Error 仍可 Apply；单一 ApplyState 管理 Working / Applied 和重复保护。后台数据库任务在自身线程创建、使用和销毁独立连接，使用同一正式路径，GUI 回主线程更新。
- Raw Dependency != Derived Graph，Not Captured != Captured Empty；保留原始声明，不把未捕获或无法解析解释成无依赖。当前依赖引用使用 exact bom-ref matching，Ambiguous 不猜测，禁止身份 fallback；该规则不替代 Phase 07 的 package identity 规则。
- SQLite = business truth；组件、依赖与捕获状态统一原子替换且一致读取，Runtime Graph = derived invalidatable state。graph build once + BFS on demand，no all-pairs closure；self-loop direct 保留、transitive 排除起点，顺序不依赖 hash 迭代。
- 后台 analysis / query 值捕获、worker 自有 DB connection、GUI 线程安装结果；Project ID / request generation 校验并 discard stale async result，不持久化 token、不另建业务 revision。
- 100000 组件 / entries / targets 的自动及人工表现是当前机器证据，不是保证；真实 SBOM 的字段长度、数据库体积与机器负载变化需重新验证，包括仍在 GUI 线程读取的当前组件列表。
- 真实 SBOM 属于 PRIVATE RUNTIME DATA；自动测试使用 synthetic 数据或已审查公开 frozen research fixtures，人工 / live 验收仅使用已审查公开包与 synthetic 数据。人工 JSON、数据库、日志、截图、缓存和构建产物均不纳入 Git；Phase 11 的例外仅为第 16.1 节明确列出的六个公开冻结输入文件。未来提交仍须分别审查候选文件、staged diff 和待推送 commit，历史 PASS 不代替当前审查。

- OSV Provider query 与 Local Applicability 分离；Candidate ≠ Finding，不以本地未命中反向推翻 provider version-query 证据。Exact versions 与 SEMVER precedence 分离，Unsupported evidence → Unknown，withdrawn → Excluded。
- Finding derived in-memory，schema 4 不变；Cache raw / Derived result 分离。Snapshot + fetchedAt + component + generation 确定结果归属，整批后台计算后原子发布；PyPI normalization 复用单一规则，不把版本比较放入 Controller。

**AI 分工：** ChatGPT 负责规划、Phase、Prompt、方案、审查及验收设计；Codex 负责读取真实工程、实现、编译、自动测试和 Git 检查；用户负责确认验收结果和决定是否进入下一 Phase；GUI 可由用户亲自操作，也可经用户明确授权由 Codex 执行并保留直接证据、由用户接受结果。

**开发证据职责：** GitHub → committed source / history truth，用于核对已提交源码、commit 与 tag；Read-Only MCP → local development state / evidence，补充 GitHub 不可见的本地状态与受限证据；PROJECT-HANDOFF → semantic project state / decisions / roadmap，仍是唯一长期项目语义交接文档。MCP 不构成第二套业务真相，其输出不自动覆盖实际运行结果、真实代码或 Git；历史 LastTest.log 不能单独证明当前 HEAD 已测试通过，授权元数据访问不代表 ChatGPT 获得完整本地文件访问权。

ChatGPT 已真实连接 Read-Only MCP。标准业务 Phase 可在开始前核验 baseline、Codex 开发完成后交叉核对本地 Git / validation evidence、最终封版后核验本地最终状态三个节点使用 MCP；它只是证据源，不新增审批层或多轮循环审计，目标是减少信息中转。

**长期阶段闭环：** 设计 → 设计审查 / 冻结 → Codex 开发 → Build / 自动测试 → ChatGPT 审查 → 用户亲自或授权执行的 GUI 验收及用户确认 → 最终回归与更新 PROJECT-HANDOFF → 授权封版与远端核验 → 下一阶段设计；MCP 只增强 baseline 与 evidence verification，不替代 Codex、用户 GUI 验收或本文，不跨阶段预建完整系统。

**具体执行与封版顺序：** 冻结设计 → 实现 → Build / 自动测试 → ChatGPT 审查 → 用户亲自或授权执行的 GUI 验收及用户确认 → 最终回归、维护性 / 隐私审查与本文更新 → 按授权 commit / push / annotated tag → 核验远端与工作区 → 报告完成。46.md 的 Phase 10 与 51.md 的 Phase 11 封版均已按此顺序完成；后续任务按用户新授权执行，不自动启动下一阶段设计或开发。

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
- **当前忽略基线：** 已覆盖运行时 SQLite 数据库及 journal / WAL / SHM 伴随文件、明确的私有配置文件名与 `config/local.*` / `config/private.*`；允许安全的 `.env.example`，不整体忽略 `.sql`、`.ini`、`.json`、`.svg`、`.csv` 或 `.pdf`，以保留公开模板、synthetic fixture 和文档资源。Phase 07 增加 `**/cache/osv-v1/`，Phase 09 增加 `**/cache/risk-evidence-v1/`，build 目录及其中 manual-data / SBOM / response / screenshot / logs 继续 ignored。后续真实导入及输出目录按实际结构隔离。
- **发现待提交敏感内容：** 停止该文件的 staging / commit；优先将敏感内容移出 tracked 文件或替换为空值／placeholder / example，必要时在获准范围内补充 `.gitignore`，保留真实用户数据供本地使用。不能以“马上会删”或“还没 push”为理由提交。
- **已进入历史的秘密：** 立即停止继续传播并报告用户，判断凭据是否真实有效，优先安排 rotation / revoke，检查 Git 历史及公开暴露范围，再决定历史清理；删除当前文件并再提交并不能清除历史泄露。未经用户明确授权，不得 force push、rewrite history、使用 BFG / filter-repo 或删除远程历史；历史重写按 19.4 停止并询问。

# 20. 当前待定事项

- **PHASE 13 COMPLETE / FINAL SEAL PASS。** 64.md 全部 gates PASS，唯一 completion commit、main 与 annotated phase-13-complete 均已推送并核验。当前等待 Final Integrated Acceptance / Thesis Delivery Hardening REQUIREMENTS / DESIGN 新授权。
- Phase 11、12 审查、验收收口及封版已闭环；Phase 12 Test 20 的 historical NOT VERIFIED / NON-BLOCKING 如实保留，不等于 Phase 13 尚有未验收项。
- 40 个真实样本为分层目的性验证集，真实 multi-CVE Finding=0、Partial=0；multi-CVE / max / Partial / stale / future / 24h 边界由 54 个独立合成用例覆盖，不夸大真实数据覆盖。
- 本轮不启动 Phase 14；已有缺失能力不自动构成后续开发授权。

# 21. 当前已知问题 / 风险

- **Phase 13：Confirmed Functional FAIL=0、BLOCKED=0、NOT VERIFIED=0；19/19 PASS。** 单 Finding、静态快照，打开后不更新 freshness；合法 reportable 用户内容可能带有用户自行输入的敏感信息。没有 native PDF renderer、project-wide report、Report history / persistence、forced-exit recovery；性能数据仅为当前机器证据。这些已知范围限制不是 FAIL。

- **Phase 12：Confirmed Functional FAIL=0、Blocker=0。** 第二次独立 Code Review PASS；GUI 为 26 PASS / 0 Confirmed FAIL / 0 BLOCKED / 1 NOT VERIFIED。唯一限制为 Test 20 的 positive ResolvedPathFound GUI scenario：existing A–G synthetic fixtures do not provide a valid positive resolved dependency-path manual scenario，实际 Root Missing；状态 NOT VERIFIED / NON-BLOCKING，不是已知产品缺陷。自动测试覆盖 dependency states，人工已确认 Dependency Path ≠ Runtime Reachability 边界，不能扩大为整个 Dependency Path 功能未验证。
- Phase 12 只新增语言映射和 UI 表达，不改善冻结数据的代表性或扩大 provider coverage。已通过 exact-byte 与 Replay 全字段比较确认 Phase 11 科学结果不变；后续 Review 若需变更核心规则、schema 或冻结输入，应停止并重新确认阶段范围，不能作为呈现修复悄悄引入。
- **Phase 11 第二次独立 Code Review PASS，GUI / Acceptance 20/20 PASS，Confirmed Functional FAIL=0。** 首次审查修复已闭环；执行来源混合，不能称为用户操作全部 20 项。
- v1 仅 PyPI/npm、matching affected.versions 的确定性 exact version；不从 ranges / 注册表 / 自然语言补版本。正式冻结输入可离线重放，后续在线获取不能保证字节相同，也不保证同样分层或 N。
- 全量公开审计仍保留约79.8MB Manifest、76.9MB Index，供Full Audit CLI、准备验证与测试使用；正式GUI仅嵌入约1.66MB Dataset及sidecar。Runtime不重新验证完整选择历史，此责任保留在Full Audit。worker不访问UI/DB/网络，关闭时已启动工作仍需结束；工程观察见第16.2节，不作性能承诺。

- **Phase 10 Blocker = 0、Must Fix = 0。** 46.md 封版时 Debug / Release 各 182/182 PASS，原 Phase00—09 各 158/158、Phase10 各 24/24；ChatGPT Code Review PASS；GUI Acceptance PASS、Confirmed Functional FAIL 0、Blocked 0、Mandatory Not Verified 0。历史 Phase 09 当次 GUI 边界与 Phase 10 新证据分别见第 18 节。
- optional Vulkan Headers 缺失仍为 non-blocking warning，不影响当前 Qt Widgets 工程；无需据此安装额外组件。
- 正式目录与 origin 已核对；Phase 00—13 均已封版，本地与远端十四个 tag 的 object / peeled target 一致。Phase 13 固定为 `aa2cc6fe3731ab528140b246aaf041921885ff39`，后续 main 可因文档维护前进；完成态与实时核验方式见第 5 节。ignored 本地产物不能因此公开或删除。
- Windows 原生文件框已有用户人工验收 PASS；自动快速关闭场景的测试限制见第 17 节，后续若修改文件框生命周期，应针对原生框重新验证。
- 已完成 Foundation、Vulnerability Finding、Risk Evidence Enrichment 与 Exploit-Signal Prioritization MVP；仅 selected Finding，不是完整综合风险模型。没有 numeric Risk Score、Risk Level、CVSS calculator / normalization、NVD、EPSS Model Version mapping、stale provisional Priority、asset criticality、mission impact、internet exposure、runtime reachability；没有 Finding / Profile / Assessment / Quality / Report persistence、batch ranking、Dashboard、project-wide report 或 remediation SLA；Phase 13 单 Finding HTML 报告已实现，ValidationReplay 实验 JSON artifact 属于另一类研究产物。Decision Evidence Support 不是 predictive confidence。
- 真实 SBOM dependency 声明可能不完整；可靠依赖分析不等于 runtime reachability，空结果不能证明没有依赖或漏洞。100000 规模性能只是当前机器证据；Phase 05 组件列表仍在 GUI 线程读取，真实长字段及负载变化须重新评估。
- 当前仅支持 PyPI/npm、仅 selected component 查询；OSV coverage / external availability、身份歧义和 cache freshness 仍有限制。ECOSYSTEM / PEP 440 与 GIT commit graph 未实现，Unsupported evidence / Unknown 不能解释为安全；Findings 不持久化，重启或失效后必须重新派生。缓存仅为历史外部证据，未实现全局容量治理或跨进程协调；身份不明、查询失败或无候选均不能当成“安全”的证明。应在对应阶段验证实际支持范围和错误 / 不确定状态。
- CVSS structural-only，不计算完整语义分数；外部 provider evidence 可能失败或过期，24h TTL 只是刷新政策。Dependency Context ≠ runtime reachability；ReferenceResolutionComplete ≠ dependency coverage complete。Quality 未持久化，当前明确 UnavailableForPersistedCurrentState；不能假定 Current Components 带有完整质量报告，缺失证据不得伪装成精确评分。
- 真实大 SBOM 差异、跨导入身份和 scope creep 仍有风险；以单 Phase 逐步冻结控制范围，不无限扩展生态、同步平台、Provider 框架或基础设施。
- 父目录旧资料包含重复版本；后续以本文当前状态和更高优先级实查证据为准。

# 22. 下一步

**PHASE 13 COMPLETE / FINAL SEAL PASS。** App 0.14.0 / schema 4 / Rules v1 / 0.90；ChatGPT Source-level Review PASS，GUI / Report Acceptance 19/19 PASS，混合 provenance 见第 18 节。64.md 已完成的封版结果以 `build-debug/phase13-final-seal/Phase13-Final-Seal-Report.md`、remote main 与 annotated phase-13-complete 为证据。

64.md 的封版操作已完成；本次独立文档维护提交推送后停止，后续业务工作需新授权。**READY FOR FINAL INTEGRATED ACCEPTANCE / THESIS DELIVERY HARDENING REQUIREMENTS / DESIGN**；该方向不是本轮开发授权，不启动 Phase 14。

**Auxiliary 已收口：** Read-Only MCP 的真实 ChatGPT 连接、调用及重连验收已 PASS，按需手动运行；MCP / Tunnel 不可用不得阻塞产品开发，可继续使用 GitHub + 最新 PROJECT-HANDOFF + Codex。

# 23. 新 Codex 接手规则

1. 先阅读本文及当前阶段具体指令，再读取真实代码、Git 和必要的 GitHub 状态；按 19.2 定位调用链、状态来源、已有实现和扩展点，不凭历史文字假定实现存在。MCP 与 Tunnel 运行且连接可用时，新 ChatGPT 对话可优先用 get_project_status、get_git_status、get_handoff_outline、get_handoff_section 读取 baseline 与必要章节，无需默认上传全文；不可用时上传最新 PROJECT-HANDOFF 作为标准 fallback，项目不依赖 MCP 才能继续。
2. 如实区分已实现、已验证和规划；发现冲突按本文顶部真实性优先级处理，并更新相关状态。
3. 延续已有简单合理结构，不因 Prompt 出现类名或目录建议就机械创建；只执行授权阶段，同时考虑功能正确性、回归风险和后续可维护性。
4. Bug 优先修根因，避免 workaround 链；允许有测试保护、与当前需求直接相关的小型重构。大范围架构调整及 19.4 所列高风险问题先暂停，提供方案并获得用户确认。
5. 实现后完成适当构建、测试及 19.6 的 diff／维护性检查，清楚记录结果与限制；阶段通过必须包含用户要求的人工验收。
6. 未经允许不得 `reset --hard`、`clean -fd`、force push、rebase、删除 branch / tag、删除用户文件或重写 Git 历史；发现已有错误 remote 先报告，不自行覆盖。身份配置优先仓库级，冻结环境不无故变更。
7. 完成阶段后按 19.6 就地更新本文，不另造动态管理文档，不追加重复状态或全过程日志。
8. 长期工程规则持续生效；各阶段以 `phase-XX-complete^{commit}` 固定定位，Phase 00—13 历史 tags 不得移动。Phase 13 已按 64.md 以单 completion commit 完成封版，固定标识见第 5 节。本次独立 docs commit 来自封版后的用户新授权，不改变封版提交或 tag；未来文档维护也不应被误称为再次封版。接手时重新读取真实 Git / remote，保留 ignored evidence；辅助 MCP 独立于产品。
9. 任何 git add / commit / push 前均须按 19.7 审查候选／staged 文件及 privacy / sensitive information，push 同时检查待推送 commits；不得将真实 runtime database、SBOM、日志、用户数据、私有配置、API Key、token 或 credential 加入 Git。`12.md` / `13.md` 的隐私规则及 `.gitignore` 维护已独立提交并推送，其规则继续适用于本次及后续封版。
10. 当前四层路线是唯一有效长期方向；旧 Phase 07—17 安排仅属 Historical / Superseded Plan。Phase 12 已封版，其历史 Test 20 NOT VERIFIED / NON-BLOCKING 保留。Phase 13 Review PASS、GUI / Report 19/19 PASS，provenance 以 64.md 为准；Final Seal 已完成，等待 Final Integrated Acceptance / Thesis Delivery Hardening REQUIREMENTS / DESIGN 新授权，不启动 Phase 14。
11. 遵循“大方向冻结 + 单 Phase 逐步冻结”；未来详细 Phase 设计须在该阶段开始前、根据已完成成果逐步冻结，不提前创建未来表、领域模型、Repository、Service / Provider 框架、UI 或评分公式。Candidate 不等于 Affected Finding，Applicability gate、Priority / Decision Evidence Support 分离和 Quality 证据原则持续生效；Phase 10 Rules v1 已冻结，不能扩大为完整风险模型或预测置信度。
