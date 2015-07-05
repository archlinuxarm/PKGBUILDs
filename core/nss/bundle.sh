#!/bin/sh
# From Fedora's ca-certificates.spec

(
  cat <<EOF
# This is a bundle of X.509 certificates of public Certificate
# Authorities.  It was generated from the Mozilla root CA list.
# These certificates are in the OpenSSL "TRUSTED CERTIFICATE"
# format and have trust bits set accordingly.
# An exception are auxiliary certificates, without positive or negative
# trust, but are used to assist in finding a preferred trust path.
# Those neutral certificates use the plain BEGIN CERTIFICATE format.
#
# Source: nss/lib/ckfw/builtins/certdata.txt
# Source: nss/lib/ckfw/builtins/nssckbi.h
#
# Generated from:
EOF
  cat certs/nssckbi.h | grep -w NSS_BUILTINS_LIBRARY_VERSION | awk '{print "# " $2 " " $3}'
  echo '#'
) > ca-bundle.trust.crt
for f in certs/*.crt; do 
  echo "processing $f"
  tbits=`sed -n '/^# openssl-trust/{s/^.*=//;p;}' $f`
  distbits=`sed -n '/^# openssl-distrust/{s/^.*=//;p;}' $f`
  alias=`sed -n '/^# alias=/{s/^.*=//;p;q;}' $f | sed "s/'//g" | sed 's/"//g'`
  targs=""
  if [ -n "$tbits" ]; then
    for t in $tbits; do
       targs="${targs} -addtrust $t"
    done
  fi
  if [ -n "$distbits" ]; then
    for t in $distbits; do
       targs="${targs} -addreject $t"
    done
  fi
  if [ -n "$targs" ]; then
    echo "trust flags $targs for $f" >> info.trust
    openssl x509 -text -in "$f" -trustout $targs -setalias "$alias" >> ca-bundle.trust.crt
  else
    echo "no trust flags for $f" >> info.notrust
    # p11-kit-trust defines empty trust lists as "rejected for all purposes".
    # That's why we use the simple file format
    #   (BEGIN CERTIFICATE, no trust information)
    # because p11-kit-trust will treat it as a certificate with neutral trust.
    # This means we cannot use the -setalias feature for neutral trust certs.
    openssl x509 -text -in "$f" >> ca-bundle.neutral-trust.crt
  fi
done

for p in certs/*.p11-kit; do 
  cat "$p" >> ca-bundle.supplement.p11-kit
done
