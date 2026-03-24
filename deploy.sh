#!/bin/bash
# Установка зависимостей
sudo apt update
sudo apt install -y build-essential cmake ninja-build libboost-all-dev libssl-dev

# Сборка
rm -rf build
mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

# Запуск (пример: слушаем 8080, пробрасываем на google.com:443)
# Замени пути к сертификатам на свои
./jerboa-tcp-tls -l 8080 -r google.com:443