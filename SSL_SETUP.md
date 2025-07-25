# Extract unencrypted private key
openssl pkcs12 -in client.p12 -out client_key.pem -nocerts -nodes

# Extract the client certificate
openssl pkcs12 -in client.p12 -out client_cert.pem -clcerts -nokeys
