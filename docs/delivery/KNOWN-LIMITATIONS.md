# 稳定产品边界

- 输入仅 CycloneDX JSON 1.4/1.5/1.6 共同字段子集，不是完整 Schema validator，也未接入 SPDX。
- 包身份仅 PyPI/npm（含 scoped npm）严格 PURL 子集；没有 name/type/bom-ref fallback。
- 只分析所选组件；没有项目级扫描、排名、漏洞总数或项目级报告。
- Applicability 支持 exact versions 与严格 SEMVER；ECOSYSTEM/PEP 440、GIT 等保留 Unknown。
- Candidate 不等于 Finding。只有 Published Affected 进入风险证据链；缺失、冲突或失败不代表安全。
- OSV/FIRST EPSS/CISA KEV 受外部可用性和覆盖范围影响；缓存不是离线完整漏洞库。
- SQLite 仅持久化项目、当前组件、原始依赖和捕获状态；Finding/Profile/Assessment/Quality/Report 不持久化。
- Preview/Quality 是导入会话内容，不自动关联已持久化 Current State。Quality unavailable 不降低规则 Support。
- 利用信号优先级是 Rules v1 子模型，没有 numeric Risk Score、Risk Level、预测置信度或 CVSS 计算器。
- 0.90 是研究 percentile 阈值，不是 90% 利用概率；Support 不是 confidence。
- Stale 为历史上下文；future timestamp 为 Ineligible。24h TTL 不保证数据真实或稳定。
- Dependency Path 不证明 runtime reachability；引用全部可解析不证明依赖声明完整。
- 导出仅单 Finding 静态 UTF-8 HTML，无 PDF 引擎、报告历史、报告持久化或强制退出恢复。
- 报告合法用户文字可能含用户自行输入的敏感信息；HTML escape 不等于自动脱敏。
- Validation 的 40 个真实样本为分层目的性验证集，54 个合成用例另计；不作通用 benchmark、预测准确率或最佳阈值声明。
- 大输入测试是特定输入和机器证据；当前组件列表仍在 GUI 线程读取，超长字段/负载变化需要复评。
- 无 NVD、新 provider、Graphviz 产品绘图、Dashboard、批量分析、云服务、整改工作流或安装器。

本文不记录动态验收结果。具体包是否通过 TLS、隔离、全新机器或最终审查，见生成的工程证据。
