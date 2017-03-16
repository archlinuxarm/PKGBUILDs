#!/bin/sh
# From Fedora's ca-certificates.spec

(
  cat <<EOF
# This is a bundle of X.509 certificates of public Certificate
# Authorities.  It was generated from the Mozilla root CA list.
# These certificates and trust/distrust attributes use the file format accepted
# by the p11-kit-trust module.
#
# Source: nss/lib/ckfw/builtins/certdata.txt
# Source: nss/lib/ckfw/builtins/nssckbi.h
#
# Generated from:
EOF
  cat certs/nssckbi.h | grep -w NSS_BUILTINS_LIBRARY_VERSION | awk '{print "# " $2 " " $3}'
  echo '#'
) > ca-bundle.trust.p11-kit

for p in certs/*.tmp-p11-kit; do 
  cat "$p" >> ca-bundle.trust.p11-kit
done
