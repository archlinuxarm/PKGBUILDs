# send_mail.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-07-02
# Public domain

# Commentary:

# TODO: implement Fcc headers (see emacs manual)

# Code:

#:docstring send_mail:
# Usage: send_mail
#
# This function serves as a simple replacement for sendmail as a client
# interface on those systems where it is not available.  It does assume
# that one can talk to an SMTP mailer on port 25 either on the local host
# or on the host specified by the MAILHOST environment variable.  If you
# have access to sendmail, it's better to use 'sendmail -t' instead of this
# script (which probably isn't as robust).
#
# Message is read from stdin, and headers are parsed to determine
# recipients.  
#:end docstring:

###;;;autoload
function send_mail ()
{
    # Need gawk, since several extensions are taken advantage of (like
    # IGNORECASE for regexps).
    local awk="${GAWK_LOCATION:-gawk}"
    local DefaultFrom="${USER:-${LOGNAME}}"
    local From
    local To
    local Cc
    local Bcc
    local tmpfile="/tmp/send_mail$$"

    while [ -e "${tmpfile}" ]; do
       tmpfile="/tmp/send_mail${RANDOM}"
    done

    # Lines consisting only of dots need one more dot appended.  SMTP
    # servers eat one of the dots (and if only 1 dot appears, it signifies
    # the end of the message).
    sed '/^\.\.*/s/^\(\.\.*\)$/\1./' > "${tmpfile}"

    # Parse mail headers in message to extract recipients list. 
    # This doesn't affect what the user sees---it's only used to generate
    # the rcpt-to lines for SMTP. 
    eval $(${awk} -f - "${tmpfile}" <<- '__EOF__'
       # Try to extract email address from amidst random data
       function parse_address (data)
       {
           # From: "real name" <foobar@host>
           # From: "" <foobar@host>
           if (match(data, /^\"[^\"]*\"[ \t]*<.*>/)) {
              data_idx = match(data, /^\"[^\"]*\"[ \t]*</)
              data = substr(data, RSTART + RLENGTH);
              if (data_idx = match(data, ">.*"))
                 data = substr(data, 1, RSTART - 1);
              return data
           }
           # From: real name <foobar@host>
           if (match(data, /<.*>/)) {
              data_idx = match(data, /</)
              data = substr(data, RSTART + RLENGTH);
              if (data_idx = match(data, ">"))
                 data = substr(data, 1, RSTART - 1);
              return data
           }
           # From: foobar@host (real name)
           if (match(data, /\(.*\)/)) {
              data_idx = match(data, /\(/);
              data = substr(data, 1, RSTART - 1);
              return data
           }
           # (hopefully) From: foobar@host
           return data
       }

       BEGIN { IGNORECASE = 1; }

       # Blank line signifies end of headers, so we can stop looking.
       /^$/ { exit(0) }

       /^from:|^to:|^cc:|^bcc:/ {
          header_idx = match($0, /^[^:]*:/)
          if (header_idx) {
             # Capitalize header name
             header_firstchar = toupper(substr($0, RSTART, 1));
             header_rest = tolower(substr($0, RSTART + 1, RLENGTH - 2));
             header = header_firstchar header_rest

             $0 = substr($0, RSTART + RLENGTH + 1);
             addresses = ""
             # parse addresses
             while ($0) {
                # Strip leading whitespace
                if (idx = match($0, /[ \t]*/))
                   $0 = substr($0, RSTART + RLENGTH);

                # Find everything up to a nonquoted comma
                # FIXME: doesnt handle quoting yet
                if (idx = match($0, /,/)) {
                   data = substr($0, 1, RSTART);
                   $0 = substr($0, RSTART + 1);
                } else {
                   data = $0
                   $0 = ""
                }
                addresses = addresses " " parse_address(data)
             }

             printf("%s='%s'\n", header, addresses);
          }
       }
	__EOF__)

    # Not sure if an address is *required* after the HELO.. every sendmail
    # I tried talking to didn't seem to care.  Some sendmails don't care
    # if there's a HELO at all. 
    cat <<- __EOF__ | telnet ${MAILHOST:-localhost} 25 > /dev/null 2>&1
	HELO
	mail from: ${From:-${DefaultFrom}}
	$(for name in ${To} ${Cc} ${Bcc} ; do
	     echo "rcpt to: ${name}"
	  done)
	data
	$(cat "${tmpfile}")
	.
	quit
	__EOF__

    rm -f "${tmpfile}"
}

provide send_mail

# send_mail.bash ends here
