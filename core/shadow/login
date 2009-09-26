#%PAM-1.0
auth		required	pam_securetty.so
auth		requisite	pam_nologin.so
auth		required	pam_unix.so nullok
auth		required	pam_tally.so onerr=succeed file=/var/log/faillog
# use this to lockout accounts for 10 minutes after 3 failed attempts
#auth		required	pam_tally.so deny=2 unlock_time=600 onerr=succeed file=/var/log/faillog
account		required	pam_access.so
account		required	pam_time.so
account		required	pam_unix.so
#password	required	pam_cracklib.so difok=2 minlen=8 dcredit=2 ocredit=2 retry=3
#password	required	pam_unix.so md5 shadow use_authtok
session		required	pam_unix.so
session		required	pam_env.so
session		required	pam_motd.so
session		required	pam_limits.so
session		optional	pam_mail.so dir=/var/spool/mail standard
session		optional	pam_lastlog.so
