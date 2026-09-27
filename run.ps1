# Ensure we use the script's actual directory as the base
$scriptDir = $PSScriptRoot
if (-not $scriptDir) { $scriptDir = Get-Location }

# 1. Ask input for "path" relative to the script
$relPath = Read-Host "Enter the relative path (e.g., test\folder\myApp.exe)"

# Clean up any accidental leading slashes or quotes from user input
$relPath = $relPath.Trim('"', "'", ' ', '\', '/')

# Resolve full target path safely using .NET
$targetFullPath = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($scriptDir, $relPath))
$targetDir = [System.IO.Path]::GetDirectoryName($targetFullPath)

# Get the top-level folder created so we can easily delete it later
$firstSegment = ($relPath -split '[/\\]')[0]
$topLevelDir = Join-Path $scriptDir $firstSegment

# 2. Create the directories recursively
Write-Host "[*] Creating directories: $targetDir" -ForegroundColor Cyan
if (-not [string]::IsNullOrEmpty($targetDir)) {
    [System.IO.Directory]::CreateDirectory($targetDir) | Out-Null
}

# 3. Copy .\dummy.exe and rename it to match the target filename
$sourceDummy = Join-Path $scriptDir "dummy.exe"
if (-not (Test-Path $sourceDummy)) {
    Write-Error "Error: 'dummy.exe' not found in the script directory!"
    exit
}

Write-Host "[*] Copying and renaming dummy.exe..." -ForegroundColor Cyan
Copy-Item -Path $sourceDummy -Destination $targetFullPath -Force

# 4. Ask input for "time (mins)"
$minsInput = Read-Host "Enter runtime duration in minutes"
$mins = [int]$minsInput

# Open the executable
Write-Host "[*] Launching application... Running for $mins minute(s)." -ForegroundColor Green
$process = Start-Process -FilePath $targetFullPath -PassThru

# Wait for the specified minutes (converting minutes to seconds)
Start-Sleep -Seconds ($mins * 60)

# Kill the process if it's still running
if (-not $process.HasExited) {
    Write-Host "[*] Time's up. Stopping the process..." -ForegroundColor Yellow
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
}

# Delete all created directories and files
Write-Host "[*] Cleaning up created directories..." -ForegroundColor Cyan
if (Test-Path $topLevelDir) {
    Remove-Item -Path $topLevelDir -Recurse -Force
}

Write-Host "[+] Cleanup complete. Done!" -ForegroundColor Green