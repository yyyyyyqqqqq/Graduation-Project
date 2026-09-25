# 毕业设计项目总交接文档

更新日期：2026-09-25（Asia/Shanghai；Phase 07.5-AUX 真实连接验收与最终收口）

本文是项目唯一的长期动态交接文档。每个阶段结束后，更新当前状态、代码结构、Git、测试、人工验收、技术决策和下一步，避免另建多套状态文件。

本文保留此前授权的审计修订、Phase 00—06 已完成事实及 `22.md` 冻结的长期路线。Phase 07 按 `23.md` / `24.md` 设计、`25.md` 开发，已按 `26.md` 最终封版并执行 Debug / Release 构建和全部回归。用户在 `26.md` 明确确认亲自完成 Debug GUI 人工验收 18/18 PASS；这是用户实际操作结果，不是 Codex 模拟或自动测试替代。Release GUI 为 AUTOMATED ONLY。

Phase 07.5-AUX 已按 `28.md` 完成本地 MCP 实验，并按 `29.md` 完成采用文档更新。本轮依据用户在 `30.md` 提供的真实 ChatGPT 连接及重启后重连 PASS 验收，完成辅助基础设施收口、修正 MCP 过时状态并更新本文；正式仓库仅增加普通 docs commit，不改变产品功能或正式业务 Phase。下文 Phase 07 的实现、构建、测试、人工验收与封版记录均为该阶段历史成果，本轮未重新执行，不开始 Phase 08。

依据：项目历史资料、已通过的理解与交接审查、`04.md` 开发任务、`05.md` 封版授权及人工验收结果、`06.md` 长期规则增强要求、`07.md` 文档提交授权、`08.md` Phase 01 开发要求、`09.md` Phase 01 封版授权、`10.md` Phase 02 开发要求、`11.md` Phase 02 人工验收及封版授权、`12.md` / `13.md` 隐私规则与维护授权、`14.md` Phase 03 开发要求、`15.md` Phase 03 人工验收及最终封版授权、此前用户授权的 Handoff 审计更新、`16.md` Phase 04 开发要求、`17.md` Phase 04 人工验收及最终封版授权、`18.md` Phase 05 开发要求、`19.md` Phase 05 人工验收及最终封版授权、`20.md` Phase 06 开发要求、`21.md` Phase 06 人工验收及最终封版授权、`22.md` 长期路线调整及独立文档提交授权，`23.md` / `24.md` Phase 07 设计、`25.md` Phase 07 开发、`26.md` 用户人工验收及最终封版授权，以及各阶段正式工程构建、测试和现场 Git / GitHub 实查。

**真实性优先级：实际运行结果 > 当前真实代码 > 当前 Git 状态 > 当前 GitHub 状态 > 已验证环境审计 > 历史项目资料 > 推测。** 当前阶段授权以用户最新具体指令为准。

# 1. 项目基本信息

