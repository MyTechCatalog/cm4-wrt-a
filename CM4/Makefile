# 
#
default : picod

picod:
	cd build && make -j$(nproc)

docker-cross:
	docker build . -f Dockerfile.cross --tag cross-slim

crossArchiveName = cross-slim_image.tar.gz
picoArchiveName = pico-image.tar.gz

save-docker-cross: ${crossArchiveName}
save-docker-cross-pico: ${picoArchiveName}

${crossArchiveName}:
	@[ ! -f $@ ] && { echo "Saving: Docker Cross-Compilation Image."; } &&\
	{ docker save cross-slim:latest | gzip > $@; }

${picoArchiveName}:
	@[ ! -f $@ ] && { echo "Saving: Docker RPi Pico Cross-Compilation Image."; } &&\
	{ docker save pico-image | gzip > $@; }

restore-docker-cross: ${crossArchiveName}
	@echo "Restoring: $<"
	docker load -i $<
restore-docker-cross-pico: ${picoArchiveName}
	@echo "Restoring: $<"
	docker load -i $<

all:
	[ -d build ] && { rm -rf build; } && { mkdir build; } || { mkdir build; }
	{ cd build && cmake -DCMAKE_TOOLCHAIN_FILE=${CROSS_TOOLCHAIN} \
	-DCMAKE_INSTALL_PREFIX=${CROSS_INSTALL_PREFIX} ../ && make -j$(nproc); }
debug:
	[ -d build ] && { rm -rf build; } && { mkdir build; } || { mkdir build; }
	{ cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=${CROSS_TOOLCHAIN} \
	-DCMAKE_INSTALL_PREFIX=${CROSS_INSTALL_PREFIX} ../ && make -j$(nproc); }
clean:
	[ -d build ] && { cd build && make clean; } || true
package: clean
	[ -d build ] && { rm -rf build; } && { mkdir build; } || { mkdir build; }
	{ cd build && cmake -DCMAKE_TOOLCHAIN_FILE=${CROSS_TOOLCHAIN} \
	-DCMAKE_INSTALL_PREFIX=${CROSS_INSTALL_PREFIX} ../; }
	CC=$C_COMPILER_ARM_LINUX CXX=$CXX_COMPILER_ARM_LINUX dpkg-buildpackage -d -us -uc -b --host-arch arm64
install:
	install -D -m 0755 ./build/picod /usr/bin/picod	
	install -D -m 0755 ./build/pico-cli /usr/bin/pico-cli
	install -D -m 0644 ./picod.service /etc/systemd/system/picod.service
	install -D -m 0644 ./picod.conf /etc/picod/picod.conf	
	sed -i 's/ttyAMA1/ttyAMA3/g' /etc/picod/picod.conf
	cp -r ./website /etc/picod/
	systemctl daemon-reload
	systemctl enable picod.service
	systemctl start picod.service
	sleep 1 && systemctl -l status picod.service
