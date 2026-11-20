# Лабораторная работа №4. PKI, HTTPS и TLS 1.3

## Вариант 7

### Параметры варианта:
* **Ключ корневого и промежуточного CA:** ECDSA P-256 (`prime256v1`)
* **Ключ сервера:** Ed25519
* **Группа ECDHE:** X25519
* **Набор шифров TLS 1.3:** `TLS_AES_128_GCM_SHA256`
* **Задача варианта:** Возобновление сессии (Session Resumption по протоколу PSK): первое соединение с сохранением сессии в файл (`-sess_out`), второе с возобновлением (`-sess_in`), оба соединения с задержкой `( sleep 1 )` для получения тикета сессии.

---

## Структура каталога `lab4`

* `root/` — приватный ключ `root.key` и самоподписанный сертификат `root.crt` (ECDSA P-256)
* `inter/` — ключ `inter.key`, конфигурация `ca.cnf`, база `index.txt`, сертификат `inter.crt` с ограничением `pathlen:0`
* `server/` — ключ сервера `server.key` (Ed25519), сертификат `server.crt` с расширением SAN (`DNS:localhost,IP:127.0.0.1`), полная цепочка `fullchain.pem`
* `client/` — журнал ключей `keys.log`, сохранённая сессия `sess.pem`, дампы тестов
* `main.cpp` — исходный код C++ для офлайн-верификации цепочки сертификатов X.509 и демонстрации TLS 1.3 Session Resumption через библиотеку OpenSSL
* `setup_pki.sh` / `setup_pki.ps1` — скрипты развёртывания CA и выпуска сертификатов
* `run_tests.sh` — скрипт выполнения тестов и задачи варианта 7
* `nginx_lab4.conf` — файл конфигурации Nginx под параметры варианта 7

---

## 1. Выпуск сертификатов

### 1.1 Корневой CA (ECDSA P-256)
```bash
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out root/root.key
openssl req -new -x509 -key root/root.key -sha256 -days 3650 \
  -subj '/C=BY/O=Lab PKI/CN=Lab Root CA' \
  -addext 'basicConstraints=critical,CA:TRUE,pathlen:1' \
  -addext 'keyUsage=critical,keyCertSign,cRLSign' \
  -addext 'subjectKeyIdentifier=hash' \
  -out root/root.crt
```

### 1.2 Промежуточный CA (ECDSA P-256, pathlen:0)
```bash
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out inter/inter.key
openssl req -new -key inter/inter.key -subj '/C=BY/O=Lab PKI/CN=Lab Intermediate CA' -out inter/inter.csr
openssl x509 -req -in inter/inter.csr -CA root/root.crt -CAkey root/root.key -CAcreateserial \
  -sha256 -days 1825 -extfile inter/inter.ext -out inter/inter.crt
```

### 1.3 Сертификат сервера (Ed25519, SAN)
```bash
openssl genpkey -algorithm ED25519 -out server/server.key
openssl req -new -key server/server.key -subj '/C=BY/O=Lab PKI/CN=localhost' -out server/server.csr
openssl ca -config inter/ca.cnf -batch -in server/server.csr -extfile server/server.ext -notext -out server/server.crt
cat server/server.crt inter/inter.crt > server/fullchain.pem
```

---

## 2. Проверка цепочки доверия

Команда проверки с полной цепочкой:
```bash
openssl verify -CAfile root/root.crt -untrusted inter/inter.crt server/server.crt
# Вывод:
# server/server.crt: OK
```

Проверка без промежуточного CA (ожидаемый отказ):
```bash
openssl verify -CAfile root/root.crt server/server.crt
# Вывод:
# error 20 at 0 depth lookup: unable to get local issuer certificate
# error server/server.crt: verification failed
```

---

## 3. Выполнение задачи Варианта 7 (Session Resumption / PSK)

В TLS 1.3 после завершения полного рукопожатия сервер отправляет сообщение `NewSessionTicket`. Клиент сохраняет сессию и при повторном подключении передает расширение `pre_shared_key`. При этом сервер восстанавливает сессию без повторной аутентификации через сертификаты и без отправки `Certificate` и `CertificateVerify`.

### Команды запуска и проверки:
1. Запуск тестового TLS 1.3 сервера:
```bash
openssl s_server -accept 8453 -cert server/server.crt -key server/server.key \
  -cert_chain inter/inter.crt -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -www
```

2. Первое соединение (сохранение сессии в `client/sess.pem`):
```bash
( sleep 1 ) | openssl s_client -connect localhost:8453 -servername localhost \
  -CAfile root/root.crt -tls1_3 -brief -sess_out client/sess.pem
```
**Фактический вывод первого соединения:**
```text
CONNECTION ESTABLISHED
Protocol version: TLSv1.3
Ciphersuite: TLS_AES_128_GCM_SHA256
Peer certificate: CN=localhost
Hash used: UNDEF
Signature type: ed25519
Verification: OK
Peer Temp Key: X25519, 253 bits
DONE
```
*Наблюдение:* Присутствует поле `Signature type: ed25519`, подтверждающее выполнение `CertificateVerify` сервером с закрытым ключом Ed25519.

3. Второе соединение (возобновление сессии из `client/sess.pem`):
```bash
( sleep 1 ) | openssl s_client -connect localhost:8453 -servername localhost \
  -CAfile root/root.crt -tls1_3 -brief -sess_in client/sess.pem
```
**Фактический вывод второго соединения:**
```text
CONNECTION ESTABLISHED
Protocol version: TLSv1.3
Ciphersuite: TLS_AES_128_GCM_SHA256
Peer certificate: CN=localhost
Verification: OK
Peer Temp Key: X25519, 253 bits
DONE
```
*Наблюдение:* Поле `Signature type: ed25519` отсутствует, так как проверка подписи не производилась — сессия была мгновенно возобновлена с помощью Pre-Shared Key (PSK), полученного в тикете первого соединения.

---

## 4. Сборка и запуск программы на C++

В каталог включена реализация на C++ (`main.cpp`), которая:
1. Производит верификацию цепочки сертификатов X.509 средствами OpenSSL C API.
2. Подключается к серверу, согласовывает TLS 1.3, шифр `TLS_AES_128_GCM_SHA256` и кривую `X25519`.
3. Сохраняет сессию и выполняет повторное подключение с подтверждением возобновления (`SSL_session_reused == true`).

Компиляция:
```bash
g++ -O2 main.cpp -lssl -lcrypto -lws2_32 -o lab4.exe
```

Запуск:
```bash
./lab4.exe
```

---

## 5. Негативные тесты

1. **Запрос без доверенного корневого CA:**
   ```bash
   curl https://localhost:8443/
   ```
   *Результат:* `exit 60, curl: (60) SSL certificate problem: unable to get local issuer certificate`.

2. **Запрос по имени, отсутствующему в SAN (127.0.0.2):**
   ```bash
   curl --cacert root/root.crt https://127.0.0.2:8443/
   ```
   *Результат:* `exit 60, SSL certificate problem: no alternative certificate subject name matches target host name '127.0.0.2'`.

3. **Попытка принудительного согласования устаревшего TLS 1.2:**
   ```bash
   ( sleep 1 ) | openssl s_client -connect localhost:8443 -tls1_2 -CAfile root/root.crt
   ```
   *Результат:* Получен `alert protocol version (70)`, так как сервер настроен строго на `ssl_protocols TLSv1.3`.
