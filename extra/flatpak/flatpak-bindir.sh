if [ -n "$XDG_DATA_HOME" ] && [ -d "$XDG_DATA_HOME/flatpak/exports/bin" ]; then
  append_path "$XDG_DATA_HOME/flatpak/exports/bin"
elif [ -n "$HOME" ] && [ -d "$HOME/.local/share/flatpak/exports/bin" ]; then
  append_path "$HOME/.local/share/flatpak/exports/bin"
fi

if [ -d /var/lib/flatpak/exports/bin ]; then
  append_path /var/lib/flatpak/exports/bin
fi
