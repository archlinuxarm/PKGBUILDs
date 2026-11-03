# pnpm 11 and later install global command shims in $PNPM_HOME/bin.
if set -q XDG_DATA_HOME
    set -gx PNPM_HOME "$XDG_DATA_HOME/pnpm"
else
    set -gx PNPM_HOME "$HOME/.local/share/pnpm"
end

fish_add_path --append --global "$PNPM_HOME/bin"
