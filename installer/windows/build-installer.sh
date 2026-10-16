#!/usr/bin/env bash
# Builds the Windows installer for every synth staged in <stage dir>.
# Runs in Git Bash on a Windows CI runner.
#
# Usage: build-installer.sh <version> <stage dir> <output dir>
#   <stage dir> holds one folder per synth, each with VST3/ and optionally
#   Standalone/, as laid out by the release workflow.
set -euo pipefail

version=$1
stage=$2
out=$3
here=$(cd "$(dirname "$0")" && pwd)

work=$(mktemp -d)
mkdir -p "$work/VST3" "$work/Standalone" "$work/Licenses" "$out"
icons="$work/icons.iss"
: > "$icons"

for synth_dir in "$stage"/*/; do
  cp -R "${synth_dir}VST3/." "$work/VST3/"
  if [ -f "${synth_dir}LICENSE.txt" ]; then
    cp "${synth_dir}LICENSE.txt" "$work/Licenses/$(basename "$synth_dir")-LICENSE.txt"
  fi
  for exe in "${synth_dir}"Standalone/*.exe; do
    [ -e "$exe" ] || continue
    cp "$exe" "$work/Standalone/"
    name=$(basename "$exe" .exe)
    echo "Name: \"{group}\\$name\"; Filename: \"{app}\\$name.exe\"" >> "$icons"
  done
done

iscc="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
if [ ! -f "$iscc" ]; then
  choco install innosetup -y --no-progress > /dev/null
fi

# ISCC accepts '-' as well as '/' for switches; '-' avoids Git Bash path mangling.
"$iscc" -Qp \
  "-DAppVersion=$version" \
  "-DStageDir=$(cygpath -w "$work")" \
  "-DIconsFile=$(cygpath -w "$icons")" \
  "-O$(cygpath -w "$out")" \
  "$(cygpath -w "$here/LoamSynthBundle.iss")"
