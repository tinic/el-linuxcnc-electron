#!/bin/sh
yarn app:build; git commit -a -m "."; git push; git push --tags; yarn publish:github
