<#
.SYNOPSIS
    采集 doc/ARCHITECTURE_EVOLUTION_PLAN.md 附录 B 的构建指标。

.DESCRIPTION
    测四项：
      1. 全量构建耗时（删掉整棵构建树，重新配置后计时构建这一步）
      2. 改 DmArc.cpp 后的增量构建耗时（叶子 .cpp，衡量单文件编译 + 链接成本）
      3. 改 GuiDocumentView.h 后的增量构建耗时（被 121 个文件包含的重头文件，
         衡量阶段 1 头文件瘦身的收益）
      4. 改 Datamodel.h 后的增量构建耗时（被 27 个文件直接包含的中心枚举，
         衡量阶段 4 去中心化的收益）

    「改」的方式是给文件追加一行注释再撤销，不改变任何语义，
    只为触发时间戳变化。脚本结束时文件内容与开始时逐字节相同。

    每个阶段结束后重跑一次，把结果追加到附录 B 的表里。

.PARAMETER BuildDir
    构建目录，默认 build/Release

.PARAMETER Config
    构建配置，默认 Release

.PARAMETER SkipFull
    跳过全量构建（最慢的一项），只测三项增量

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/measure_build.ps1
    powershell -ExecutionPolicy Bypass -File tools/measure_build.ps1 -SkipFull
#>

param(
    [string]$BuildDir = "build/Release",
    [string]$Config = "Release",
    [switch]$SkipFull
)

$ErrorActionPreference = "Stop"

$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo

# cmake 与 .NET 的相对路径解析基准不一致，统一成绝对路径
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $repo $BuildDir
}

$targets = @(
    @{ Name = "DmArc.cpp";          Path = "YiCAD/src/kernel/builder_model/DmArc.cpp" },
    @{ Name = "GuiDocumentView.h";  Path = "YiCAD/src/kernel/gui/GuiDocumentView.h" },
    @{ Name = "Datamodel.h";        Path = "YiCAD/src/kernel/math/Datamodel.h" }
)

function Invoke-Build {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    # `-- -m`：MSBuild 的解决方案级并行。阶段 3 把 YiCadCore 拆成
    # YiCadMath -> YiCadModel -> YiCadPersistence -> YiCadCore 一条依赖链后，
    # 没有这个参数时 Visual Studio 生成器按项目依赖顺序逐个构建，各层内部的
    # /MP 并行度用不满，全量构建反而比阶段 1 的单一 OBJECT 库更慢
    # （实测无 -m 时 189.7 秒，比阶段 1 的 129.5 秒还慢）。CI 的构建步骤
    # 同步加了这个参数，见 .github/workflows/build.yml。
    & cmake --build $BuildDir --config $Config -- -m 2>&1 | Out-Null
    $sw.Stop()
    if ($LASTEXITCODE -ne 0) {
        throw "构建失败，退出码 $LASTEXITCODE"
    }
    return $sw.Elapsed.TotalSeconds
}

$results = [ordered]@{}

Write-Host "先做一次构建，确保起点是最新状态..." -ForegroundColor Cyan
$null = Invoke-Build

if (-not $SkipFull) {
    Write-Host ""
    Write-Host "[1/4] 全量构建（删掉构建树后重新配置再构建）..." -ForegroundColor Cyan
    # 整棵构建树删掉重来，只计时构建那一步，不含配置。
    #
    # 试过两种更省事的做法，都不对：
    #   - `cmake --build ... --target clean`：对 Visual Studio 生成器映射到
    #     MSBuild 的 Clean，清不干净 .obj 与 PCH，测出来是几秒钟的假数字。
    #   - 删掉全部 *.dir 目录：<target>.dir 里除了 .obj，还有 CMake 在配置期
    #     生成的 cmake_pch.hxx；CMakeFiles/*_autogen.dir 里是 AutogenInfo.json。
    #     删掉这些之后重新配置也不会补回 PCH 头，构建直接失败。
    # 从现有缓存里取回配置参数，删完再原样重放。
    # 不用 `cmake --preset`：仓库里 CMakeUserPresets.json 同时 include 了
    # 多份 conan 生成的 preset，presets 名字重复，cmake 会直接报
    # "Duplicate presets" 拒绝加载。
    $cachePath = Join-Path $BuildDir "CMakeCache.txt"
    if (-not (Test-Path $cachePath)) {
        throw "找不到 $cachePath。先正常配置一次再跑本脚本。"
    }
    $cache = Get-Content $cachePath

    function Get-CacheValue($name) {
        $line = $cache | Where-Object { $_ -match ("^" + [regex]::Escape($name) + ":[^=]*=") } | Select-Object -First 1
        if ($line) { return ($line -split "=", 2)[1] }
        return $null
    }

    $generator = Get-CacheValue "CMAKE_GENERATOR"
    $platform  = Get-CacheValue "CMAKE_GENERATOR_PLATFORM"
    $configureArgs = @("-S", ".", "-B", $BuildDir)
    if ($generator) { $configureArgs += @("-G", $generator) }
    if ($platform)  { $configureArgs += @("-A", $platform) }
    foreach ($v in @("CMAKE_TOOLCHAIN_FILE", "SARIBBON_DIR", "CDT_DIR", "CMAKE_INSTALL_PREFIX")) {
        $value = Get-CacheValue $v
        if ($value) { $configureArgs += ("-D{0}={1}" -f $v, $value) }
    }

    Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue

    Write-Host "  重新配置（不计入耗时）..."
    & cmake @configureArgs 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "配置失败，退出码 $LASTEXITCODE。先确认 conan install 已跑过。"
    }

    $results["全量构建 ($Config)"] = Invoke-Build
    Write-Host ("  {0:N1} 秒" -f $results["全量构建 ($Config)"])
}

$i = 2
foreach ($t in $targets) {
    Write-Host ""
    Write-Host ("[{0}/4] 改 {1} 后的增量构建..." -f $i, $t.Name) -ForegroundColor Cyan
    $i++

    # [System.IO.File] 用的是进程的当前目录，PowerShell 的 Set-Location
    # 不会同步过去，所以这里必须拼成绝对路径。
    $path = Join-Path $repo $t.Path
    if (-not (Test-Path $path)) {
        Write-Host "  跳过：找不到 $path" -ForegroundColor Yellow
        continue
    }
    $path = (Resolve-Path $path).ProviderPath

    # 保存原始字节，改完再原样写回
    $originalBytes = [System.IO.File]::ReadAllBytes($path)
    try {
        Add-Content -Path $path -Value "// build-measure touch" -Encoding utf8
        $results[("改 {0} 后增量构建" -f $t.Name)] = Invoke-Build
        Write-Host ("  {0:N1} 秒" -f $results[("改 {0} 后增量构建" -f $t.Name)])
    }
    finally {
        [System.IO.File]::WriteAllBytes($path, $originalBytes)
    }
}

# 把文件恢复后再构建一次，让构建树回到干净状态
Write-Host ""
Write-Host "恢复源文件并重建..." -ForegroundColor Cyan
$null = Invoke-Build

Write-Host ""
Write-Host "==== 构建指标 ====" -ForegroundColor Green
Write-Host ("{0,-40} {1,12}" -f "指标", "耗时 (秒)")
foreach ($k in $results.Keys) {
    Write-Host ("{0,-40} {1,12:N1}" -f $k, $results[$k])
}
Write-Host ""
Write-Host "把这些数字填进 doc/BASELINE.md 的构建指标表。"
