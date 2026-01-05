param (
    [string]$Remote = "origin",
    [string]$Suffix = ""
)

$Version = "$(git log --date=short --format='%ad-%h' -1)$Suffix"
Write-Host "Version: $Version"
git tag $Version
git push --tags $Remote $Version
