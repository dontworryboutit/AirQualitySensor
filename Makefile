PORT := $(shell arduino-cli board list | grep -i MKR1000 | awk '{print $$1}')
FQBN := "arduino:samd:mkr1000"
PROJECT_NAME = Blink

connect:
	cd src/$(PROJECT_NAME) && arduino-cli board attach serial://$(PORT) $(PROJECT_NAME)

compile: connect
	cd src/$(PROJECT_NAME)  && arduino-cli -vvv compile --fqbn $(FQBN) $(PROJECT_NAME)


upload: compile
	cd src/$(PROJECT_NAME)  && arduino-cli -vvv upload -p $(PORT) --fqbn $(FQBN) $(PROJECT_NAME)


