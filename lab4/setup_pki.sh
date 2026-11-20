#!/usr/bin/env bash
set -e

echo "=== Создание инфраструктуры PKI для Лабораторной работы №4 (Вариант 7) ==="
mkdir -p root inter server client

# 1. Корневой CA (ECDSA P-256)
echo "[1/4] Генерация Root CA..."
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out root/root.key
chmod 600 root/root.key
openssl req -new -x509 -key root/root.key -sha256 -days 3650 \
  -subj '/C=BY/O=Lab PKI/CN=Lab Root CA' \
  -addext 'basicConstraints=critical,CA:TRUE,pathlen:1' \
  -addext 'keyUsage=critical,keyCertSign,cRLSign' \
  -addext 'subjectKeyIdentifier=hash' \
  -out root/root.crt

# 2. Промежуточный CA (ECDSA P-256)
echo "[2/4] Генерация Intermediate CA..."
cat > inter/inter.ext <<'EOF'
basicConstraints=critical,CA:TRUE,pathlen:0
keyUsage=critical,keyCertSign,cRLSign
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always
EOF

openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out inter/inter.key
chmod 600 inter/inter.key
openssl req -new -key inter/inter.key -subj '/C=BY/O=Lab PKI/CN=Lab Intermediate CA' -out inter/inter.csr
openssl x509 -req -in inter/inter.csr -CA root/root.crt -CAkey root/root.key -CAcreateserial \
  -sha256 -days 1825 -extfile inter/inter.ext -out inter/inter.crt

# 3. База данных промежуточного CA
touch inter/index.txt
echo 1000 > inter/serial
echo 01 > inter/crlnumber

cat > inter/ca.cnf <<'EOF'
[ ca ]
default_ca = CA_default
[ CA_default ]
dir              = ./inter
database         = $dir/index.txt
new_certs_dir    = $dir
serial           = $dir/serial
crlnumber        = $dir/crlnumber
certificate      = $dir/inter.crt
private_key      = $dir/inter.key
default_md       = sha256
default_days     = 365
default_crl_days = 30
unique_subject   = no
copy_extensions  = none
policy           = policy_any
[ policy_any ]
commonName       = supplied
EOF

# 4. Серверный сертификат (Ed25519) с SAN
echo "[3/4] Выпуск серверного сертификата (Ed25519)..."
cat > server/server.ext <<'EOF'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=serverAuth
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always
subjectAltName=DNS:localhost,IP:127.0.0.1
EOF

openssl genpkey -algorithm ED25519 -out server/server.key
chmod 600 server/server.key
openssl req -new -key server/server.key -subj '/C=BY/O=Lab PKI/CN=localhost' -out server/server.csr
openssl ca -config inter/ca.cnf -batch -in server/server.csr -extfile server/server.ext -notext -out server/server.crt

# 5. Сборка fullchain
echo "[4/4] Сборка fullchain.pem..."
cat server/server.crt inter/inter.crt > server/fullchain.pem

echo "=== Готово! Сертификаты успешно выпущены. ==="
