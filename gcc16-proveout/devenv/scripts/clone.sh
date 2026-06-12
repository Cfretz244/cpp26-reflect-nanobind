#!/usr/bin/env bash
# Clone the GCC source into devenv/gcc/ (git-ignored). Shallow-ish: full
# trees are ~5 GB of history; --filter=blob:none gives full branch/log
# topology with lazy blob fetch -- the right tradeoff for development
# (bisect works; blobs stream in on checkout).
#
# Branches fetched:
#   master            -- where bugs are FILED against and fixes land first
#   releases/gcc-16   -- what the corpus runs (16.1) and what 16.2 ships from
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

if [ -d gcc/.git ]; then
  echo "gcc/ already cloned; updating instead"
  git -C gcc fetch origin master releases/gcc-16
  exit 0
fi

git clone --filter=blob:none https://gcc.gnu.org/git/gcc.git gcc
git -C gcc checkout master
# Make the release branch available locally too.
git -C gcc branch --track releases/gcc-16 origin/releases/gcc-16 || true
echo
echo "Cloned. Branches: master (checked out), releases/gcc-16."
echo "Next: bash scripts/configure.sh [master|gcc-16]"
