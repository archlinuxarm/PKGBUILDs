THIS_JRE='java-8-openjdk/jre'

fix_default() {
  if [ ! -x /usr/bin/java ]; then
    /usr/bin/archlinux-java unset
    echo ""
  else
    /usr/bin/archlinux-java get
  fi
}

post_install() {
  default=$(fix_default)
  case ${default} in
    "")
      /usr/bin/archlinux-java set ${THIS_JRE}
      ;;
    ${THIS_JRE} | ${THIS_JRE/\/jre})
      # Nothing
      ;;
    *)
      echo "Default Java environment is already set to '${default}'"
      echo "See 'archlinux-java help' to change it"
      ;;
  esac

  xdg-icon-resource forceupdate --theme hicolor 2> /dev/null
  echo "when you use a non-reparenting window manager,"
  echo "set _JAVA_AWT_WM_NONREPARENTING=1 in /etc/profile.d/jre.sh"

#  update-desktop-database -q
}

post_upgrade() {
  if [ -z $(fix_default) ]; then
    /usr/bin/archlinux-java set ${THIS_JRE}
  fi

  xdg-icon-resource forceupdate --theme hicolor 2> /dev/null

#  update-desktop-database -q
}

pre_remove() {
  if [ "x$(fix_default)" = "x${THIS_JRE/\/jre}" ]; then
    /usr/bin/archlinux-java unset
    echo "No Java environment is set as default anymore"
  fi
}

post_remove() {
  xdg-icon-resource forceupdate --theme hicolor 2> /dev/null

#  update-desktop-database -q
}
