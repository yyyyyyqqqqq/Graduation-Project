param(
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [Parameter(Mandatory=$true)][string]$QtDirectory,
    [Parameter(Mandatory=$true)][string]$CompilerDirectory,
    [Parameter(Mandatory=$true)][string]$QtSourceArchive,
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-fA-F]{64}$')][string]$QtSourceSHA256,
    [Parameter(Mandatory=$true)][string]$Destination
)
$ErrorActionPreference = 'Stop'
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path
$qtRoot = (Resolve-Path -LiteralPath $QtDirectory).Path
$compilerRoot = (Resolve-Path -LiteralPath $CompilerDirectory).Path
$sourceArchive = (Resolve-Path -LiteralPath $QtSourceArchive).Path
if ((Get-FileHash -LiteralPath $sourceArchive -Algorithm SHA256).Hash -ne $QtSourceSHA256) {
    throw 'Qt corresponding source does not match the independently obtained official SHA256.'
}
$appRoot = [IO.Path]::GetFullPath($Destination)
if (Test-Path -LiteralPath $appRoot) { throw 'Destination must not exist; never overwrite a tested artifact.' }
foreach ($file in @('SupplyChainRiskAssessment.exe','DeploymentProbe.exe')) {
    if (!(Test-Path -LiteralPath (Join-Path $buildRoot $file) -PathType Leaf)) { throw "Missing build output: $file" }
}
New-Item -ItemType Directory -Path $appRoot | Out-Null
foreach ($file in @('SupplyChainRiskAssessment.exe','DeploymentProbe.exe')) {
    Copy-Item -LiteralPath (Join-Path $buildRoot $file) -Destination $appRoot
}
$savedPath = $env:PATH
try {
    $env:PATH = "$qtRoot\bin;$compilerRoot\bin;$env:SystemRoot\System32;$env:SystemRoot"
    # Widgets/GDI application: no QML, SVG, software OpenGL, unrelated SQL drivers or OpenSSL.
    $arguments = @('--release','--no-patchqt','--compiler-runtime','--no-translations',
        '--no-opengl-sw','--no-system-d3d-compiler','--no-system-dxc-compiler','--no-ffmpeg',
        '--skip-plugin-types','generic,iconengines,imageformats,qmltooling',
        '--exclude-plugins','qopensslbackend,qcertonlybackend,qdirect2d,qminimal,qoffscreen,qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql',
        '--dir',$appRoot,(Join-Path $appRoot 'SupplyChainRiskAssessment.exe'),(Join-Path $appRoot 'DeploymentProbe.exe'))
    & (Join-Path $qtRoot 'bin/windeployqt.exe') @arguments
    if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed; partial destination is retained for diagnosis.' }
} finally { $env:PATH = $savedPath }
[IO.File]::WriteAllText((Join-Path $appRoot 'qt.conf'), "[Paths]`nPrefix=.`nPlugins=.`n", [Text.UTF8Encoding]::new($false))
$licenseRoot = Join-Path $appRoot 'licenses'
New-Item -ItemType Directory -Path $licenseRoot | Out-Null
$installationRoot = Split-Path (Split-Path $qtRoot -Parent) -Parent
Copy-Item -LiteralPath (Join-Path $installationRoot 'Licenses/LICENSE') -Destination (Join-Path $licenseRoot 'Qt-LGPL-GPL.txt')
Copy-Item -LiteralPath $sourceArchive -Destination (Join-Path $licenseRoot 'qtbase-everywhere-src-6.11.2.tar.xz')
foreach ($pair in @(@('gcc/COPYING3','GCC-GPLv3.txt'),@('gcc/COPYING.RUNTIME','GCC-Runtime-Exception.txt'),
    @('winpthreads/COPYING','winpthreads.txt'),@('mingw-w64/COPYING.MinGW-w64-runtime.txt','MinGW-runtime.txt'))) {
    Copy-Item -LiteralPath (Join-Path $compilerRoot ('licenses/'+$pair[0])) -Destination (Join-Path $licenseRoot $pair[1])
}
$docRoot = Join-Path $installationRoot 'Docs/Qt-6.11.2'
$attributions = Join-Path $licenseRoot 'qt-attributions'
New-Item -ItemType Directory -Path $attributions | Out-Null
# Original installed notices, verbatim. Broader platform attributions within shipped modules
# may appear; this is not a claim that every listed optional backend was compiled in.
foreach ($module in @('qtcore','qtgui','qtnetwork','qtsql','qtwidgets','qtconcurrent')) {
    Get-ChildItem -LiteralPath (Join-Path $docRoot $module) -Filter '*attribution*.html' -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $attributions
    }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../docs/delivery/THIRD-PARTY-NOTICES.md') -Destination (Join-Path $appRoot 'THIRD-PARTY-NOTICES.md')
Write-Output 'Deployment assembled; still requires dependency, license, isolation and runtime gates.'
