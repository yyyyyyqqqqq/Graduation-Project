# 用户手册

本系统使用 C++20 / Qt Widgets 实现软件供应链分析的本科毕业设计。
1.0.0 表示冻结范围的稳定交付版本，不表示工业级 SCA 平台。
流程是：项目 → SBOM → 质量诊断 → 显式应用 → 当前组件/依赖 → 所选组件漏洞候选
→ 适用性 → Finding → 风险证据 → 利用信号优先级/证据支持 → 单 Finding 报告。

## 启动与数据

在 Windows x64 解压完整交付目录，运行 `app/SupplyChainRiskAssessment.exe`，不要只复制 EXE。
默认数据保存在 `%APPDATA%/GraduationProject/SupplyChainRiskAssessment`。
数据库在 `data/`，日志在 `logs/`，配置为 `settings.ini`，外部证据在 `cache/`。
可从 PowerShell 指定隔离目录：

```powershell
& .\app\SupplyChainRiskAssessment.exe --data-dir "$env:TEMP\GraduationProject-Defense"
```

参数必须是非空绝对路径。每次答辩可选择新的目录，避免把个人数据带入演示。
应用不需要 Qt Creator、Python 或 Graphviz 才能运行。具体支持平台及包状态见随包验证记录。

## 项目、导入与质量

在“项目”页创建名称与可选描述，选中项目，再点击导入 SBOM。
支持 CycloneDX JSON 1.4、1.5、1.6 的共同字段子集，不是完整 Schema 校验器。
可使用随包 `demo/defense-sbom.json`：3 个普通组件、1 个根组件、4 个依赖条目。
组件预览与质量诊断均只读。质量页按 Error / Warning / Info 列出问题及位置。
“当前诊断规则未发现问题”不意味着 SBOM 完整、真实或没有漏洞。

导入成功只产生预览。点击“应用”才把整个当前组件、原始依赖和捕获状态原子替换到 SQLite。
这不是合并；旧组件行 ID 会重新生成。Quality Error 仍可保存原始输入。
取消或解析失败保留此前成功预览；应用失败保留旧数据库状态。
应用期间等待任务完成，避免强制结束进程。关闭预览后未应用的内容不保存。

## 当前组件与依赖

“当前组件”显示 SQLite 已保存的数据，根组件单独标明。
“依赖关系”显示原始声明、引用解析状态以及直接/传递的依赖和被依赖关系。
引用按原始 bom-ref 精确解析，重复引用可能为 Ambiguous；缺失或未知引用不猜测绑定。
Not Captured 与已捕获但为空不同。传递查询使用最短深度，环不会无限展开。
依赖路径反映声明关系，不能证明运行时可达性或依赖声明完整性。

## 漏洞与适用性

进入“漏洞匹配”，选择已应用的组件。身份解析只支持严格 PURL 子集中的 PyPI/npm
（含 scoped npm）。缺少身份、版本冲突及不支持生态会给出诊断，不能解释为安全。
在线查询由用户点击触发。首次确认显示发送的生态、包名及版本；只发送所选组件信息。
可以选择仅本地缓存，缓存有 Fresh/Stale 区分；失败/取消不能当成零漏洞。

OSV 返回的是 Candidate。系统对明确版本及严格 SEMVER 范围作本地判断，
仅 Published Affected 形成 Finding。Unknown、Withdrawn/Excluded 和 Finding 分开显示。
OSV 版本查询与本地未命中冲突会保留 Unknown / ProviderEvidenceConflict。
原始证据及技术详情用于追溯；不支持 PEP 440/ECOSYSTEM 和 GIT 范围解释。

## 风险证据与解释

选择 confirmed Finding 后加载证据。FIRST EPSS 查询发送经确认的 CVE；
CISA KEV 获取完整公开目录后本地匹配，不逐 CVE 向 CISA 查询。
可优先缓存、刷新或仅缓存。缺失/失败/NotScored 保持可见，不能填成零。
当前结论、为什么、驱动、关键证据和边界位于“结果解释”；详细原始字段位于技术页签。

Fresh KEV Listed 优先，其次按最大 Fresh EPSS percentile 与研究阈值 0.90 比较；
没有 Fresh 驱动时为证据不足。Support 为 Complete / Partial / Insufficient，
说明支持该规则判断的证据情况，不是预测置信度。Percentile 不是利用概率。
Below 不代表 Safe/Low Risk；NotListed 不代表从未利用。
24 小时为本地刷新政策，到期会重新评估已有证据，不会自动联网。
严重性、依赖及不可用的质量证据仅为上下文，不生成综合风险分数。

## 方法验证与离线答辩

打开“方法验证”即从内嵌冻结输入离线重放。40 个真实样本和 54 个合成规则用例
各有用途：前者是分层目的性验证集，后者检查边界，不混合统计。
0.85 / 0.90 / 0.95 选择只改变实验展示，不修改生产 0.90。
该实验不能推导全生态识别准确率、预测准确率或最佳阈值。
离线核心答辩使用本地 Foundation、方法验证、共享解释及已生成样例报告；
实时服务不可用时直接走此路线，见 [答辩脚本](DEFENSE-DEMO-SCRIPT.md)。

## 报告

在完整当前 Finding 分析可导出时点击报告导出，选择 HTML 文件。
报告来自保存框打开前捕获的单 Finding 快照，保存失败不会直接覆盖旧目标。
取消保存不会生成报告。Support=Partial/Insufficient 也可如实导出；
失败或取消操作保留的旧结果不能绕过导出门控。
HTML 可离线打开或用浏览器打印；本软件没有原生 PDF 渲染器。
三个时间分别表示 Profile 生成、Assessment 评估、报告上下文捕获。
以后打开 HTML 不会刷新证据；报告不代表项目级全量扫描或安全认证。

## 隐私与常见错误

真实 SBOM、数据库、缓存、日志及导出报告是私有运行数据，分享前检查。
项目描述按合法用户内容保留，即使包含路径样式或 token 样式文本；HTML 转义防止脚本执行，
但不等同于自动隐私脱敏。不要把真实秘密写进项目描述。
遇到 DLL/plugin 缺失应重新解压完整包；数据目录错误应选可写绝对路径；
未知数据库版本不能通过删除真实数据库解决。网络错误可使用缓存或离线验证。
详细边界见 [已知限制](KNOWN-LIMITATIONS.md)。
