param([Parameter(ValueFromRemainingArguments=$true)][string[]]$ReaArguments)
$ErrorActionPreference='Stop'
$root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$runtime=Join-Path $root '.local/tooling/node-v24.22.1-win-x64/node.exe'
$entry=Join-Path $root '.local/tooling/rea-runtime/node_modules/rea-agents/scripts/rea.mjs'
if(-not (Test-Path -LiteralPath $runtime) -or -not (Test-Path -LiteralPath $entry)){throw 'REA runtime missing; see docs/tooling.md'}
& $runtime $entry @ReaArguments
exit $LASTEXITCODE
