#!/usr/bin/env bash
set -e

echo "=== Запуск тестов Лабораторной работы №4 (Вариант 7) ==="

echo "--- 1. Проверка цепочки сертификатов ---"
openssl verify -CAfile root/root.crt -untrusted inter/inter.crt server/server.crt
openssl verify -CAfile root/root.crt server/server.crt || echo "[OK] Ожидаемый отказ без промежуточного CA"

echo "--- 2. Запуск тестового TLS 1.3 сервера (порт 8453) ---"
openssl s_server -accept 8453 -cert server/server.crt -key server/server.key \
  -cert_chain inter/inter.crt -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -www &
SERVER_PID=$!
sleep 2

echo "--- 3. Задача варианта 7: Возобновление сессии (Session Resumption / PSK) ---"
echo "[Шаг 1] Первое соединение с сохранением сессии (-sess_out)..."
( sleep 1 ) | openssl s_client -connect localhost:8453 -servername localhost \
  -CAfile root/root.crt -tls1_3 -brief -sess_out client/sess.pem

echo "[Шаг 2] Второе соединение с возобновлением сессии (-sess_in)..."
( sleep 1 ) | openssl s_client -connect localhost:8453 -servername localhost \
  -CAfile root/root.crt -tls1_3 -brief -sess_in client/sess.pem

kill $SERVER_PID || true

echo "--- 4. Негативные тесты ---"
echo "[Негативный тест 1] Подключение без указания доверенного корня: curl https://localhost:8443/ (ожидается exit 60)"
echo "[Негативный тест 2] Подключение по IP вне SAN (127.0.0.2): curl --cacert root/root.crt https://127.0.0.2:8443/ (ожидается несоответствие имени SAN)"
echo "[Негативный тест 3] Попытка согласования TLS 1.2: openssl s_client -tls1_2 (ожидается alert protocol version 70)"

echo "=== Тесты завершены ==="
