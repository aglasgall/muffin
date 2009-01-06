all:
	make -C src
	sh update_image.sh
clean:
	make -C src clean

