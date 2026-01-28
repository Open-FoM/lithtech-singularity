#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

engine_bin="${root_dir}/bin/singularity"
rez_dir="${root_dir}/game/renderdemo/rez"
working_dir="${root_dir}/game/renderdemo/bin"

exec "${engine_bin}" \
  -workingdir "${working_dir}" \
  -rez "${root_dir}/engine.rez" \
  -rez "${rez_dir}" \
  "$@"
