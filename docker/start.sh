#!/bin/sh
set -e

# Start C++ spreadsheet server in background
spreadsheet_server 3001 &

# Start Nginx in foreground (keeps container alive)
nginx -g 'daemon off;'
