#!/bin/sh
sudo apt install -y curl git
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo bash - &&\
sudo apt install -y nodejs
sudo apt install -y build-essential expect python3 linuxcnc-uspace linuxcnc-uspace-dev mesaflash python3-flask python3-flask-cors python3-waitress grub-customizer
sudo npm install --global yarn
cd elle-app
yarn install
yarn app:build
