#!/bin/bash

BOOT_CONFIG_PATH=/boot/firmware/config.txt
NGINX_FCGI_CONFIG_PATH=/etc/nginx/fastcgi.conf
NGINX_SITE_PATH=/etc/nginx/sites-available/floppcgi
SCRIPT_DIR="`dirname "$0"`"

# Install required packages.
apt install -y nginx libfcgi-dev mtools git || exit 1

if [ ! -x "$SCRIPT_DIR/floppcgi" ]; then
	echo "compiling floppcgi..."
	make floppcgi || exit 1
fi

# Add dwc2 overlay for USB Gadget support.
if ! grep -q "^dtoverlay=dwc2$" "$BOOT_CONFIG_PATH"; then
	echo "adding dtoverlay=dwc2 to $BOOT_CONFIG_PATH..."
	echo "dtoverlay=dwc2" >> "$BOOT_CONFIG_PATH"
fi

# Add dwc2 module for USB Gadget support.
if ! grep -q "^dwc2$" "/etc/modules"; then
	echo "adding dwc2 to /etc/modules..."
	echo "dwc2" >> /etc/modules
fi

if [ ! -f "$NGINX_SITE_PATH" ]; then
	echo "setting up floppcgi nginx site..."
	cp -v floppcgi.nginx "$NGINX_SITE_PATH" || exit 1
	ln -vs "$NGINX_SITE_PATH" /etc/nginx/sites-enabled || exit 1
	rm -v /etc/nginx/sites-enabled/default || exit 1
fi

# Add config variables to FastCGI environment.
if ! grep -q "^fastcgi_param\s*FLOPPIES_ROOT" "$NGINX_FCGI_CONFIG_PATH"
then
	echo "adding FLOPPIES_ROOT to $NGINX_FCGI_CONFIG_PATH..."
	echo "fastcgi_param  FLOPPIES_ROOT  /srv/Floppies;" \
		>> "$NGINX_FCGI_CONFIG_PATH"
fi

if ! grep -q "^fastcgi_param\s*FLOPPIES_CONTAINER" "$NGINX_FCGI_CONFIG_PATH"
then
	echo "adding FLOPPIES_CONTAINER to $NGINX_FCGI_CONFIG_PATH..."
	echo "fastcgi_param  FLOPPIES_CONTAINER  /tmp/floppyc.img;" \
		>> "$NGINX_FCGI_CONFIG_PATH"
fi

if ! grep -q "^fastcgi_param\s*FLOPPIES_ASSETS" "$NGINX_FCGI_CONFIG_PATH"
then
	echo "adding FLOPPIES_ASSETS to $NGINX_FCGI_CONFIG_PATH..."
	echo "fastcgi_param  FLOPPIES_ASSETS /opt/floppcgi;" \
		>> "$NGINX_FCGI_CONFIG_PATH"
fi

# Copy the nginx service to /etc/systemd/system.
if [ ! -f /etc/systemd/system/floppcgi.service ]; then
	echo "installing systemd service..."
	cp -v floppcgi.service /etc/systemd/system || exit 1
	sed -i "s|/opt/floppcgi|$SCRIPT_DIR|g" \
		/etc/systemd/system/floppcgi.service || exit 1
	systemctl daemon-reload || exit 1
	systemctl enable --now floppcgi || exit 1
fi

