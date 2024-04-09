################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := C:\tools\build\make381\rm -f

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Demo
BUILD_ARTIFACT_EXTENSION := img
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
RAW_BINARY += \
Demo.elf \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: Demo.img

# Tool invocations
Demo.img: $(RAW_BINARY) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: Q161 Binary Packer'
	C:\tools\NOV30\vpostype\Q161\dtools  mkappimg $(RAW_BINARY) "Demo.img"
	@echo 'Finished building target: $@'
	@echo ' '

Demo.elf: $(OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Q161 Linker'
	C:\gcc-arm-none-eabi\bin\arm-none-eabi-ld -o "Demo.elf" $(OBJS) "C:\tools\NOV30\vpostype\Q161\inits\core_stub.o" "C:\tools\NOV30\vpostype\Q161\lib\libQREncode.a" "C:\tools\NOV30\vpostype\Q161\lib\libimageCore.a" "C:\tools\NOV30\vpostype\Q161\lib\libmbedtls.a" "C:\tools\NOV30\vpostype\Q161\lib\libmqttClient.a" "C:\gcc-arm-none-eabi\libc\libc.a" "C:\gcc-arm-none-eabi\libc\libm.a" "C:\gcc-arm-none-eabi\lib\gcc\arm-none-eabi\7.2.1\thumb\v7-ar\fpv3\hard\libgcc.a" -Map Demo.map -nostdlib --gc-sections -T C:\tools\NOV30\vpostype\Q161\inits\app_flashimg.ld
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Demo.elf Demo.img
	-@echo ' '

post-build:
	-C:\tools\NOV30\vpostype\Q161\pack Demo C:\tools & copy C:\tools\NOV30\vpostype\Q161\prk_json.txt .\prk_json.txt & C:\tools/NOV30/vpostype/Q161/VanstoneSign Demo.img & del prk_json.txt
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