- 题目：**基于 C++ 的软件供应链漏洞风险评估系统的设计与实现**。
- 性质：本科毕业设计，面向软件项目开发者、维护者及答辩演示。
- 定位：使用 C++ 实现能够运行、测试、演示并支撑论文的软件供应链风险评估桌面系统，控制在本科毕业设计合理范围内。
- 实现语言为 C++ 不代表输入组件仅限 C++ 生态；当前身份与漏洞候选匹配支持 PyPI / npm（含 scoped npm）。
- 正式仓库：[yyyyyyqqqqq/Graduation-Project](https://github.com/yyyyyyqqqqq/Graduation-Project)。

# 2. 当前一句话状态

Phase 07 已完成 Component Identity 与 OSV-first Vulnerability Matching MVP，并通过自动和用户人工验收。Foundation 基本完成，Vulnerability 层已实现 Current Components → Package Identity → OSV Candidate；Version Applicability / Finding 尚未实现，漏洞风险评估核心闭环仍待后续阶段完成。

# 3. 当前阶段

**CURRENT PHASE: PHASE 07 COMPLETE**

**NEXT: Vulnerability continuation — Version Applicability / Finding design**

**READY FOR NEXT PHASE DESIGN。** Phase 08+ 仍先做需求与技术设计，具体名称和范围待下一轮冻结；本轮封版后停止，不开始设计或开发。

# 4. 正式项目目录

唯一正式根目录：`D:\codex\Graduation Project\project`。

后续源码、CMake、测试、项目文档和 Git 均围绕此目录组织。不维护多个正式工程副本；父目录的 `01.md` 至 `26.md` 是既有准备、设计、开发、封版与文档维护任务的输入资料，另已核验 `28.md` 为独立 MCP 实验、`29.md` 为采用文档更新、`30.md` 为真实连接验收及本轮最终收口授权。

`D:\codex\SupplyChainRiskAssessment` 是历史环境验证目录，不是正式项目；本轮未检查或修改其内容。

# 5. Git / GitHub 当前状态

Phase 07 开发及封版前基线为 `dedd79dfeaab8245bfcb1568ccf28a8913f6f59e`（`docs: realign project roadmap around vulnerability risk pipeline`）。本轮开始时，HEAD / main / origin/main / 远端 main 均为已授权的采用文档提交 `d77f4f50868d5b63842159e1f8198ba70225fa33`（`docs: record phase 07.5 readonly mcp experiment`），ahead / behind 为 0 / 0，工作区 clean；本轮仅在其后追加普通 docs commit，Phase 07 completion commit 与 tag 保持下表固定值。

| 项目 | 封版标识与核验方式 |
| --- | --- |
| 当前分支 / upstream | `main` / `origin/main` |
| origin（fetch / push） | `https://github.com/yyyyyyqqqqq/Graduation-Project.git` |
| Phase 07 completion commit | `ad8c75274a2a8c4db275ab099f949a7c47fcecdb`（`feat: complete phase 07 osv vulnerability matching`），由 `phase-07-complete^{commit}` 固定定位，不随后续 docs commit 移动 |
| Phase 07 tag | annotated `phase-07-complete`；message：`Phase 07 complete: component identity and OSV matching` |
| main 同步核验 | 本轮普通 docs commit 推送后核验 HEAD = origin/main = remote main，ahead / behind = 0 / 0 |
| tag 核验 | 核对本地 / 远端既有 tag object 与 peeled target；Phase 00—07 tags 均不移动，不创建 Phase 07.5 tag |
| 工作区核验 | 本轮仅提交 PROJECT-HANDOFF.md，main 推送与历史标签核对后确认 tracked / untracked clean；ignored build / manual artifacts 保留 |

Phase 07 封版时，本文随 completion commit 保存，后续可通过普通 docs commit 维护，不能再用“本文所在提交”推断产品封版提交。本轮新 docs commit 的精确 SHA 与远端实查结果在 Phase 07.5-AUX Final Seal Report 回复中报告，不把自身尚未生成的 hash 写入自身；后续接手用下述命令重新核对，不只依赖文档中的阶段状态。

既有提交使用仓库级身份 `yyyyyyqqqqq` / `104704290+yyyyyyqqqqq@users.noreply.github.com`。仓库级 `credential.https://github.com.username` 为 `yyyyyyqqqqq`，用于 HTTPS 认证账户选择；认证账户与 commit 作者配置不同。本轮不修改凭据或全局 Git 配置。

Phase 00—06 的七个 annotated tags 在封版前已核对本地对象、远端对象与 peeled target；封版后须再次保持下列 target 及原 tag 对象不变：

| Tag | 固定 target |
| --- | --- |
| phase-00-complete | `765bfaeb69bab26de80ff0bf3b24358688d89bca` |
| phase-01-complete | `64292ab39bc9ffbf13c36058e47b055c812a6344` |
| phase-02-complete | `fc9ef4ea6b68a9924fed9430cb64cbcd7bbeb0b8` |
| phase-03-complete | `e5ef271c5249c9df6afe91abf37f163e4c9e73df` |
| phase-04-complete | `febb52ef16100eaa61d84e6fc7e6e643b2a5c5f8` |
| phase-05-complete | `a7ff0bb0338841b46975e832373d6eb26e5060b7` |
| phase-06-complete | `a9a1c180b7093c4553c0862d55536cf8e6ae7d0d` |

历史阶段开发前基线：Phase 03 为 `e10351523d5115cfb620d1175265061979fc220c`；Phase 04 为 `e5ef271c5249c9df6afe91abf37f163e4c9e73df`；Phase 05 为 `febb52ef16100eaa61d84e6fc7e6e643b2a5c5f8`；Phase 06 为 `a7ff0bb0338841b46975e832373d6eb26e5060b7`。Phase 06 tag 固定指向该阶段 completion commit，不追随后续 main。

同步核验使用 `git status`、`git branch -vv`、`git log`、`git rev-list --left-right --count main...origin/main`、`git ls-remote origin`、`git cat-file -t phase-07-complete` 及 `git rev-parse 'phase-07-complete^{commit}'`。main 同步不能代替 tag 核验。远程是重新创建后的同名仓库，不继承旧仓库历史；今后操作前仍需读取真实状态。

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
| Qt Network / Qt JSON | Network 已在 Phase 07 的 OsvMatching 正式接入；JSON 使用 Qt Core 中的 QJson 类型 |
| Qt Concurrent | 后台导入 / 诊断、数据库 Apply、依赖快照 / 图与关系查询；Phase 07 用于 Current Components / Identity、OSV 解析、文件缓存与证据显示序列化 |
| Qt Test / CTest | 自动测试及测试执行 |
| Graphviz / Qt Svg | Graphviz 16.1.0 当前仅用于 Phase 00 dot → SVG 冒烟；正式应用未接入 Graphviz 依赖图绘制或 Qt Svg 展示 |
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

历史结论证明当时工具链集成可用。正式工程的 Phase 00 原位最小复验已通过，后续 Phase 01 / 02 / 03 / 04 / 05 / 06 / 07 延续同一冻结工具链；CMakeCache、编译命令及运行结果均确认 Qt 6.11.2，未混入 6.11.1，没有重装或替换工具链。

# 7. 环境已知提醒

- Qt 6.11.2 是正式基线；6.11.1 仅是已验证回退环境，不随意切换、卸载、升级或替换工具链，不无故修改系统 PATH。
- **NVD API Key 历史状态为未确认 / PENDING，本轮没有重新检查。** 早期阶段不受阻，批量同步前处理；密钥禁止写入源码、日志和 Git，可用用户环境变量 `NVD_API_KEY` 或受忽略的本地配置。
- Graphviz 最终部署策略待定，`dot.exe` 属于外部依赖，不由 windeployqt 自动打包。
- SQLite 的 `QSqlDatabase`、`QSqlQuery` 等对象不跨线程共享；各线程使用自己的连接，处理事务与锁；GUI 更新在主线程执行。
- 历史 `LongPathsEnabled=0`，保持浅目录；正式路径含空格，命令必须正确引用。
- 历史 optional Vulkan Headers / DX12 compiler 提示不阻塞当前 Widgets + SVG 方案；Qt 导入配置为 RelWithDebInfo，历史 Debug 已验证，不据此重装。
- 历史独立部署验证是在当前电脑排除开发路径后完成，尚不等同于全新 Windows 电脑验收。
- **当前无已知环境 Blocker。** 本轮按 `26.md` 重新执行正式工程 Debug / Release 构建与全部回归，沿用正常的已有构建目录，没有无故 Clean Configure、重装或更换冻结工具链；未重跑完整历史环境审计。

# 8. 系统核心目标

最终产品应回答：项目使用了什么组件，哪些组件关联漏洞，哪些漏洞适用于当前版本，哪些风险应优先处理，以及为什么得到这样的风险和置信度结论。Foundation 已基本完成：能预览 SBOM 组件、诊断质量、显式应用当前组件与原始依赖，并分析可靠直接 / 反向 / 传递关系；已能由 Current Components 得到身份与 OSV 候选，版本适用性、Finding 和风险核心闭环尚未实现。解析成功或当前质量规则未发现问题，均不等于 SBOM 完整、组件真实存在或没有风险。

结果需要可解释、可追溯，后续通过实验、Dashboard 和报告支持论文及演示；扫描记录、历史比较的必要范围在对应阶段再确定。不能让完整业务完全依赖实时公网 API，应按实际 Phase 设计缓存、可复现验证及本地演示方式，不因此提前建设完整离线镜像。

论文与答辩以 **Validation Experiment** 为定位，不作 General Benchmark。暂定约 30–50 个真实 component-version 样本，最终数据集尚未冻结；结论限于选定测试项目、package ecosystem 与验证数据集上实际观察到的系统功能和方法行为，不宣称所有生态的普遍识别准确率。

Trivy、Grype、Dependency-Track、OSV-Scanner、cve-bin-tool 等成熟工具在覆盖面、成熟度和生态上明显高于本科项目；本系统不以替代它们为目标。项目价值在于完整桌面系统、可运行的 SBOM → Vulnerability → Risk 分析链，以及 Component Identity、Version Applicability、Risk / Confidence 分离、Explainability 与验证实验。

候选主要工作 / 特色设计为：（1）基于 SBOM 的组件漏洞候选匹配、版本适用性判断与可解释风险优先级分析流程；（2）将 SBOM 数据质量与证据完整性关联，通过 Risk 与 Assessment Confidence 分离表达风险程度和结论可信程度。这些是后续拟实现并验证的工作，不是已经完成的创新算法，不作“首次提出”或“全面优于”声明。

# 9. 核心业务流程

**当前已实现：** Project → SBOM Parse → Quality Diagnosis → Explicit Apply → Current Components + Raw Dependencies → Dependency Analysis；Current Components → Package Identity → OSV /v1/query → Vulnerability Candidate。

**唯一有效长期路线：Foundation → Vulnerability → Risk → Validation / Presentation。**

| 层 | 正式技术主线 | 当前状态与职责 |
| --- | --- | --- |
| Layer 1 — Foundation | SBOM → Component → Quality → Dependency | 基本完成；Phase 00—06 已建立标准化供应链输入、CycloneDX 解析、组件模型、质量诊断、SQLite / Current Components / Raw Dependencies 持久化及依赖分析，为漏洞分析提供可追溯输入与项目上下文 |
| Layer 2 — Vulnerability | Component Identity → Candidate Matching → Version Applicability → Finding | 部分完成：Identity + OSV Candidate Matching 已实现；Applicability / Finding 尚未实现，下一步先设计 |
| Layer 3 — Risk | CVSS / EPSS / KEV + Dependency Context + Quality Evidence → Risk + Assessment Confidence → Explanation | 尚未实现；解释 Finding 成立时的处理优先级、证据充分程度及判断理由；Risk Model 尚未冻结 |
| Layer 4 — Validation / Presentation | Experiment → Dashboard → Report | 尚未实现；用于方法验证、论文实验、答辩和结果呈现，不是当前优先开发目标 |

缺失数据保留诊断或不确定状态，不能解释成无漏洞、无风险。第 13 节只将这条主线映射为已完成阶段、下一阶段定位和后续方向，不另设并行正式路线。

# 10. 核心模块

模块职责按第 9 节四层路线组织；这是业务职责划分，不是提前创建未来类、目录或框架的指令：

| 模块 | 主要职责 |
| --- | --- |
| Foundation | 项目创建 / 列表 / 详情 / 删除、CycloneDX JSON 1.4 / 1.5 / 1.6 共同字段子集解析与只读预览、issue-based 质量诊断、当前组件及原始依赖事务持久化、精确引用解析、直接 / 反向及按需传递查询均已实现；无总体质量分或自动修复 |
| Vulnerability | 已实现 PyPI/npm 最小身份与所选组件 OSV 候选查询、原始证据、文件缓存和隐私确认；版本适用性与 Finding 待后续设计 |
| Risk | 未来结合漏洞 enrichment、依赖上下文与质量证据，分别给出 Risk、Assessment Confidence 和 Explanation；模型及实现未冻结 |
| Validation / Presentation | 未来通过验证实验、Dashboard 和 Report 呈现结论；Graphviz 后移为辅助 presentation / explanation 能力，不是下一开发优先项或 blocker |

Project / projects 与 Component / components 已实现正式持久化；SbomDocument、SbomComponent、SbomDependency、SbomQualityIssue、SbomQualityReport 仍为导入或诊断内存模型。DependencySnapshot 表达一致读取结果，DependencyGraph 为可失效的内存派生图；原始依赖已持久化。PackageIdentity / QueryIdentity 与 VulnerabilityCandidate / OsvSnapshot 已实现为内存值模型，完整成功 OSV 查询另有外部证据文件缓存；没有正式 identity / vulnerability / finding / risk 表，缓存不是第二业务数据库。Finding、Risk、Confidence、扫描与报告仍待后续设计。

# 11. 漏洞数据源定位

依据用户在 `22.md` 确认的路线决策，**OSV 是 Phase 07 MVP 的首选 vulnerability matching source；OSV-first 是 MVP 和第一版端到端闭环的当前主数据源策略，不是系统唯一最终漏洞数据源。**

- **OSV**：围绕 package / version identity 获得真实漏洞候选。选择理由是与 PURL / ecosystem 路线自然衔接、实现成本相对可控、较快跑通候选链，并避免 MVP 起步即陷入完整 PURL → CPE 映射。
- **NVD**：未来可用于漏洞基础信息与 CVSS 等 enrichment，不作为 Phase 07 MVP 主匹配链。
- **EPSS**：未来可补充 exploit likelihood，服务风险分析，不能证明版本适用性。
- **CISA KEV**：未来可补充 known exploited evidence；未收录不等于没有风险。

OSV 已正式接入：固定 HTTPS `https://api.osv.dev/v1/query`，仅查询所选 PyPI/npm 组件，支持完整分页、错误分类、取消和 AppData 文件缓存，详见第 16 节。NVD / EPSS / KEV 尚未接入。开发轮真实 Qt Network smoke 于 2026-09-18T09:57:04Z 查询公开 PyPI six 1.17.0 成功（当时 0 candidates，约 2.3 秒）；该数量仅为当时结果，不是永久保证。本轮最终 CTest 为离线回归，不重跑实时服务 smoke。

# 12. 风险评估原则

**以下仅为长期设计原则，Applicability、Finding、Risk 和 Assessment Confidence 均未实现。Risk ≠ CVSS；Risk Model 尚未冻结。**

正式长期顺序为 **Component → Vulnerability Candidate → Version Applicability → Finding → Risk**。Applicability 是 Risk 的前置门控：Affected 可以形成有效 Finding；Not Affected 不形成有效风险 Finding；Unknown 不得输出看起来确定的 Risk，可进入 Assessment Uncertain。名称相似、相关 CVE 或获得 Candidate 均不能直接认定当前版本受影响；依赖关系也不等于 runtime reachability。

Phase 07 已保留 OSV candidate 的 affected 原始证据，但未执行 ranges / events / versions 的适用性判断。下一阶段须根据真实 QueryIdentity、Candidate、affected evidence、ecosystem 与 version 设计 Applicability / Finding；**Identity Resolved ≠ Version Applicable；Candidate ≠ Affected ≠ Finding。** 不得从 Candidate 直接进入 Risk。

**Risk** 回答“如果 Finding 确实成立，它有多值得优先处理”；**Assessment Confidence** 回答“当前证据对该判断支持有多充分”；Explanation 解释两者结论。Confidence 当前倾向 High / Medium / Low，不采用 0–100 的伪精确 Confidence Score，具体规则仍须未来冻结。

Quality Diagnostics 后续应成为证据完整性输入，不能永远只是独立 Warning 页面：Missing Version 可能使 Applicability 无法可靠判断；Missing PURL 表示 Identity Evidence 降低；Dependency Missing / Partial 表示 Dependency Context 不完整。这些问题更自然地影响 Confidence 及 Identity / Applicability 能否确定，**不能简单转为 Risk 加分项**，缺版本不等于风险更高。

第一版风险模型须在获得真实 Affected Findings 与 enrichment 数据之后再冻结。当前仅保留 CVSS / EPSS / KEV、Dependency Context、Quality Evidence 等候选证据方向，不规定正式权重、数学公式、阈值、分数映射、聚合方式或缺失值规则；历史环境实验也不构成正式风险算法。

# 13. Phase 路线

以下是第 9 节唯一长期路线的阶段状态。Phase 00—07 成果完成；Phase 08+ 仅保留方向，不提前分配详细实现阶段。

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
| 08+ | Vulnerability continuation → Risk → Validation / Presentation | 仅方向；具体阶段逐步设计和冻结 |

**Historical / Superseded Plan：** Phase 00—06 开发期间使用的旧 Phase 07—17 未来安排已被 `22.md` 路线决策取代，旧表从正式路线中移除，历史可由 Git 查询；不得据其启动开发。Graphviz 仅为后续辅助展示与解释能力，不是 Phase 07 任务。

## 13.1 Phase 07 已完成定位与范围

**Phase 07 — Component Identity + OSV-first Vulnerability Matching MVP，COMPLETE。** 已实现 **SQLite Current Component → Package Identity → OSV → Real Vulnerability Candidate**；正式来源是 Explicit Apply 后的 Current Components，不是 Preview。

当前 PURL-first 严格子集支持 PyPI、npm 与 scoped npm，无 Component.name / type / bom-ref fallback，不改写原始 Component。Quality report 仍为未持久化的导入期内存模型，Current Component 不自动携带完整质量报告；后续证据关联仍需设计。

已输出 ecosystem、查询包名、精确版本、版本来源、Identity 状态 / reason、OSV ID、CVE aliases 与原始证据。Resolved / Insufficient / Ambiguous 和 NotStarted / Working / Success / Failed / Cancelled 分离；只有完整成功查询的候选数才有确定含义。失败、取消或身份不足显示未知，完整零结果只说明当前查询未返回候选。

具体身份规则、分页 / 限额 / 缓存、异步生命周期及 UI 见第 16 节；应用版本 0.8.0，schema 保持 4，没有新增业务表。

## 13.2 Phase 07 明确边界

Phase 07 未实现且本轮不增加：

- 完整 PURL → CPE；NVD 作为 MVP 主匹配链；EPSS；KEV。
- Risk Score；Risk Model；Assessment Confidence 实现；正式 Applicability / Finding 业务层。
- 所有 package ecosystem；Debian / Ubuntu backport；vendor-specific patch 完整处理；SPDX。
- Graphviz 新功能；企业级漏洞同步；完整离线漏洞镜像；多 Provider Framework。
- 二进制逆向；Machine Learning 风险模型。

已保留候选中的 affected evidence，下一步进入 Version Applicability / Finding 的需求与技术设计，门控原则见第 12 节；不从封版授权推断 Phase 08 开发授权。

## 13.3 Auxiliary Development Infrastructure

| Task | Name | Type | Status |
| --- | --- | --- | --- |
| Phase 07.5-AUX | Read-Only Project Context MCP | Auxiliary Development Infrastructure | COMPLETE / CHATGPT CONNECTION PASS |

Phase 07.5-AUX 已完成本地实验验收并采用为辅助开发基础设施。它位于正式 Git 仓库之外的同级独立目录 `project-readonly-mcp`，不属于 Product Feature 或论文业务 Phase，不插入上方正式 Phase 表或 Foundation → Vulnerability → Risk → Validation / Presentation 业务路线；不改变正式 CURRENT PHASE、产品架构、数据库、版本或产品能力。

实现使用 Python + 官方 MCP SDK，经 localhost Streamable HTTP 提供服务，绑定 `127.0.0.1`、endpoint `/mcp`。v1 恰好 7 个 tool：项目高层状态、本地 Git 状态、Handoff 单章节 / outline、历史 build / CTest evidence、匿名 runtime SQLite summary、runtime storage metadata；extra tools / resources / resource templates / prompts 均为 0。

Git 仅受限只读查询，SQLite 使用真正 read-only connection，仅支持的 schema 4 返回匿名 aggregate；runtime storage 仅元数据。无任意 filesystem / shell / Git args / SQL，不返回 Project / Component 业务记录、包身份 / PURL / SBOM、应用日志、settings 或原始 OSV cache 正文。历史 CTest artifact 不证明当前 HEAD 已重新验证，数据库 aggregate 不代表业务内容。

本地验收结论：正式工程与数据库只读 integrity proof PASS；自动测试 76 项（75 PASS / 0 FAIL / 1 SKIP），skip 为当前 Windows 权限无法实际创建 file symlink，真实 junction 与 reparse 防护已验证；MCP Inspector 52 项 PASS。实验未修改正式产品工程、数据库或运行数据。详细技术资料和验收记录留在独立工具的 README.md / REPORT.md，不复制 local 证据，也不作为第二套长期项目状态源。

**CHATGPT MCP CONNECTION TEST: PASS**

用户在 `30.md` 确认真实链路 ChatGPT → OpenAI Secure MCP Tunnel → tunnel-client → localhost Read-Only MCP 已建立；ChatGPT MCP App 已连接，恰好 7 个工具发现 PASS，get_project_status / get_git_status / get_handoff_outline 实际调用 PASS，23 个一级章节 outline 读取成功，停止并重启本地服务后的 reconnect / invocation 也 PASS。这是实际用户 / ChatGPT 验收，不是 Codex 模拟或仅由 Inspector 推断。此前 READY FOR CHATGPT CONNECTION TEST 已完成验收；当前 **PHASE 07.5-AUX: COMPLETE**、**READ-ONLY MCP: OPERATIONAL**。

get_project_status 的 auxiliary_task_status 从本文明确的连接验收标记派生，表示已记录的验收结论，不是实时 Tunnel 健康探测。MCP 与 tunnel-client 仍需用户手动启动，用毕 Ctrl+C 停止；两进程关闭后 ChatGPT 无法继续访问。仅 localhost MCP 与出站 Secure MCP Tunnel，无公网 MCP 入站 endpoint、常驻服务或自动启动；不改变产品工程，不开始 Phase 08。

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
- 用户在 `21.md` 确认全部 26 项 Debug GUI 人工验收 PASS，包含大规模响应与 stale-result 无串数据；Release GUI 为 AUTOMATED ONLY。本轮未重新人工操作，性能仅为当前机器证据。
- 最终维护性、数据库 / 线程 / 图生命周期及 Sensitive / Privacy Review PASS；独立 completion commit、main 推送及 `phase-06-complete` annotated tag 封版。后续路线见第 9 / 13 节，当前 Phase 07 状态见下。

**Phase 07 —— COMPLETE。**

- 正式链路为 SQLite Current Components → Package Identity → OSV /v1/query → Vulnerability Candidate，Preview 不作为分析来源；支持 PyPI/npm（含 scoped npm）的严格 PURL 子集，无 name / type / bom-ref fallback。
- Identity Resolved / Insufficient / Ambiguous 与版本适用性分离，保留精确查询版本及来源；版本冲突不猜测，不因尚无完整版本 parser 而拒绝 epoch / local / v prefix。
- 所选组件单并发 HTTPS 查询，分页含 token-only page、重复 token 防护、累计字节 / 页数 / 记录限制、超时与 deadline、错误分类、取消及完整结果发布；OSV ID 为候选主身份，CVE 仅是 alias，affected 原始证据保留但不求值。
- AppData 文件缓存随 --data-dir 隔离；安全固定文件名、内部 QueryIdentity 校验、QSaveFile 原子写、完整零结果可缓存、24 小时 Fresh / Stale、Live / Cache、仅本地查询和显式清除。
- 首次实际联网发送确认、最小请求数据、无自动批量查询；Current Components / Identity、解析与缓存后台任务、generation 失效保护及窗口 / reply 生命周期均有回归覆盖。
- 应用 0.8.0，schema 4 和六表保持；未新增 identity / vulnerability / finding / risk 正式表。100000 Components 读取、身份计算、表格与 GUI responsiveness 验证通过。
- 本轮最终 Debug / Release Build PASS，CTest 各 108/108 PASS（旧回归 82 + Phase 07 26）；用户在 `26.md` 亲自确认 Debug GUI 18/18 PASS，Release GUI 为 AUTOMATED ONLY。
- 维护性及 Sensitive / Privacy Review 按第 19 节执行；completion commit、main 推送与 annotated `phase-07-complete` 的最终实查值见本轮封版报告。封版后停止，下一步只进入新一轮设计。

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
│  ├─ Component.h
│  ├─ ComponentRepository.h / .cpp
│  ├─ DependencySnapshot.h
│  ├─ DependencyAnalyzer.h / .cpp
│  ├─ DependencyPage.h / .cpp
│  ├─ PackageIdentity.h / .cpp
│  ├─ OsvResponseParser.h / .cpp
│  ├─ OsvCache.h / .cpp
│  ├─ OsvClient.h / .cpp
│  ├─ VulnerabilityController.h / .cpp
│  ├─ VulnerabilityPage.h / .cpp
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
   ├─ Phase04Test.cpp
   ├─ Phase05Test.cpp
   ├─ Phase06Test.cpp
   └─ Phase07Test.cpp
```

`.git` 为版本元数据；本地 `build-debug/`、`build-release/`、Qt Creator 的 `build/` 和 `.qtcreator/` 均被忽略，不进入正式提交。运行时数据库、日志和配置位于应用数据目录，不属于正式源码；测试数据库位于临时目录，截图和构建日志留在被忽略的构建目录。

人工验收用 `phase05-large-sbom.json`、`phase05-replacement-sbom.json`、`phase05-empty-sbom.json`、`phase03-demo-sbom.json`、`phase03-not-cyclonedx.json`、`phase03-invalid-json.json`、`phase04-clean-sbom.json`、`phase04-quality-issues.json` 由测试生成于构建目录，全部为 synthetic 数据，保持 ignored / not tracked / not staged / not committed。

`build-debug/phase05-manual.cmd`、`build-debug/phase06-manual.cmd`、Phase 06 normal / issues / cycle / empty / replacement / parser-error / large synthetic JSON、人工验收数据目录及截图继续保留于 ignored 构建目录；Release 自动生成的对应产物也保持 ignored / not tracked / not staged / not committed。Phase06Test 仅在人工数据库不存在时生成 synthetic schema 3 起点，不覆盖用户已有人工验收数据库。

临时理解报告已按封版授权删除，未进入任何正式提交；唯一长期动态交接文档为本文。

Phase 07 的 public / large / empty / invalid SBOM、cache fixture、launch 脚本、人工验收说明、截图、manual DB / logs、live smoke 记录均留在 ignored 构建目录；`phase07-manual-data` 不提交、不覆盖或删除。测试源码仅包含公开包或 synthetic 构造数据，不提交运行时 response dump。`.gitignore` 另覆盖 `**/cache/osv-v1/`。

当前应用版本为 **0.8.0**，CMake 与 main.cpp 一致。`VulnerabilityCandidate` / `OsvSnapshot` 实际定义在 `OsvResponseParser.h`，没有同名独立文件。CMake target 边界：`SbomParsing`（Parser / Quality Analyzer）与 `DependencyAnalysis` 仅链接 Qt Core；`AppFoundation` 链接 Qt Core / Sql；`VulnerabilityCore`（PackageIdentity / OsvResponseParser / OsvCache）仅链接 Qt Core；`OsvMatching`（OsvClient / VulnerabilityController）链接 VulnerabilityCore、AppFoundation、Qt Network / Concurrent。应用链接 Widgets / Concurrent 及上述业务库，VulnerabilityPage 为应用 UI 源码。测试为 Phase00—07，开启时加入 Qt Test，Graphviz dot 仍只用于既有 smoke；没有新增 Qt Svg 或 Graphviz 业务。

# 16. 当前已经实现的功能

当前已经建立 **Phase 00 工程基础 + Phase 01 Application Foundation + Phase 02 Project Management + Phase 03 CycloneDX SBOM Import + Phase 04 SBOM Quality Diagnosis + Phase 05 Component Persistence + Phase 06 Dependency Analysis + Phase 07 Component Identity / OSV Candidate Matching**。具备 Application Shell、导航、路径、日志、配置和 SQLite 基础；项目支持 create / list / findById / delete / persistence。业务链见第 9 节；预览文档及质量报告仍仅在内存中，当前组件、原始依赖及 Captured State 在明确 Apply 后统一持久化。

默认运行目录由 `QStandardPaths::AppDataLocation` 决定，Windows 通常为 `%APPDATA%/GraduationProject/SupplyChainRiskAssessment`：数据库 `data/supply_chain_risk.db`、日志 `logs/application.log`、配置 `settings.ini`。可用 `--data-dir <绝对路径>` 指定隔离数据根目录，空路径或相对路径会被拒绝。应用自行保存的 UI 配置仅含 `ui/lastNavigationPage`，启动及切换时保存，重启时恢复，未知页面回退概览。

AppLogger 以追加方式写入 UTC 时间、级别和单行消息，写入失败回退标准错误输出；启动时打开或首次写入失败还会显示警告，并继续运行。不能据此宣称所有后续写日志失败都会弹窗。本地基础日志包含运行目录，项目创建 / 删除日志包含项目 ID；它们仍属私有运行数据，不能把 Phase 03 导入日志的安全摘要约束解释为整个日志可以直接公开。

Project 仅包含 `id`、`name`、`description`、`createdAt`：应用生成无花括号 UUID；名称 trim 后必填、1—100 个 Unicode 码点，允许重名；描述 trim 后可空、最多 500 个码点；创建时间为 UTC Unix 毫秒，界面显示本地时间。列表按 `created_at DESC, id DESC` 排序。创建后自动选中；选择项目显示详情；删除需二次确认，取消不写数据库，删除最后一项恢复空状态。

**当前数据库：schema_version = 4；仅有 app_meta、projects、components、dependency_capture、dependency_entries、dependency_targets 六张表。**

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

**当前 SQLite 持久化范围仅为 Project、Current Components、Raw Dependencies 与 Capture State。** 无 sboms / quality_reports / quality_issues / identity / vulnerability / finding / risk / Scan 等表；Phase 07 未升级 schema。Project 不保存 sbomPath 或 import history。OSV 文件缓存为历史外部证据，不是第二业务数据库。

## Phase 07：身份、候选、缓存与异步边界

`ComponentRepository::listForProjectInFile` 在 worker 自有连接和单读事务内验证项目并读取 Current Components，不读取整个 DependencySnapshot。身份解析在同一后台任务完成，加载请求以 generation 合并；UI 只呈现结果，不执行 SQL。PackageIdentity 规则集中且不改写 Component，UUID 仅为 row identity；QueryIdentity 为 ecosystem / name / exact version / identityRulesVersion（当前 1）。

PURL 最长 4096 UTF-16 单元，scheme / type 不区分大小写；源文本采用严格 ASCII URL 子集，percent-encoded UTF-8 只解码一次，非法转义 / UTF-8 拒绝，`+` 不转空格。只支持 pypi / npm，无 name / type / bom-ref fallback；qualifiers 与 subpath 明确拒绝。PyPI 名称转小写并合并连续 `[-_.]`，无 namespace；npm 保留大小写，scoped npm 使用 `%40scope/name`，名称长度上限 214。

Identity 为 Resolved / Insufficient / Ambiguous，reason 包括 MissingPurl、MissingVersion、UnsupportedEcosystem、VersionConflict、MalformedPurl、UnsupportedQualifiers、UnsupportedSubpath 及长度 / 输入安全限制。版本来源为 Purl / Component / Both；两者完全相同或仅一个来源可 Resolved，两者明确不同为 Ambiguous / VersionConflict。纯空白视为缺失；非空字符串保留原值，不 trim、补位、转小写或移除 v prefix，最长 256 UTF-16 单元，拒绝控制字符及无效 UTF-16。epoch、local version、v prefix 或不被本地完整 parser 解释的形式不会仅因此被拒绝。未实现 PEP 440、SemVer applicability、生态版本排序或 range evaluation；**Identity Resolved ≠ Version Applicable**。

OSV 固定 HTTPS `POST https://api.osv.dev/v1/query`，正文只含 package.ecosystem / package.name / version，后续分页增加服务返回的 page_token。每次只查询所选组件，最大并发 1；自动分页支持 token-only page，重复 token 拒绝，最多 20 页、累计 10000 条原始记录、16 MiB 解码后响应字节。reply 读取缓冲为 64 KiB，单次 transfer timeout 30 秒，网络及解析链 total deadline 120 秒。Qt Network 异步 I/O，解析 Qt Concurrent 值任务；仅所有页完整成功才发布及缓存，不暴露部分成功。

错误区分 HTTP 400 / 401、403 / 404 / 429 / 5xx、Timeout、TlsFailure、ConnectionFailure、ResponseInvalid、ResponseLimitExceeded、UnexpectedRedirect、Cancelled 及 Cache / Database 错误。429 解析 Retry-After 并限制过早重发，没有自动重试或修改 identity 重试。禁止重定向自动跟随、自动 cookie 读取 / 保存与 TLS 绕过。QNetworkReply 完成后 deleteLater，销毁时断开回调并 abort；借用 transport 必须在相同线程且寿命覆盖 client。解析任务不捕获 UI。

Candidate 以 OSV ID 为主记录身份，保留完整 QJsonObject：summary、modified、published / withdrawn（若存在）、aliases、affected 及未知扩展字段；必要字段 / 已知结构校验后原样保留。CVE 只是 alias，可 0 / 1 / 多个，不按 CVE 合并 OSV 记录。同 ID 跨页按较新 modified 去重，比较保留亚毫秒精度；相同 modified 但内容冲突则整次失败。affected evidence 不求值；**Candidate ≠ Affected ≠ Finding**。

缓存路径为 `<AppData root>/cache/osv-v1/`，随 `--data-dir` 隔离。endpoint / rules / ecosystem / name / version 的结构化键经 SHA-256 生成安全固定文件名，SHA-256 不用于内容真实性或 package identity 判定。文件内部保存完整 QueryIdentity、endpoint、format / rules version、fetchedAt、complete 标记和 vulns，读取再次核对及解析，读写上限 20 MiB。QSaveFile 原子写；仅完整成功结果可写，完整零 Candidate 可写，失败 / 部分页不覆盖旧成功缓存。

24 小时 TTL 仅为刷新策略，不保证漏洞数据不变；支持 Fresh / Stale、Live / Cache、CacheMiss / CacheInvalid / CacheIo、仅本地缓存查询和显式清除。未来时间戳视为 stale。刷新失败仍可呈现以前完整快照，但状态明确 Failed、本次 Count 为未知，与历史快照分离。clear 仅删除受管的 64 位小写十六进制 `.json` 文件，不递归，不碰 SQLite / logs / 外来文件，跳过链接并拒绝缓存路径祖先中的 symlink / junction。没有完整离线漏洞库、LRU、100 MiB 全局配额、enterprise manager 或跨进程协调。

ProjectPage 第四页签为“漏洞匹配”。Identity 表显示状态 / reason / 包名 / 版本及来源；Candidate 表显示 OSV ID、CVE aliases、summary、modified、withdrawn。原始证据以纯文本异步生成，显示最多 65536 字符，完整记录仍保留；小窗口通过滚动容器可达全部操作。NotStarted / Working / Success / Failed / Cancelled 与 None / Live / FreshCache / StaleCache 独立；只有 Success 的完整快照显示确定的本次候选数，零结果文案为“当前查询未返回候选”。

首次实际联网前用非阻塞窗口模态确认框展示待发送 ecosystem / name / version，确认仅在当前 controller 生命周期内保留，不持久化。查询由用户明确点击；启动、Project switch、Apply、组件选择均不自动联网。不发送完整 SBOM、项目描述、UUID、bom-ref、本地路径、依赖图、SQLite、日志或其他组件；分页 token 只用于继续该查询。OSV 日志只记录固定错误代码，不记录包身份、正文、token 或 response dump。

加载与所选查询使用独立 generation；Project switch / selection / Reload / Apply success 使旧结果失效，Preview 与 Apply failure 保留旧状态。隐藏页切换再显示也会正确合并加载。查询 busy 覆盖 cache read → consent → network / parse → cache write，失效或取消不会提前释放仍运行的 worker；完成前禁用 clear，clear 期间禁止查询，避免缓存清除后被旧任务重新写回。无生产 sleep、busy wait、nested event loop、mock endpoint 或 test-only switch。

本轮 100000 Components 自动用例通过：Debug apply / read+identity+install / scroll+select 为 839 / 729 / 6 ms，GUI wakeups 63；Release 为 755 / 494 / 6 ms，GUI wakeups 50；两者 HTTP=0。用户 GUI 验收另行通过；这些性能仅为当前机器证据，不是跨机器保证。Applicability、Finding、Risk / Confidence、enrichment、bulk scan / querybatch、正式 Graphviz 业务与报告仍未实现。

# 17. 当前自动测试状态

**Phase 00—06 Regression：82/82 PASS。Phase 07 Automated Acceptance：26/26 PASS。Debug / Release 均 108/108 PASS。**

下表为 2026-09-25 按 `26.md` 重新执行的最终回归实际结果，未复制开发轮耗时。早期阶段验收保留在第 14 / 18 节。已有构建缓存正常，两次增量构建均为 no work to do；未 Clean Configure，未更换工具链，未模拟重做用户 18 项人工验收。

| 正式工程验证 | 结果 |
| --- | --- |
| Debug Build | PASS；1.45 s |
| Release Build | PASS；0.11 s |
| Debug CTest | 108/108 PASS；50.69 s |
| Release CTest | 108/108 PASS；40.32 s |
| Phase 00 Regression：Qt / C++20、SQLite、Graphviz SVG | Debug / Release 均 3/3 PASS |
| Phase 01：路径、配置、数据库、未知 schema、日志、导航、真实程序启动关闭 | Debug / Release 均 7/7 PASS |
| Phase 02：schema 初始化 / 迁移 / 回滚 / 冲突保护、项目读写 / 校验 / 排序 / 持久化、UI 和错误处理 | Debug / Release 均 12/12 PASS |
| Phase 03：版本 / 元信息 / 嵌套组件 / 依赖、错误 / UTF-8 / 安全边界 / 文件读取、预览原子性 / Project 集成 / 大预览 / 异步关闭 | Debug / Release 均 15/15 PASS |
| Phase 04：clean / 字段缺失 / 重复标识 / 空文档 / 依赖规则 / 确定性与计数 / 文档不变 / Parse 与 Quality 分离 / UI / 大输入 / 数据库不变 | Debug / Release 均 11/11 PASS |
| Phase 05：schema / 迁移 / 回滚 / FK / 原值与顺序 / replacement / 错误 / Preview / Apply / Parser Failure / 重试 / 项目 UI / 大替换 / 大 Apply / worker 生命周期 | Debug / Release 均 16/16 PASS |
| Phase 06：schema 4 / 全迁移链与回滚 / raw 保存 / capture / 原子 Apply / 一致 snapshot / exact resolution / metrics / deterministic graph / self-loop / BFS / UI / graph reuse / stale result / 大规模 / worker 生命周期 | Debug / Release 均 18/18 PASS |
| Phase 07：identity / version / parser / merge / network errors / pagination / limits / cancel / privacy / cache / controller / lifecycle / hidden page / UI / Apply integration / 100000 components / fixtures | Debug / Release 均 26/26 PASS |
| GUI smoke | PASS；真实程序隔离启动和关闭、项目与导航回归、只读组件与质量预览、正常 / 最小尺寸 / 超长字段、失败保留旧结果、10000 组件预览 / 500000 问题行及异步关闭检查通过 |
| 最终维护性检查、git diff --check | PASS |

测试源码为 `tests/Phase00SmokeTest.cpp` 至 `tests/Phase07Test.cpp`（Phase 00 文件名含 Smoke）。旧 82 项保留，新增 Phase07 的 26 项，数据驱动子用例在各注册项内执行。本阶段旧测试仅适配 ProjectPage 的 cacheDirectory 构造参数，原断言未弱化。既有迁移 / FK / 事务回滚、六表不变、原值与顺序、Preview / Apply、图解析 / BFS / 一致快照、100000 规模及 GUI / worker 回归继续通过。

Phase07 使用 FakeNetwork / ControlledReply 离线覆盖错误、分页、token-only、重复 token、跨页去重、限额、取消及最小发送数据。缓存覆盖过期 / 未来时间戳、完整零结果、坏格式 / 键、原子写和清除边界；controller 覆盖状态分离、刷新失败保留历史、stale result、隐藏页切换与销毁。测试内以确定性线程池控制复现 read / parse / write 未结束时 cancel / clear 的边界，不给生产加入延迟或钩子。项目集成用真实 Preview / Apply 和 SQLite trigger 注入失败，验证回滚保留、重试成功及结果失效。100000 Components 表格与响应验证 HTTP=0；manualFixtures 仅生成公开包 / synthetic 文件于 ignored build 目录。普通 CTest 不访问公网；`liveSmoke` 不注册 CTest，且须显式按名字调用，历史 live 结果见第 11 节。

文件选择自动测试仅在测试进程启用 AA_DontUseNativeDialogs；既有用例通过作用域恢复，Phase07 在独立测试进程初始化时设置。Phase 03 开发时自动快速开关原生框曾在 Qt Windows 平台线程出现崩溃，因此自动测试覆盖的是 Qt 控件文件框与导入入口连接，不覆盖原生框实现。生产程序仍使用原生框，其打开、取消、重新打开已由用户在 `15.md`、`17.md`、`19.md` 及 `21.md` 人工确认 PASS；尚不能据此声称所有原生框异步关闭场景均经自动验证。未用 QTimer / sleep 延迟补丁替代生命周期处理。

本轮 Debug / Release 的 Phase00—07 测试日志未发现 QWARN / QFATAL / FAIL。最终逐文件维护性与公开内容检查通过：身份规则集中，SQL 留在 repository，纯 Core 模型不依赖 Network，QObject / reply / worker 生命周期与 generation 边界清楚；无生产阻塞循环、TLS bypass、测试 endpoint、临时 debug 输出或无关重构。新增代码只含正式实现与公开 / synthetic 测试构造，运行时缓存、数据库、日志、真实 SBOM、截图和本机临时配置未纳入候选提交；staged 与待推送 commit 仍须按第 19.7 节复核。

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

- **Phase 05 Manual Acceptance：PASS**，用户在 `19.md` 亲自操作并确认全部 12 项通过；Debug GUI：**PASS**，Release GUI：**AUTOMATED ONLY**，不是 Codex 本轮重新人工操作的结果。
- Preview 不持久化、Explicit Apply、重启保持、Project 隔离、完整 Replacement 而非 Merge、Parser Failure 保留旧 persisted Components、Quality Error 明确提示且可 Apply、防双击 / 重复 Apply、Empty Replace、Project Delete Cascade 均 PASS。
- 100000 Components GUI responsiveness PASS；用户实际观察 Apply 约停留 1 秒左右完成，无明显不可接受卡死。这是当前机器的一次人工验收证据，不是长期性能保证。
- Phase 00—04 GUI regression PASS；未发现崩溃、SQLite、DLL / Qt platform plugin 错误或其他阻塞问题。

**Phase 06 Manual Acceptance：PASS。Phase 06 Debug GUI Manual Acceptance: PASS。**

用户在 `21.md` 明确确认亲自完成全部 26 项 Debug GUI 人工验收；以下为用户实际 GUI 人工验收证据，不是 Codex 本轮重新人工操作：

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

**Phase 07 Manual Acceptance：PASS。Debug GUI：18/18 PASS。** 用户在 `26.md` 明确确认“全部测试通过”，亲自完成完整 18 项验收；以下记录来自用户实际操作，不是 Codex 模拟，也不是自动测试替代。本轮无需重新自动化操作这 18 项。

- 漏洞匹配页面、Preview / Explicit Apply 分离、Current Components 正式来源；PyPI、npm、scoped npm；MissingPurl / MissingVersion / UnsupportedEcosystem / VersionConflict；epoch / local version 原值保留均 PASS。
- 首次实际在线发送确认、Candidate Found、零 Candidate 正确措辞、npm 在线查询均 PASS；实时 OSV 候选数量按当时实际结果验收通过，不写成永久保证。
- Fresh Cache、重启复用、Cancel / Working 防重入、网络失败与历史缓存分离、Stale Cache、CacheInvalid、显式清除边界均 PASS。
- Preview / Apply / Project switch / Reload 失效边界、100000 Components GUI responsiveness、无自动批量网络查询均 PASS。
- 无 crash / SQLite / TLS / DLL / Qt plugin blocker。

**Phase 07 Release GUI：AUTOMATED ONLY。** 当前仅有 Release Build / CTest 自动验收，没有新增用户 Release GUI 人工验收结论。

# 19. 当前重要技术决策

- **唯一有效长期方向**为第 9 节 Foundation → Vulnerability → Risk → Validation / Presentation；Foundation 基本完成，后续重心转向漏洞风险核心，不无限扩大基础设施。旧 Phase 07—17 安排只属 Historical / Superseded Plan。
- **大方向冻结 + 单 Phase 逐步冻结。** 长期冻结四层方向，未来数据模型、schema、repository、provider、API、service、UI、scoring formula 仅在对应 Phase 开始前正式设计和冻结。这样控制本科毕设复杂度、避免过度设计，根据真实实现结果调整后续方案，防止下一阶段建立在错误假设上，并降低 Codex 长上下文开发的错误假设风险。
- 不得因为路线中未来存在功能，就提前创建对应 database table、domain model、repository、service / provider framework、UI 或评分公式。Phase 07 已按设计与开发授权完成，OSV-first 已跑通候选链；下一轮只先设计 Version Applicability / Finding，未授权 Phase 08 开发。
- Applicability 是 Risk 前置门控；Affected 才可形成有效 Finding，Not Affected 不形成有效风险 Finding，Unknown 保留 Assessment Uncertain，不能输出确定 Risk。Risk 与 Assessment Confidence 分离，Quality 影响证据完整性及确定性而非直接增加 Risk；Risk Model 等真实 Affected Findings 与 enrichment 数据具备后再冻结，不提前设权重、公式、阈值或映射。
- 工业工具比较与论文声明遵循第 8 节：承认成熟工具的覆盖、成熟度与生态优势，不以替代为目标；将候选工作称为主要工作 / 特色设计，经验证后再作有证据范围的结论，不声称已有创新算法或普遍领先。
- **C++20 + Qt 是软件工程技术选型**，不是因为 C++ 天然比 Python 更适合漏洞分析。Qt 当前统一提供 Desktop GUI、JSON、SQLite、Concurrent 和本地处理，已支撑 SBOM Parse → Persistence → Background Analysis → GUI Presentation；Phase 07 已正式接入 Network 查询 OSV。Python 等生态在安全工具和快速原型方面更成熟。
- 继续冻结现有 C++20、Qt、CMake、SQLite、Graphviz 工具环境；Graphviz 仅已有 Phase 00 冒烟验证，未来业务能力后移到辅助展示 / 解释，不构成下一 blocker；Network 已有实际职责，Svg 仍未链接，不安装无关新工具。
- SBOM 优先 CycloneDX JSON，SPDX 留作可选扩展；匹配优先依赖软件包身份和版本证据。
- GUI、应用逻辑、领域与基础设施可作职责划分，但不强制提前建满架构层。
- 一次只做一个 Phase；每个文件须有实际职责，不创建未来模块、无用接口或 Factory / Adapter / Manager 空架构。
- 稳定代码不无故重构，自动测试不能替代用户 GUI 人工验收。
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
- 真实 SBOM 属于 PRIVATE RUNTIME DATA；Phase 03—06 验收使用 synthetic 数据；Phase 07 自动验收使用 synthetic 数据，人工 / live 验收仅使用已审查公开包与 synthetic 数据。人工 JSON、数据库、日志、截图和构建产物均不纳入 Git；Phase 06 封版 staged 内容及待推送 commit 已通过 19.7 隐私审查。本轮 Phase 07 completion commit 及未来提交仍分别重新审查候选文件、staged diff 和待推送 commit，历史 PASS 不代替当前审查。

**AI 分工：** ChatGPT 负责规划、Phase、Prompt、方案、审查及验收设计；Codex 负责读取真实工程、实现、编译、自动测试和 Git 检查；用户负责 GUI 操作、人工验收和决定是否进入下一 Phase。

**开发证据职责：** GitHub → committed source / history truth，用于核对已提交源码、commit 与 tag；Read-Only MCP → local development state / evidence，补充 GitHub 不可见的本地状态与受限证据；PROJECT-HANDOFF → semantic project state / decisions / roadmap，仍是唯一长期项目语义交接文档。MCP 不构成第二套业务真相，其输出不自动覆盖实际运行结果、真实代码或 Git；历史 LastTest.log 不能单独证明当前 HEAD 已测试通过，授权元数据访问不代表 ChatGPT 获得完整本地文件访问权。

ChatGPT 已真实连接 Read-Only MCP。标准业务 Phase 可在开始前核验 baseline、Codex 开发完成后交叉核对本地 Git / validation evidence、最终封版后核验本地最终状态三个节点使用 MCP；它只是证据源，不新增审批层或多轮循环审计，目标是减少信息中转。

**长期阶段闭环：** 设计 → 设计审查 / 冻结 → Codex 开发 → Build / 自动测试 → ChatGPT 审查 → 用户 GUI 人工验收 → 最终封版 → 更新 PROJECT-HANDOFF → 下一阶段设计；MCP 只增强 baseline 与 evidence verification，不替代 Codex、用户 GUI 验收或本文，不跨阶段预建完整系统。

**具体执行与封版顺序：** 明确并设计当前 Phase → 读取真实工程 → 实现 → Build / 自动 Test → 用户人工验收 → 确认 PASS → 更新本文并完成最终回归、维护性及隐私审查 → 按授权 commit / push / tag → 核验远端与工作区 → 报告完成。本文随封版同步维护，Git 结果必须实查后报告；必要修正如实更新，不以预期代替完成。个别阶段有特殊顺序时，以具体授权为准。Phase 07 已按 `26.md` 封版；本轮 `30.md` 仅授权辅助 MCP 过时状态最小修正及必要回归、相关文档更新、正式仓库单一 Handoff docs commit 与 main 推送核验，不重跑正式 C++ 回归、不创建业务 tag，完成后停止。

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
- **当前忽略基线：** 已覆盖运行时 SQLite 数据库及 journal / WAL / SHM 伴随文件、明确的私有配置文件名与 `config/local.*` / `config/private.*`；允许安全的 `.env.example`，不整体忽略 `.sql`、`.ini`、`.json`、`.svg`、`.csv` 或 `.pdf`，以保留公开模板、synthetic fixture 和文档资源。Phase 07 增加 `**/cache/osv-v1/`，build 目录及其中 manual-data / SBOM / response / screenshot / logs 继续 ignored。后续真实导入及输出目录按实际结构隔离。
- **发现待提交敏感内容：** 停止该文件的 staging / commit；优先将敏感内容移出 tracked 文件或替换为空值／placeholder / example，必要时在获准范围内补充 `.gitignore`，保留真实用户数据供本地使用。不能以“马上会删”或“还没 push”为理由提交。
- **已进入历史的秘密：** 立即停止继续传播并报告用户，判断凭据是否真实有效，优先安排 rotation / revoke，检查 Git 历史及公开暴露范围，再决定历史清理；删除当前文件并再提交并不能清除历史泄露。未经用户明确授权，不得 force push、rewrite history、使用 BFG / filter-repo 或删除远程历史；历史重写按 19.4 停止并询问。

# 20. 当前待定事项

- **下一核心设计问题：Version Applicability / Finding。** 基于已实现的 QueryIdentity、OSV Candidate、affected evidence、ecosystem 与 exact version 设计，不从 Candidate 直接计算 Risk。
- **后续证据关联：** Current Components 与未持久化 Quality report 的关联方式、未来 Finding 的证据及持久化必要性须另行设计；不预建 schema / Repository / Provider 框架。
- **后续 Applicability：** Phase 07 完成后依据真实 payload、ecosystem、version range 和 Component 数据，设计 Affected / Not Affected / Unknown、证据与 Finding 边界；不提前承诺所有生态、backport 或 vendor patch 支持。
- **后续 Risk：** 在真实 Affected Findings 和所需 enrichment 数据基础上决定第一版风险模型、Confidence 规则、解释、缺失证据处理与验证；当前不冻结公式、权重、阈值或映射。
- **实验：** Validation Experiment 的项目、生态、约 30–50 个 component-version 样本及判断依据；最终数据集与实验方法尚未冻结，结果声明须受证据范围约束。
- NVD / EPSS / KEV 的实际 enrichment 需求、NVD API Key 历史 PENDING 状态与相应数据获取策略在未来对应阶段处理，不阻塞已完成的 OSV-first MVP。
- CycloneDX 格式和质量规则扩展、跨扫描身份、快照 / 比较 / 报告必要范围、Graphviz 辅助展示、部署、全新电脑验证及 CI 按后续实际需求决定；当前共同字段子集仍为 1.4 / 1.5 / 1.6，质量规则仍为第 16 节已实现的 16 条。

以上逐阶段决策，不是当前待实现清单；Graphviz 不作为下一 blocker，Dashboard / Report 不提升为当前优先目标。

# 21. 当前已知问题 / 风险

- **Phase 07 Blocker = None。** 最终双配置自动回归和用户 Debug 18/18 人工验收均 PASS；维护性、隐私、diff 及 Git 封版按本轮授权核验，精确 Git 结果见最终报告。未发现需新增功能或修改 schema 的封版问题。
- optional Vulkan Headers 缺失仍为 non-blocking warning，不影响当前 Qt Widgets 工程；无需据此安装额外组件。
- 本轮核对未发现正式目录冲突或错误 origin；Phase 00—06 tag 保持原对象与 target，本轮 Phase 07 main / tag 独立核验。Qt Creator 本地配置和构建目录继续忽略并保留。
- Windows 原生文件框已有用户人工验收 PASS；自动快速关闭场景的测试限制见第 17 节，后续若修改文件框生命周期，应针对原生框重新验证。
- 项目管理及 SBOM 导入 / 解析 / 只读预览 / 规则化质量诊断 / 当前组件与原始依赖持久化 / 依赖关系分析已实现；Component Identity 与 OSV 候选匹配已实现；适用性、Finding、风险及正式 Graphviz 业务仍未实现，不要把候选链与完整系统混同。
- 真实 SBOM dependency 声明可能不完整；可靠依赖分析不等于 runtime reachability，空结果不能证明没有依赖或漏洞。100000 规模性能只是当前机器证据；Phase 05 组件列表仍在 GUI 线程读取，真实长字段及负载变化须重新评估。
- 当前仅支持 PyPI/npm、仅 selected component 查询；OSV coverage / external availability、身份歧义及后续版本判断仍有限制。缓存仅为历史外部证据，未实现全局容量治理或跨进程协调；身份不明、查询失败或无候选均不能当成“安全”的证明。应在对应阶段验证实际支持范围和错误 / 不确定状态。
- Quality evidence incomplete 可能使身份、版本适用性和依赖上下文缺少可靠依据；当前质量报告未持久化，不能假定历史 Current Components 自动带有完整报告。后续 risk model lack of evidence 可能造成伪精确结论，须依真实 Findings 和 enrichment 冻结及验证模型。
- 真实大 SBOM 差异、跨导入身份和 scope creep 仍有风险；以单 Phase 逐步冻结控制范围，不无限扩展生态、同步平台、Provider 框架或基础设施。
- 父目录旧资料包含重复版本；后续以本文当前状态和更高优先级实查证据为准。

# 22. 下一步

**PHASE 07 COMPLETE。READY FOR NEXT PHASE DESIGN。**

下一核心方向为 **Vulnerability continuation — Version Applicability / Finding design**。依据已有 QueryIdentity、真实 OSV Candidate / affected evidence 与支持生态设计；具体 Phase 名称、范围、算法与数据模型等下一轮正式设计再冻结。本轮封版后停止，不开始 Phase 08，不创建类 / schema，不执行 applicability 或风险计算，不新增 NVD / EPSS / KEV。

**Auxiliary 已收口：** Read-Only MCP 真实 ChatGPT 连接、调用及 restart / reconnect 已 PASS，按需手动运行。MCP failure / offline 或 Tunnel 临时不可用不得阻塞产品开发，仍可通过 GitHub + 最新 PROJECT-HANDOFF + Codex 继续；下一正式业务方向仍为 Version Applicability / Finding design。本轮收口后停止，等待下一业务阶段设计授权。

# 23. 新 Codex 接手规则

1. 先阅读本文及当前阶段具体指令，再读取真实代码、Git 和必要的 GitHub 状态；按 19.2 定位调用链、状态来源、已有实现和扩展点，不凭历史文字假定实现存在。MCP 与 Tunnel 运行且连接可用时，新 ChatGPT 对话可优先用 get_project_status、get_git_status、get_handoff_outline、get_handoff_section 读取 baseline 与必要章节，无需默认上传全文；不可用时上传最新 PROJECT-HANDOFF 作为标准 fallback，项目不依赖 MCP 才能继续。
2. 如实区分已实现、已验证和规划；发现冲突按本文顶部真实性优先级处理，并更新相关状态。
3. 延续已有简单合理结构，不因 Prompt 出现类名或目录建议就机械创建；只执行授权阶段，同时考虑功能正确性、回归风险和后续可维护性。
4. Bug 优先修根因，避免 workaround 链；允许有测试保护、与当前需求直接相关的小型重构。大范围架构调整及 19.4 所列高风险问题先暂停，提供方案并获得用户确认。
5. 实现后完成适当构建、测试及 19.6 的 diff／维护性检查，清楚记录结果与限制；阶段通过必须包含用户要求的人工验收。
6. 未经允许不得 `reset --hard`、`clean -fd`、force push、rebase、删除 branch / tag、删除用户文件或重写 Git 历史；发现已有错误 remote 先报告，不自行覆盖。身份配置优先仓库级，冻结环境不无故变更。
7. 完成阶段后按 19.6 就地更新本文，不另造动态管理文档，不追加重复状态或全过程日志。
8. 长期工程规则持续生效；Phase 07 已按 `26.md` 封版，`phase-07-complete` 固定指向 `ad8c75274a2a8c4db275ab099f949a7c47fcecdb`，Phase 00—06 tags 同样不移动。本轮 `30.md` 的正式仓库修改仅为本文及普通 docs commit / main 推送；辅助 MCP 修正留在独立目录，不创建 Phase 07.5 tag、常驻服务、公网 endpoint 或 Phase 08 实现。精确新提交与 tag 对象按第 5 节实查。
9. 任何 git add / commit / push 前均须按 19.7 审查候选／staged 文件及 privacy / sensitive information，push 同时检查待推送 commits；不得将真实 runtime database、SBOM、日志、用户数据、私有配置、API Key、token 或 credential 加入 Git。`12.md` / `13.md` 的隐私规则及 `.gitignore` 维护已独立提交并推送，其规则继续适用于本次及后续封版。
10. 当前四层路线是唯一有效长期方向；旧 Phase 07—17 安排只属 Historical / Superseded Plan，不得据其自行启动开发。Phase 07 的 Component Identity + OSV-first Vulnerability Matching MVP 已 COMPLETE；下一轮先做 Vulnerability continuation 的需求与技术设计，不直接开发。
11. 遵循“大方向冻结 + 单 Phase 逐步冻结”；未来详细 Phase 设计须在该阶段开始前、根据已完成成果逐步冻结，不提前创建未来表、领域模型、Repository、Service / Provider 框架、UI 或评分公式。Candidate 不等于 Affected Finding，Applicability gate、Risk / Confidence 分离和 Quality 证据原则持续生效；Risk Model 尚未冻结。
