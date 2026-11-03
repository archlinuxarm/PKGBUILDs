# pnpm 11 and later install global command shims in $PNPM_HOME/bin.
# Keep the distribution-owned pnpm executable in /usr/bin while exposing
# user-owned global commands without running `pnpm setup`.
export PNPM_HOME="${XDG_DATA_HOME:-$HOME/.local/share}/pnpm"
append_path "$PNPM_HOME/bin"
