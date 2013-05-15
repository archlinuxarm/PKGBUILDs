post_install() {
  if [ ! -f /etc/ssl/certs/java/cacerts ]; then
    /usr/bin/init-jks-keystore
  fi
}

post_upgrade() {
  if [ ! -f /etc/ssl/certs/java/cacerts ]; then
    /usr/bin/init-jks-keystore
  fi
}
