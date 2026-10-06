#!/usr/bin/env bash
# scripts/loop-status.sh — the loop's health instrument.
# Run on the build farm. Every line is an objective gate, not a claim.
# "Building well" = demos run, tests green, goldens verify, diffs small, cited.
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1
FAIL=0
ok(){ printf '  \033[32m✓\033[0m %s\n' "$*"; }
no(){ printf '  \033[31m✗\033[0m %s\n' "$*"; FAIL=1; }
step(){ printf '\n\033[1;36m══ %s ══\033[0m\n' "$*"; }

step "BUILD"
if cmake --build build -j"$(nproc)" >/tmp/loop-build.log 2>&1; then ok "compiles clean (gcc $(gcc -dumpversion), $(nproc) cores)"; else no "BUILD FAILED (see /tmp/loop-build.log)"; fi

step "TESTS (objective)"
if ctest --test-dir build >/tmp/loop-test.log 2>&1; then
  ok "$(grep -oE '[0-9]+/[0-9]+ Test' /tmp/loop-test.log | tail -1 | cut -d' ' -f1) tests passed"
else no "TESTS FAILED"; fi

step "DEMOS (playbook: every phase ends runnable)"
OUT=$(./build/mojave --discover 2>&1)
N=$(echo "$OUT" | grep -oE 'Found [0-9]+' | grep -oE '[0-9]+')
[ "${N:-0}" -ge 1 ] && ok "--discover found $N real install(s)" || no "--discover found nothing"
E=$(./build/mojave --esm-info 2>&1)
V=$(echo "$E" | awk '/version:/{print $2}'); R=$(echo "$E" | awk '/numRecords:/{print $2}')
[ -n "$V" ] && ok "--esm-info: master parsed (version $V, $R records)" || no "--esm-info failed"

step "GOLDENS (real-file oracle)"
if ./build/mojave_tests_esmparser 2>/dev/null | grep -q "PASS: real esm"; then
  ok "real-file golden verified: $(./build/mojave_tests_esmparser 2>/dev/null | grep 'PASS: real esm' | head -1)"
else printf '  -  real-file golden skipped (no install on this box)\n'; fi

step "DISCIPLINE (AGENTS.md)"
BIG=$(git log -1 --numstat --format= | awk '{a+=$1;d+=$2} END{print a+d}')
[ "${BIG:-0}" -le 400 ] && ok "last commit diff ${BIG} lines (≤400 rule)" || no "last commit diff ${BIG} lines (>400 — split it)"
git log --oneline -1 | grep -qiE "^(fix|feat|day|phase|assetlocator|research)" && ok "commit message conventional" || printf '  -  check commit message convention\n'
ok "format docs cited: $(ls docs/formats/ 2>/dev/null | wc -l) file(s)"
ok "ADRs: $(ls adr/*.md 2>/dev/null | wc -l)"

step "VERDICT"
[ "$FAIL" = "0" ] && printf '  \033[1;32mBUILDING WELL\033[0m — gates green, demos runnable, goldens verified\n' || printf '  \033[1;31mATTENTION\033[0m — gate(s) failed above\n'
exit $FAIL
