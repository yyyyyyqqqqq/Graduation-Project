# 论文证据索引

路径均相对交付目录根。下表只定义证据位置，不预设结果；阅读对应文件中的
actual status、timestamp、Candidate SHA、operator 和限制。历史 tag 为不可变源码参考。

| 论文主张/工程事实 | 稳定证据引用 | 解释范围 |
| --- | --- | --- |
| C++20/Qt 分层与构建复现 | `evidence/build/clean-reproduction.json` | 精确 candidate、工具链、Debug/Release 和源码指纹 |
| 自动回归与大输入边界 | `evidence/build/test-regression.json` | 271 历史名称、实际新增数量、failures/disabled；不是跨机器性能承诺 |
| 冻结输入和 Rules v1 实验 | `evidence/validation/replay-comparison.json` | 40 实样/54 合成，科学字段全等；不是预测准确率 |
| 动态库部署及 SQLite | `evidence/deployment/runtime-capability.json` | deployed probe 实际 QSQLITE/schema 4 |
| TLS 能力 | `evidence/deployment/tls-capability.json` | 验证过的 TLS 握手和 backend；与 provider 可用性分离 |
| 第三方再分发材料 | `evidence/deployment/redistribution-audit.json` | 实际 ship 文件、来源、许可证和对应源码 |
| 开发环境隔离与加载模块 | `evidence/deployment/isolation.json` | isolated PATH/数据目录、模块路径归类；不替代 fresh-machine |
| 集成/离线答辩 | `evidence/acceptance/acceptance-summary.json` | Automated/Codex-operated/User-reserved 分开 |
| 演示输入来源 | `demo/manifest.json` | synthetic topology、公开包身份、exact SHA256 |
| 界面与解释 | `evidence/screenshots/manifest.json` | deployed 1.0.0、截图范围、隐私检查 |
| 单 Finding 导出 | `evidence/sample-report/manifest.json` | app artifact 来源、输入、时间、未手改 HTML、hash |
| 完整应用文件身份 | `manifests/application-deployment.json` | app 逐文件大小与 SHA256，不引用未创建的 tag |
| 证据与交付完整性 | `manifests/evidence.json`、`manifests/final-delivery.json` | 无自引用的清单；外层 ZIP 用 sidecar |
| 历史工程阶段 | `phase-00-complete` 至 `phase-13-complete` | 正式 Git annotated tags；历史验收事实见 PROJECT-HANDOFF |

引用论文时说明实验单位、数据分层、范围和不确定性，不把历史/合成截图当成线上实时结果。
尚未执行的用户操作应保持 PENDING；缺少证据文件必须在装配路径审计中阻断。
