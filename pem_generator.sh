#!/bin/bash
set -e

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <bankid_cert.p12> <password>"
  exit 1
fi

P12_FILE="$1"
P12_PASS="$2"

if [ ! -f "$P12_FILE" ]; then
  echo "Error: File '$P12_FILE' not found."
  exit 2
fi

openssl pkcs12 -in "$P12_FILE" -out bankid_key.pem -nocerts -nodes -password "pass:$P12_PASS"
openssl pkcs12 -in "$P12_FILE" -out bankid_cert.pem -clcerts -nokeys -password "pass:$P12_PASS"

echo "PEM files generated successfully:"
echo "  bankid_key.pem"
echo "  bankid_cert.pem"