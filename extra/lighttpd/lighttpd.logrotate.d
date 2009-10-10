/var/log/lighttpd/*log {
   postrotate
      /bin/kill -HUP `cat /var/run/lighttpd/lighttpd.pid 2>/dev/null` 2> /dev/null || true
   endscript
}
