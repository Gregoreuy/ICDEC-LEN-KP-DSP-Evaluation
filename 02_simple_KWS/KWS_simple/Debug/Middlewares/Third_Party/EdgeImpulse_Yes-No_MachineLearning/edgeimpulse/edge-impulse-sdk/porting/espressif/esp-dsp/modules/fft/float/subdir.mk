################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.c \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.c \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.c 

C_DEPS += \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.d \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.d \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.d 

OBJS += \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.o \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.o \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.o 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/%.o Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/%.su Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/%.cyclo: ../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/%.c Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5A9xx -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-EdgeImpulse_Yes-2d-No_MachineLearning-2f-edgeimpulse-2f-edge-2d-impulse-2d-sdk-2f-porting-2f-espressif-2f-esp-2d-dsp-2f-modules-2f-fft-2f-float

clean-Middlewares-2f-Third_Party-2f-EdgeImpulse_Yes-2d-No_MachineLearning-2f-edgeimpulse-2f-edge-2d-impulse-2d-sdk-2f-porting-2f-espressif-2f-esp-2d-dsp-2f-modules-2f-fft-2f-float:
	-$(RM) ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_bitrev_tables_fc32.su ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ae32.su ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/float/dsps_fft2r_fc32_ansi.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-EdgeImpulse_Yes-2d-No_MachineLearning-2f-edgeimpulse-2f-edge-2d-impulse-2d-sdk-2f-porting-2f-espressif-2f-esp-2d-dsp-2f-modules-2f-fft-2f-float

