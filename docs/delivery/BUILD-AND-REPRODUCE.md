# 构建与复现

## 依赖边界

工程基线：Windows 11 x64；C++20；Qt 6.11.2 MinGW 64-bit；MinGW GCC 13.1.0；
CMake 3.30.5；Ninja 1.12.1。Qt Creator 20.0.2 是可选 IDE，不是构建要求。
Full Test 还需要 Qt Test 和 Graphviz `dot`（已验证基线 16.1.0）。
Python 3.12+ 只用于交付哈希工具；部署脚本使用 PowerShell。
应用运行只需要包内 Qt/MinGW DLL 和插件，以及 Windows 系统运行能力。
App Runtime 不需要 Graphviz；不得因 Phase00 测试依赖将它打入运行包。

## 来自精确提交的新目录

使用候选提交创建独立本地 clone 或 detached worktree；以生成的复现记录中的 SHA 为准。
不要复制原工作目录的 build、ignored 或 untracked 文件。Windows clone 时显式设置
`git clone --no-checkout -c core.autocrlf=false <repository> clean-source`，再 checkout
精确候选 SHA；保留 Git blob 的 LF 字节，避免全局 autocrlf 改写冻结 JSON/sidecar 和 demo 哈希。
Windows 未启用长路径时必须选浅层 source/build 目录；Qt 大资源 target 的对象路径较长。
预检发现深层 build-final-delivery 下的编译目录会触发此限制，证据目录仍可保留在那里。
下面变量由实际安装位置填写，不修改系统 PATH：

```powershell
$qtKit = 'X:/Qt/6.11.2/mingw_64'
$compilerBin = 'X:/Qt/Tools/mingw1310_64/bin'
$cmake = 'X:/Qt/Tools/CMake_64/bin/cmake.exe'
$ninja = 'X:/Qt/Tools/Ninja/ninja.exe'
$env:PATH = "$qtKit/bin;$compilerBin;" + $env:PATH
& $cmake -S ./clean-source -B ./fresh-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$qtKit" "-DCMAKE_CXX_COMPILER=$compilerBin/g++.exe" "-DCMAKE_MAKE_PROGRAM=$ninja" -DBUILD_TESTING=ON -DBUILD_DELIVERY_SUPPORT=ON
& $cmake --build ./fresh-debug --parallel 4
# Release 使用另一个 fresh-release 目录，并设置 -DCMAKE_BUILD_TYPE=Release。
```

每条命令必须检查退出码。Graphviz 不在 PATH 时明确传入 `-DGRAPHVIZ_DOT_EXECUTABLE=...`。
测试含 Widgets，需要可用的 Windows 桌面会话。不要以无显示环境冒充 GUI 验收。

```powershell
& 'X:/Qt/Tools/CMake_64/bin/ctest.exe' --test-dir ./fresh-debug --output-on-failure --parallel 2
& 'X:/Qt/Tools/CMake_64/bin/ctest.exe' --test-dir ./fresh-release --output-on-failure --parallel 2
& ./fresh-debug/ValidationReplay.exe ./clean-source/validation ./debug-result.json
& ./fresh-release/ValidationReplay.exe ./clean-source/validation ./release-result.json
```

保留 Phase00–13 的 271 个注册名，不禁用或降低断言。Phase14 演示数据用例验证
真实解析→质量→持久化路径。测试数量、耗时与结果以生成证据为准。
既有 scale/limits 用例覆盖 100000 组件、50 MiB 输入边界与 10000 CVE 报告，
不把这些本机观察写成通用性能保证。

Replay 必须记录实际候选 SHA、dirty=false 和 sourceInputsSHA256。
源码指纹算法在 CMake 中：按路径排序，逐文件 exact SHA256 后拼接 `path:hash\n` 再哈希。
该指纹针对 CMake 声明的源码输入；交付脚本/文档/演示文件另外用完整 Git commit
及逐文件交付 manifest 绑定，不把它误称为整仓库字节指纹。
六个冻结输入及全部科学结果必须与 Phase13 基线相同，允许构建/运行元数据变化。

## 部署

在新目录执行 `tools/deploy.ps1`，传入 Release build、Qt kit、compiler root、
官方 Qt Base 6.11.2 源码压缩包和不存在的 Destination。
源码压缩包须先与官方 `.sha256` 核对，并用 `-QtSourceSHA256` 显式传入该官方值。脚本调用该 kit 的 windeployqt，
使用 Schannel，写入相对路径 qt.conf，附官方许可及对应源码材料。
不引入安装器；重复部署必须换新目录，不能覆盖已验收 app。

在受限 PATH（Windows 系统目录）及清空 Qt plugin 环境变量的子进程内，
以独立 `--data-dir` 启动 app；检查实际加载模块均来自 app 或 Windows。
`DeploymentProbe` 是独立支持工具：真实 AppDatabase 初始化 schema 4，
报告 SSL backend/protocols/HTTPS scheme，并执行经过证书验证的 HTTPS 握手。
可对 localhost TLS server 传入 `--ca-file`；CA 只作用于该请求，不写系统信任库，
不允许对公网 URL 使用额外测试 CA，不忽略 SSL 错误。输出必须是新文件。
能力测试可独立于公网完成；Provider live smoke 另行记录，不能互相替代。

最终 app 冻结后，通过 `delivery_inventory.py` 比较逐路径、大小和 SHA256，
Delivery 内 app 必须 exact copy。Manifest 放在 app 外避免自引用。
证据/交付 ZIP 的 SHA256 用外部 sidecar 记录，内部 manifest 不包含自身或外层 ZIP 的哈希。
任何必需认证门失败后停止，不修改 tracked 文件继续使用原候选 SHA。

许可证及 Qt 第三方来源见 [第三方声明](THIRD-PARTY-NOTICES.md)。
开发环境隔离不能冒称另一台全新 Windows 验证。
