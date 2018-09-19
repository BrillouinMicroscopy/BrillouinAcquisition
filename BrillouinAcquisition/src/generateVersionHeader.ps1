Param (
  [String]$Project='.',
  [String]$VERSION_MAJOR = 0,
  [String]$VERSION_MINOR = 0,
  [String]$VERSION_PATCH = 1,
  [String]$VERSION_PRERELEASE = "alpha",
  [String]$Namespace = 'Version',
  [String]$GitRoot,
  $Author = "Raimund Schlüßler",
  $AuthorEmail = "raimund.schluessler@tu-dresden.de",
  [String]$HeaderFile = "src/version.h",
  [String]$VerPrefix = "https://github.com/BrillouinMicroscopy/BrillouinAcquisitionCpp/commit/"
)

Push-Location -LiteralPath $GitRoot

$VerFileHead = "#ifndef VERSION_H`r`n#define VERSION_H`r`n`r`n#include <string>`r`n`r`nnamespace $Namespace {`r`n"
$VerFileTail = "}`r`n#endif // VERSION_H`r`n"

$VerMajor = "  const int MAJOR = "+$VERSION_MAJOR+";" | Out-String
$VerMinor = "  const int MINOR = "+$VERSION_MINOR+";" | Out-String
$VerPatch = "  const int PATCH = "+$VERSION_PATCH+";" | Out-String
$VerPreRelease = "  const std::string PRERELEASE = `""+$VERSION_PRERELEASE+"`";" | Out-String
$VerCommit = git log -n 1 --format=format:"  const std::string Commit = `\`"%H`\`";%n" | Out-String
#$VerBy   = git log -n 1 --format=format:"  const std::string Author = `\`"%an `<%ae`>`\`";%n" | Out-String
$VerAuthor = "  const std::string Author = `""+$Author+"`";" | Out-String
$VerEmail = "  const std::string AuthorEmail = `""+$AuthorEmail+"`";" | Out-String
$VerUrl  = git log -n 1 --format=format:"  const std::string Url = `\`"$VerPrefix%H`\`";%n" | Out-String
$VerDate = git log -n 1 --format=format:"  const std::string Date = `\`"%ai`\`";%n" | Out-String
$VerSubj = git log -n 1 --format=format:"  const std::string Subject = `\`"%f`\`";%n" | Out-String

$VerChgs = ((git ls-files --exclude-standard -d -m -o -k) | Measure-Object -Line).Lines

if ($VerChgs -gt 0) {
  $VerDirty = "  const bool VerDirty = true;`r`n"
} else {
  $VerDirty = "  const bool VerDirty = false;`r`n"
}

"Written $Project\" + (
  New-Item -Force -Path "$Project" -Name "$HeaderFile" -ItemType "file" -Value "$VerFileHead$VerMajor$VerMinor$VerPatch$VerPreRelease$VerCommit$VerUrl$VerDate$VerSubj$VerAuthor$VerEmail$VerDirty$VerFileTail"
).Name + " as:"
""
Get-Content "$Project\$HeaderFile" -Encoding UTF8
""

Pop-Location
