Param (
  [String]$Project='.',
  [String]$VERSION_MAJOR = 0,
  [String]$VERSION_MINOR = 3,
  [String]$VERSION_PATCH = 5,
  [String]$VERSION_PRERELEASE = "",
  [String]$Namespace = 'Version',
  [String]$GitRoot,
  $Author = "Raimund Schl��ler",
  $AuthorEmail = "raimund.schluessler@tu-dresden.de",
  [String]$HeaderFile = "src/version.h",
  [String]$VerPrefix = "https://github.com/BrillouinMicroscopy/BrillouinAcquisition/commit/"
)

Push-Location -LiteralPath $GitRoot

$VerFileHead = "#ifndef VERSION_H`r`n#define VERSION_H`r`n`r`n#include <string>`r`n`r`nnamespace $Namespace {`r`n"
$VerFileTail = "}`r`n#endif // VERSION_H`r`n"

$VerMajor = "  const auto MAJOR{ "+$VERSION_MAJOR+" };" | Out-String
$VerMinor = "  const auto MINOR{ "+$VERSION_MINOR+" };" | Out-String
$VerPatch = "  const auto PATCH{ "+$VERSION_PATCH+" };" | Out-String
$VerPreRelease = "  const auto PRERELEASE = std::string{ `""+$VERSION_PRERELEASE+"`" };" | Out-String
$VerCommit = git log -n 1 --format=format:"  const auto Commit = std::string{ `\`"%H`\`" };%n" | Out-String
$VerAuthor = "  const auto Author = std::string{ `""+$Author+"`" };" | Out-String
$VerEmail = "  const auto AuthorEmail = std::string{ `""+$AuthorEmail+"`"};" | Out-String
$VerUrl  = git log -n 1 --format=format:"  const auto Url = std::string{ `\`"$VerPrefix%H`\`" };%n" | Out-String
$VerDate = git log -n 1 --format=format:"  const auto Date = std::string{ `\`"%ai`\`" };%n" | Out-String
$VerSubj = git log -n 1 --format=format:"  const auto Subject = std::string{ `\`"%f`\`" };%n" | Out-String

$VerChgs = ((git ls-files --exclude-standard -d -m -o -k) | Measure-Object -Line).Lines

if ($VerChgs -gt 0) {
  $VerDirty = "  const auto VerDirty{ true };`r`n"
} else {
  $VerDirty = "  const auto VerDirty{ false };`r`n"
}

"Written $Project\" + (
  New-Item -Force -Path "$Project" -Name "$HeaderFile" -ItemType "file" -Value "$VerFileHead$VerMajor$VerMinor$VerPatch$VerPreRelease$VerCommit$VerUrl$VerDate$VerSubj$VerAuthor$VerEmail$VerDirty$VerFileTail"
).Name + " as:"
""
Get-Content "$Project\$HeaderFile" -Encoding UTF8
""

Pop-Location
