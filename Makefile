# https://github.com/arduino/arduino-cli/releases

port := $(shell python3 board_detect.py)

default:
	arduino-cli compile --fqbn=arduino:avr:nano:cpu=atmega328old code

upload:
	@# echo $(port)
	arduino-cli compile --fqbn=arduino:avr:nano:cpu=atmega328old code
	arduino-cli -v upload -p "${port}" --fqbn=arduino:avr:nano:cpu=atmega328old code

install_platform:
	arduino-cli core install arduino:avr

deps:
	arduino-cli lib install "Etherkit Si5351"
	arduino-cli lib install "LiquidCrystal"


install_arduino_cli:
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
