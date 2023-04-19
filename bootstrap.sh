#!/bin/sh
sudo apt install -y curl git
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo bash - &&\
sudo apt install -y nodejs
sudo apt install -y build-essential expect python3 linuxcnc-uspace linuxcnc-uspace-dev mesaflash python3-flask python3-flask-cors python3-waitress grub-customizer
cd elle-app
sudo npm install --global yarn
yarn install
yarn app:build
