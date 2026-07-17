<#
.SYNOPSIS
Compatibility entry point for docs/report/export_report.ps1.
#>
[CmdletBinding()]
param(
    [string]$CjkFont = "Microsoft YaHei",
    [string]$OutputPath = ""
)

$CanonicalScript = Join-Path $PSScriptRoot "..\report\export_report.ps1"
if (-not (Test-Path -LiteralPath $CanonicalScript -PathType Leaf)) {
    Write-Error "Canonical export script was not found: $CanonicalScript"
    exit 1
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    & $CanonicalScript -CjkFont $CjkFont
}
else {
    & $CanonicalScript -CjkFont $CjkFont -OutputPath $OutputPath
}
exit $LASTEXITCODE
